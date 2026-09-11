#include "gpu_task_runner.h"
#include "../../util/dstack.h"
#include "../../util/errors.h"
#include <servers/rendering/rendering_device_binds.h>
#include "../../util/godot/classes/rendering_device.h"
#include "../../util/godot/classes/rendering_server.h"
#include "../../util/math/funcs.h"
#include "../../util/memory/memory.h"
#include "../../util/profiling.h"

#include "../../shaders/shaders.h"

namespace voxel {

GPUTaskRunner::GPUTaskRunner() {}

GPUTaskRunner::~GPUTaskRunner() {
	stop();

	// 此时不应有任何任务，我们在销毁 RenderingDevice 之前于线程中删除它们。
	// 但理论上，在这之后仍然没有什么能阻止任务被添加……
	for (IGPUTask *task : _shared_tasks) {
		VOXEL_DELETE(task);
	}
}

void GPUTaskRunner::start() {
	VOXEL_ASSERT(!_running);
	VOXEL_PRINT_VERBOSE("Starting GPUTaskRunner");
	_running = true;
	_thread.start(
			[](void *p_userdata) {
				GPUTaskRunner *runner = static_cast<GPUTaskRunner *>(p_userdata);
				runner->thread_func();
			},
			this
	);
}

void GPUTaskRunner::stop() {
	if (!_running) {
		VOXEL_PRINT_VERBOSE("GPUTaskRunner::stop() was called but it wasn't running.");
		return;
	}
	_running = false;
	_semaphore.post();
	_thread.wait_to_finish();
}

bool GPUTaskRunner::is_running() const {
	return _running;
}

void GPUTaskRunner::push(IGPUTask *task) {
	VOXEL_ASSERT_RETURN(task != nullptr);
	MutexLock mlock(_mutex);
	_shared_tasks.push_back(task);
	_semaphore.post();
	++_pending_count;
}

unsigned int GPUTaskRunner::get_pending_task_count() const {
	return _pending_count;
}

// RenderingDevice *GPUTaskRunner::get_rendering_device() const {
// 	MutexLock mlock(_rendering_device_ptr_mutex);
// 	return _rendering_device;
// }

void GPUTaskRunner::thread_func() {
	VOXEL_PROFILE_SET_THREAD_NAME("Voxel GPU tasks");
	VOXEL_DSTACK();

	{
		VOXEL_PRINT_VERBOSE("Creating Voxel RenderingDevice");
		// MutexLock mlock(_rendering_device_ptr_mutex);
		// 我们必须在使用 RenderingDevice 的同一线程中创建它，否则其某些方法中的线程安全保护会触发错误。
		// 这反过来影响了许多关于如何使用它管理资源的设计决策……
		_rendering_device = RenderingServer::get_singleton()->create_local_rendering_device();
	}

	if (_rendering_device == nullptr) {
		VOXEL_PRINT_VERBOSE("Could not create local RenderingDevice, GPU functionality won't be supported.");
		return;
	}

	_storage_buffer_pool.set_rendering_device(_rendering_device);
	_base_resources.load(*_rendering_device);

	StdVector<IGPUTask *> tasks;

	// 对于需要将结果下载回 CPU 的任务，我们使用公共输出缓冲区，
	// 因为由于 Godot 的 API 是同步的，调用一次 `buffer_get_data` 比多次调用更便宜。
	RID shared_output_storage_buffer_rid;
	unsigned int shared_output_storage_buffer_capacity = 0;
	struct SBRange {
		unsigned int position;
		unsigned int size;
	};
	StdVector<SBRange> shared_output_storage_buffer_segments;

	// Godot 不支持异步计算，因此要从计算着色器获取结果，唯一的方法是同步设备，等待所有内容完成。
	// 所以与其一次运行一个着色器，我们一次运行几个。
	// 每帧执行多少也不明确。
	// 在 nVidia 1060 上，4 个任务对于细节渲染来说已经足够了，但对于成本不同的任务，可能需要
	// 不同的配额以防止渲染变慢……
	const unsigned int batch_count = 16;

	while (_running) {
		{
			MutexLock mlock(_mutex);
			tasks = std::move(_shared_tasks);
		}
		if (tasks.size() == 0) {
			_semaphore.wait();
			continue;
		}

		VOXEL_ASSERT(_rendering_device != nullptr);
		GPUTaskContext ctx(*_rendering_device, _storage_buffer_pool, _base_resources);

		for (size_t begin_index = 0; begin_index < tasks.size(); begin_index += batch_count) {
			VOXEL_PROFILE_SCOPE_NAMED("Batch");

			const size_t end_index = math::min(begin_index + batch_count, tasks.size());

			unsigned int required_shared_output_buffer_size = 0;
			shared_output_storage_buffer_segments.clear();

			// 获取本批次需要从 GPU 下载多少数据
			for (size_t i = begin_index; i < end_index; ++i) {
				IGPUTask *task = tasks[i];
				const unsigned size = task->get_required_shared_output_buffer_size();
				// TODO 我们是否应该用某种对齐方式来填充分段？
				shared_output_storage_buffer_segments.push_back(SBRange{ required_shared_output_buffer_size, size });
				required_shared_output_buffer_size += size;
			}

			// 确保分配的存储缓冲区能够容纳本批次的所有输出数据
			if (required_shared_output_buffer_size > shared_output_storage_buffer_capacity) {
				VOXEL_PROFILE_SCOPE_NAMED("Resize shared output buffer");
				if (shared_output_storage_buffer_rid.is_valid()) {
					godot::free_rendering_device_rid(ctx.rendering_device, shared_output_storage_buffer_rid);
				}
				// TODO 是否要按某个倍数向上调整大小？
				shared_output_storage_buffer_rid =
						ctx.rendering_device.storage_buffer_create(required_shared_output_buffer_size);
				shared_output_storage_buffer_capacity = required_shared_output_buffer_size;
				VOXEL_ASSERT_CONTINUE(shared_output_storage_buffer_rid.is_valid());
			}

			ctx.shared_output_buffer_rid = shared_output_storage_buffer_rid;

			// 准备任务
			for (size_t i = begin_index; i < end_index; ++i) {
				VOXEL_PROFILE_SCOPE_NAMED("GPU Task Prepare");

				const SBRange range = shared_output_storage_buffer_segments[i - begin_index];
				ctx.shared_output_buffer_begin = range.position;
				ctx.shared_output_buffer_size = range.size;

				IGPUTask *task = tasks[i];
				task->prepare(ctx);
			}

			// 提交工作并等待完成
			{
				VOXEL_PROFILE_SCOPE_NAMED("RD Submit");
				ctx.rendering_device.submit();
			}
			{
				VOXEL_PROFILE_SCOPE_NAMED("RD Sync");
				ctx.rendering_device.sync();
			}

			// 从共享缓冲区下载数据
			if (required_shared_output_buffer_size > 0 && shared_output_storage_buffer_rid.is_valid()) {
				VOXEL_PROFILE_SCOPE_NAMED("Download shared output buffer");
				// 遗憾的是，我们无法为该缓冲区复用内存，Godot 总是希望使用 malloc 分配它。
				// 该缓冲区可能有几兆字节长……
				ctx.downloaded_shared_output_data = ctx.rendering_device.buffer_get_data(
						shared_output_storage_buffer_rid, 0, required_shared_output_buffer_size
				);
			}

			// 收集结果并完成任务
			for (size_t i = begin_index; i < end_index; ++i) {
				VOXEL_PROFILE_SCOPE_NAMED("GPU Task Collect");

				const SBRange range = shared_output_storage_buffer_segments[i - begin_index];
				ctx.shared_output_buffer_begin = range.position;
				ctx.shared_output_buffer_size = range.size;

				IGPUTask *task = tasks[i];
				task->collect(ctx);
				VOXEL_DELETE(task);
				--_pending_count;
			}

			ctx.downloaded_shared_output_data = PackedByteArray();
		}

		tasks.clear();
	}

	VOXEL_ASSERT(tasks.size() == 0);

	// 清理
	if (shared_output_storage_buffer_rid.is_valid()) {
		godot::free_rendering_device_rid(*_rendering_device, shared_output_storage_buffer_rid);
	}

	{
		MutexLock mlock(_mutex);
		for (IGPUTask *task : _shared_tasks) {
			VOXEL_DELETE(task);
		}
	}

	_base_resources.clear(*_rendering_device);

	if (is_verbose_output_enabled()) {
		_storage_buffer_pool.debug_print();
	}

	_storage_buffer_pool.clear();
	_storage_buffer_pool.set_rendering_device(nullptr);

	{
		VOXEL_PRINT_VERBOSE("Freeing Voxel RenderingDevice");
		// MutexLock mlock(_rendering_device_ptr_mutex);
		memdelete(_rendering_device);
		_rendering_device = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BaseGPUResources::load(RenderingDevice &rd) {
	VOXEL_PROFILE_SCOPE();
	{
		VOXEL_PROFILE_SCOPE_NAMED("Base Compute Shaders");

		VOXEL_PRINT_VERBOSE("Loading VoxelEngine shaders");

		dilate_normalmap_shader.load_from_glsl(rd, g_dilate_normalmap_shader, "voxel.dilate_normalmap");
		detail_gather_hits_shader.load_from_glsl(rd, g_detail_gather_hits_shader, "voxel.detail_gather_hits");
		detail_normalmap_shader.load_from_glsl(rd, g_detail_normalmap_shader, "voxel.detail_normalmap_shader");

		detail_modifier_sphere_shader.load_from_glsl(
				rd,
				String(g_detail_modifier_shader_template_0) + String(g_modifier_sphere_shader_snippet) +
						String(g_detail_modifier_shader_template_1),
				"voxel.detail_modifier_sphere_shader"
		);

		detail_modifier_mesh_shader.load_from_glsl(
				rd,
				String(g_detail_modifier_shader_template_0) + String(g_modifier_mesh_shader_snippet) +
						String(g_detail_modifier_shader_template_1),
				"voxel.detail_modifier_mesh_shader"
		);

		block_modifier_sphere_shader.load_from_glsl(
				rd,
				String(g_block_modifier_shader_template_0) + String(g_modifier_sphere_shader_snippet) +
						String(g_block_modifier_shader_template_1),
				"voxel.block_modifier_sphere_shader"
		);

		block_modifier_mesh_shader.load_from_glsl(
				rd,
				String(g_block_modifier_shader_template_0) + String(g_modifier_mesh_shader_snippet) +
						String(g_block_modifier_shader_template_1),
				"voxel.block_modifier_mesh_shader"
		);
	}

	{
		Ref<RDSamplerState> sampler_state;
		sampler_state.instantiate();
		// 使用采样器是为了利用它们的插值特性。
		// 否则我认为没有理由使用采样器。
		sampler_state->set_mag_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
		sampler_state->set_min_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
		filtering_sampler_rid = voxel::godot::sampler_create(rd, **sampler_state);
	}
}

void BaseGPUResources::clear(RenderingDevice &rd) {
	dilate_normalmap_shader.clear(rd);
	detail_gather_hits_shader.clear(rd);
	detail_normalmap_shader.clear(rd);
	detail_modifier_sphere_shader.clear(rd);
	detail_modifier_mesh_shader.clear(rd);
	block_modifier_sphere_shader.clear(rd);
	block_modifier_mesh_shader.clear(rd);

	voxel::godot::free_rendering_device_rid(rd, filtering_sampler_rid);
	filtering_sampler_rid = RID();
}

} // namespace voxel
