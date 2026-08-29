#include "load_all_blocks_data_task.h"
#include "../engine/voxel_engine.h"
#include "../storage/voxel_data.h"
#include "../util/io/log.h"
#include "../util/profiling.h"
#include "../util/string/format.h"

namespace voxel {

void LoadAllBlocksDataTask::run(voxel::ThreadedTaskContext &ctx) {
	VOXEL_PROFILE_SCOPE();

	CRASH_COND(stream_dependency == nullptr);
	Ref<VoxelStream> stream = stream_dependency->stream;
	CRASH_COND(stream.is_null());

	stream->load_all_blocks(_result);

	VOXEL_PRINT_VERBOSE(format("Loaded {} blocks for volume {}", _result.blocks.size(), volume_id));
}

TaskPriority LoadAllBlocksDataTask::get_priority() {
	return TaskPriority();
}

bool LoadAllBlocksDataTask::is_cancelled() {
	return !stream_dependency->valid;
}

void LoadAllBlocksDataTask::apply_result() {
	if (VoxelEngine::get_singleton().is_volume_valid(volume_id)) {
		// TODO 比较指针未必可靠
		// 请求的响应必须与当初请求时所使用的依赖项相匹配。
		// 如果不匹配，说明我们已不再关心该结果。
		if (stream_dependency->valid) {
			VoxelEngine::VolumeCallbacks callbacks = VoxelEngine::get_singleton().get_volume_callbacks(volume_id);
			ERR_FAIL_COND(callbacks.data_output_callback == nullptr);

			for (auto it = _result.blocks.begin(); it != _result.blocks.end(); ++it) {
				VoxelStream::FullLoadingResult::Block &rb = *it;

				VoxelEngine::BlockDataOutput o;
				o.voxels = rb.voxels;
#ifdef VOXEL_ENABLE_INSTANCER
				o.instances = std::move(rb.instances_data);
#endif
				o.position = rb.position;
				o.lod_index = rb.lod;
				o.dropped = false;
				o.max_lod_hint = false;
				o.initial_load = true;

				callbacks.data_output_callback(callbacks.data, o);
			}

			data->set_full_load_completed(true);
		}

	} else {
		// 如果用户在请求尚未返回时移除了体积，就可能发生这种情况
		VOXEL_PRINT_VERBOSE("Stream data request response came back but volume wasn't found");
	}
}

} // namespace voxel