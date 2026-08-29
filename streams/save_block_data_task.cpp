#include "save_block_data_task.h"
#include "../engine/voxel_engine.h"
#include "../generators/generate_block_task.h"
#include "../storage/voxel_buffer.h"
#include "../util/godot/core/string.h"
#include "../util/io/log.h"
#include "../util/profiling.h"
#include "../util/string/format.h"
#include "../util/tasks/async_dependency_tracker.h"

namespace voxel {

namespace {
std::atomic_int g_debug_save_block_tasks_count = { 0 };
}

SaveBlockDataTask::SaveBlockDataTask(
		VolumeID p_volume_id,
		Vector3i p_block_pos,
		uint8_t p_lod,
		std::shared_ptr<VoxelBuffer> p_voxels,
		std::shared_ptr<StreamingDependency> p_stream_dependency,
		std::shared_ptr<AsyncDependencyTracker> p_tracker,
		bool flush_on_last_tracked_task
) :
		_voxels(p_voxels),
		_position(p_block_pos),
		_volume_id(p_volume_id),
		_lod(p_lod),
		_save_instances(false),
		_save_voxels(true),
		_flush_on_last_tracked_task(flush_on_last_tracked_task),
		_stream_dependency(p_stream_dependency),
		_tracker(p_tracker) {
	//
	++g_debug_save_block_tasks_count;
}

#ifdef VOXEL_ENABLE_INSTANCER

SaveBlockDataTask::SaveBlockDataTask(
		VolumeID p_volume_id,
		Vector3i p_block_pos,
		uint8_t p_lod,
		UniquePtr<InstanceBlockData> p_instances,
		std::shared_ptr<StreamingDependency> p_stream_dependency,
		std::shared_ptr<AsyncDependencyTracker> p_tracker,
		bool flush_on_last_tracked_task
) :
		_instances(std::move(p_instances)),
		_position(p_block_pos),
		_volume_id(p_volume_id),
		_lod(p_lod),
		_save_instances(true),
		_save_voxels(false),
		_flush_on_last_tracked_task(flush_on_last_tracked_task),
		_stream_dependency(p_stream_dependency),
		_tracker(p_tracker) {
	//
	++g_debug_save_block_tasks_count;
}

#endif

SaveBlockDataTask::~SaveBlockDataTask() {
	--g_debug_save_block_tasks_count;
}

int SaveBlockDataTask::debug_get_running_count() {
	return g_debug_save_block_tasks_count;
}

void SaveBlockDataTask::run(voxel::ThreadedTaskContext &ctx) {
	VOXEL_PROFILE_SCOPE();

	CRASH_COND(_stream_dependency == nullptr);
	Ref<VoxelStream> stream = _stream_dependency->stream;
	VOXEL_ASSERT_RETURN_MSG(stream.is_valid(), "Save task was triggered without a stream, this is a bug");

	if (_save_voxels) {
		if (_voxels == nullptr) {
			if (_tracker != nullptr) {
				_tracker->abort();
			}
			VOXEL_PRINT_ERROR("Voxels to save shouldn't be null");
			return;
		}

		VoxelBuffer voxels_copy(VoxelBuffer::ALLOCATOR_POOL);
		// 注意，这里我们没有锁定体素。这一步本应在调度该任务时完成。
		// 如果这不是一份副本，意味着它所属的地图无论如何都将被卸载。
		// TODO 优化：那份副本是否必要？在发出请求时它可能已经制作过了
		_voxels->copy_to(voxels_copy, true);
		_voxels = nullptr;
		VoxelStream::VoxelQueryData q{ voxels_copy, _position, _lod, VoxelStream::RESULT_ERROR };
		stream->save_voxel_block(q);
	}

#ifdef VOXEL_ENABLE_INSTANCER
	if (_save_instances) {
		if (stream->supports_instance_blocks()) {
			// 如果提供的数据为空，意味着这个实例区块从未被修改过。
			// 由于我们处于保存请求中，保存的数据将恢复为未修改状态。
			// 另一方面，如果我们想表达“此处的一切都已被删除”这一事实，
			// 那么数据就不应该为空。

			VOXEL_PRINT_VERBOSE(format(
					"Saving instance block {} lod {} with data {}", _position, static_cast<int>(_lod), _instances.get()
			));

			VoxelStream::InstancesQueryData instances_query{
				std::move(_instances), _position, _lod, VoxelStream::RESULT_ERROR
			};
			stream->save_instance_blocks(Span<VoxelStream::InstancesQueryData>(&instances_query, 1));

		} else {
			VOXEL_PRINT_WARNING_ONCE(
					format("Tried to save instance block, but {} does not support them.", String(stream->get_class()))
			);
		}
	}
#endif

	if (_tracker != nullptr) {
		if (_flush_on_last_tracked_task && _tracker->get_remaining_count() == 1) {
			// 这是被追踪的保存任务组中的最后一个任务，现在可以执行刷新了
			stream->flush();
		}
		_tracker->post_complete();
	}

	_has_run = true;
}

TaskPriority SaveBlockDataTask::get_priority() {
	TaskPriority p;
	p.band2 = constants::TASK_PRIORITY_SAVE_BAND2;
	p.band3 = constants::TASK_PRIORITY_BAND3_DEFAULT;
	return p;
}

bool SaveBlockDataTask::is_cancelled() {
	return false;
}

void SaveBlockDataTask::apply_result() {
	if (VoxelEngine::get_singleton().is_volume_valid(_volume_id)) {
		if (_stream_dependency->valid) {
			// TODO 也许应该把保存与加载的回调分开？
			VoxelEngine::BlockDataOutput o;
			o.position = _position;
			o.lod_index = _lod;
			o.dropped = !_has_run;
			o.max_lod_hint = false; // 未使用
			o.initial_load = false; // 未使用
			o.had_instances = _save_instances;
			o.had_voxels = _save_voxels;
			o.type = VoxelEngine::BlockDataOutput::TYPE_SAVED;

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
