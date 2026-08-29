#ifndef VOXEL_GENERATE_COLUMN_MULTIPASS_TASK_H
#define VOXEL_GENERATE_COLUMN_MULTIPASS_TASK_H

#include "../../util/tasks/threaded_task.h"
#include "voxel_generator_multipass_cb.h"

namespace voxel {

class BufferedTaskScheduler;

// 此任务设计为从另一个任务调度。
// 在生成器的缓存中查找列，以便在特定列上运行一次 pass。
// 如果地图中至少有一列找不到，任务被取消，其所有调用者也应被取消。
// 否则：
// 如果某列不满足依赖要求：
//     - 如果另一个任务正在处理该列，当前任务将被推迟到稍后运行。
//     - 否则，会生成一个子任务来处理该依赖。
//       当前任务会排在以此方式生成的每个子任务之后。
// 否则，任务运行 pass，重新调度其调用者，然后返回。
//
// 使用此模式而非"金字塔差异"的原因之一是，它可以在没有任何假设的情况下被调用。即使地图
// 处于不一致状态，它也会在必要时返回结果。我们甚至可以决定覆盖状态。
// 它主动查找依赖，而不是假设它们由独立的逻辑加载。
class GenerateColumnMultipassTask : public IThreadedTask {
public:
	GenerateColumnMultipassTask(
			Vector2i p_column_position,
			VoxelFormat p_format,
			uint8_t p_block_size,
			uint8_t p_subpass_index,
			std::shared_ptr<VoxelGeneratorMultipassCBStructs::Internal> p_generator_internal,
			Ref<VoxelGeneratorMultipassCB> p_generator,
			TaskPriority p_priority,
			// 当当前任务完成时，它会递减给定的计数器，并在计数器到达 0 时
			// 将控制权返回给以下调用者任务。
			IThreadedTask *p_caller,
			std::shared_ptr<std::atomic_int> p_caller_dependency_count
	);

	~GenerateColumnMultipassTask();

	const char *get_debug_name() const override {
		return "GenerateColumnMultipassTask";
	}

	void run(ThreadedTaskContext &ctx) override;

	TaskPriority get_priority() override {
		return _priority;
	}

	// 目前不能为此 API 使用取消（那会阻止任务运行），因为任务必须运行
	// 才能重新调度其调用者。最终我们可能会找到将这种模式集成到框架中的方法。
	// bool is_cancelled() {}

private:
	void schedule_final_block_tasks(
			VoxelGeneratorMultipassCBStructs::Column &column,
			BufferedTaskScheduler &task_scheduler
	);
	void return_to_caller(bool success);

	Vector2i _column_position;
	VoxelFormat _format;
	TaskPriority _priority;
	uint8_t _block_size;
	uint8_t _subpass_index;
	bool _cancelled = false;
	std::shared_ptr<VoxelGeneratorMultipassCBStructs::Internal> _generator_internal;
	Ref<VoxelGeneratorMultipassCB> _generator;
	// 当 `caller_task_dependency_counter` 到达零时执行的任务。
	// 这意味着当前任务是由该任务生成以计算依赖的。
	IThreadedTask *_caller_task = nullptr;
	// 当任务以相当于"请求的数据块已被处理"的结果结束时递减的计数器。
	std::shared_ptr<std::atomic_int> _caller_task_dependency_counter;
	GenerateColumnMultipassTask *_caller_mp_task = nullptr;
};

} // namespace voxel

#endif // VOXEL_GENERATE_COLUMN_MULTIPASS_TASK_H
