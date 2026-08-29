#ifndef VOXEL_GENERATE_BLOCK_MULTIPASS_CB_TASK_H
#define VOXEL_GENERATE_BLOCK_MULTIPASS_CB_TASK_H

#include "../../engine/ids.h"
#include "../../engine/priority_dependency.h"
#include "../../engine/streaming_dependency.h"
#include "../../util/tasks/threaded_task.h"
#include "../voxel_generator.h"


namespace voxel {

class AsyncDependencyTracker;
class VoxelData;

class GenerateBlockMultipassCBTask : public IThreadedTask {
public:
	GenerateBlockMultipassCBTask(const VoxelGenerator::BlockTaskParams &params);
	~GenerateBlockMultipassCBTask();

	const char *get_debug_name() const override {
		return "GenerateBlockMultipassCBTask";
	}

	void run(ThreadedTaskContext &ctx) override;
	TaskPriority get_priority() override;
	bool is_cancelled() override;
	void apply_result() override;

	// 不是输入，但可以分配一个可复用的实例，避免在任务中分配一个
	std::shared_ptr<VoxelBuffer> voxels;

private:
	void run_cpu_generation();
	void run_stream_saving_and_finish();

	Vector3i _block_position;
	VoxelFormat _format;
	VolumeID _volume_id;
	uint8_t _lod_index;
	uint8_t _block_size;
	bool _drop_beyond_max_distance = true;
	PriorityDependency _priority_dependency;
	std::shared_ptr<StreamingDependency> _stream_dependency; // 用于保存生成器的输出
	std::shared_ptr<AsyncDependencyTracker> _tracker; // 用于异步编辑
	TaskCancellationToken _cancellation_token;

	bool _has_run = false;
	bool _too_far = false;
	uint8_t _stage = 0;
};

} // namespace voxel

#endif // VOXEL_GENERATE_BLOCK_MULTIPASS_CB_TASK_H
