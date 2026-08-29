#include "load_block_data_task.h"
#include "../engine/voxel_engine.h"
#include "../generators/generate_block_task.h"
#include "../storage/voxel_buffer.h"
#include "../storage/voxel_data.h"
#include "../util/dstack.h"
#include "../util/io/log.h"
#include "../util/profiling.h"

namespace voxel {

namespace {
std::atomic_int g_debug_load_block_tasks_count = { 0 };
}

LoadBlockDataTask::LoadBlockDataTask(
		VolumeID p_volume_id,
		Vector3i p_block_pos,
		uint8_t p_lod,
		uint8_t p_block_size,
		bool p_request_instances,
		std::shared_ptr<StreamingDependency> p_stream_dependency,
		PriorityDependency p_priority_dependency,
		bool generate_cache_data,
		bool generator_use_gpu,
		const std::shared_ptr<VoxelData> &vdata,
		TaskCancellationToken cancellation_token
) :
		_priority_dependency(p_priority_dependency),
		_position(p_block_pos),
		_volume_id(p_volume_id),
		_lod_index(p_lod),
		_block_size(p_block_size),
#ifdef VOXEL_ENABLE_INSTANCER
		_request_instances(p_request_instances),
#endif
		_generate_cache_data(generate_cache_data),
#ifdef VOXEL_ENABLE_GPU
		_generator_use_gpu(generator_use_gpu),
#endif
		//_request_voxels(true),
		_stream_dependency(p_stream_dependency),
		_voxel_data(vdata),
		_cancellation_token(cancellation_token) {
	//
	++g_debug_load_block_tasks_count;
}

LoadBlockDataTask::~LoadBlockDataTask() {
	--g_debug_load_block_tasks_count;
}

int LoadBlockDataTask::debug_get_running_count() {
	return g_debug_load_block_tasks_count;
}

void LoadBlockDataTask::run(voxel::ThreadedTaskContext &ctx) {
	VOXEL_DSTACK();
	VOXEL_PROFILE_SCOPE();

	CRASH_COND(_stream_dependency == nullptr);
	Ref<VoxelStream> stream = _stream_dependency->stream;
	CRASH_COND(stream.is_null());

	ERR_FAIL_COND(_voxels != nullptr);
	_voxels = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
	const VoxelFormat format = _voxel_data->get_format();
	_voxels->create(Vector3iUtil::create(_block_size), &format);

	// TODO 我们应当考虑再次进行批量处理，但需要谨慎操作。
	// 每个任务对应一个区块，其优先级取决于到最近观察者的距离。
	// 如果要批量处理区块，也必须按距离来分组。

	// TODO 在可用时设置 max_lod_hint

	VoxelStream::VoxelQueryData voxel_query_data{ *_voxels, _position, _lod_index, VoxelStream::RESULT_ERROR };
	stream->load_voxel_block(voxel_query_data);

	if (voxel_query_data.result == VoxelStream::RESULT_ERROR) {
		ERR_PRINT("Error loading voxel block");

	} else if (voxel_query_data.result == VoxelStream::RESULT_BLOCK_NOT_FOUND) {
		if (_generate_cache_data) {
			Ref<VoxelGenerator> generator = _stream_dependency->generator;

			if (generator.is_valid()) {
				VoxelGenerator::BlockTaskParams params;
				params.voxels = _voxels;
				params.volume_id = _volume_id;
				params.block_position = _position;
				params.format = format;
				params.lod_index = _lod_index;
				params.block_size = _block_size;
				params.stream_dependency = _stream_dependency;
				params.priority_dependency = _priority_dependency;
#ifdef VOXEL_ENABLE_GPU
				params.use_gpu = _generator_use_gpu;
#endif
				params.data = _voxel_data;

				IThreadedTask *task = generator->create_block_task(params);

				VoxelEngine::get_singleton().push_async_task(task);
				_requested_generator_task = true;

			} else {
			// 如果没有生成器……该怎么办？由什么来定义那个空区块的格式？
			// 如果用户保留默认值倒没问题，否则格式不一致的区块可能会
			// 进入体积中，从而可能引发错误。
			// TODO 在体积上定义格式？
			}
		} else {
			_voxels.reset();
		}
	}

#ifdef VOXEL_ENABLE_INSTANCER
	if (_request_instances && stream->supports_instance_blocks()) {
		ERR_FAIL_COND(_instances != nullptr);

		VoxelStream::InstancesQueryData instances_query;
		instances_query.lod_index = _lod_index;
		instances_query.position_in_blocks = _position;
		stream->load_instance_blocks(Span<VoxelStream::InstancesQueryData>(&instances_query, 1));

		if (instances_query.result == VoxelStream::RESULT_ERROR) {
			ERR_PRINT("Error loading instance block");

		} else if (voxel_query_data.result == VoxelStream::RESULT_BLOCK_FOUND) {
			_instances = std::move(instances_query.data);
		}
		// 如果未找到，实例将返回空，
		// 这意味着它可以在网格化处理之后由实例生成器生成
	}
#endif

	_has_run = true;
}

TaskPriority LoadBlockDataTask::get_priority() {
	float closest_viewer_distance_sq;
	const TaskPriority p =
			_priority_dependency.evaluate(_lod_index, constants::TASK_PRIORITY_LOAD_BAND2, &closest_viewer_distance_sq);
	_too_far = closest_viewer_distance_sq > _priority_dependency.drop_distance_squared;
	return p;
}

bool LoadBlockDataTask::is_cancelled() {
	if (_stream_dependency->valid == false) {
		return true;
	}
	if (_cancellation_token.is_valid()) {
		return _cancellation_token.is_cancelled();
	}
	return _too_far;
}

void LoadBlockDataTask::apply_result() {
	if (VoxelEngine::get_singleton().is_volume_valid(_volume_id)) {
		// TODO 比较指针未必可靠
		// 请求的响应必须与当初请求时所使用的依赖项相匹配。
		// 如果不匹配，说明我们已不再关心该结果。
		if (_stream_dependency->valid && !_requested_generator_task) {
			VoxelEngine::BlockDataOutput o;
			o.voxels = _voxels;
#ifdef VOXEL_ENABLE_INSTANCER
			o.instances = std::move(_instances);
#endif
			o.position = _position;
			o.lod_index = _lod_index;
			o.dropped = !_has_run;
			o.max_lod_hint = _max_lod_hint;
			o.initial_load = false;
			o.type = VoxelEngine::BlockDataOutput::TYPE_LOADED;

			VoxelEngine::VolumeCallbacks callbacks = VoxelEngine::get_singleton().get_volume_callbacks(_volume_id);
			CRASH_COND(callbacks.data_output_callback == nullptr);
			callbacks.data_output_callback(callbacks.data, o);
		}

	} else {
		// 如果用户在请求尚未返回时移除了体积，就可能发生这种情况
		VOXEL_PRINT_VERBOSE("Stream data request response came back but volume wasn't found");
	}
}

} // namespace voxel
