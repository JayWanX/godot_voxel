#include "async_dependency_tracker.h"
#include "../memory/memory.h"
#include "threaded_task_runner.h"

namespace voxel {

AsyncDependencyTracker::AsyncDependencyTracker() :
		_count(0), _aborted(false), _tasks_have_started(false), _count_was_set(false) {}

AsyncDependencyTracker::AsyncDependencyTracker(int initial_count) :
		_count(initial_count), _aborted(false), _tasks_have_started(false), _count_was_set(true) {}

AsyncDependencyTracker::AsyncDependencyTracker(
		int initial_count, Span<IThreadedTask *> next_tasks, ScheduleNextTasksCallback scheduler_cb) :
		_count(initial_count),
		_aborted(false),
		_tasks_have_started(false),
		_count_was_set(true),
		_next_tasks_schedule_callback(scheduler_cb) {
	//
	VOXEL_ASSERT(scheduler_cb != nullptr);

	_next_tasks.resize(next_tasks.size());

	for (unsigned int i = 0; i < next_tasks.size(); ++i) {
		IThreadedTask *task = next_tasks[i];
#ifdef DEBUG_ENABLED
		for (unsigned int j = i + 1; j < next_tasks.size(); ++j) {
			// 不能重复添加同一个任务
			VOXEL_ASSERT(next_tasks[j] != task);
		}
#endif
		_next_tasks[i] = task;
	}
}

AsyncDependencyTracker::~AsyncDependencyTracker() {
	// 如果我们从这里销毁任务，说明已中止。它们尚未被调度，因此我们仍拥有
	// 其所有权，必须自行清理。
	for (auto it = _next_tasks.begin(); it != _next_tasks.end(); ++it) {
		IThreadedTask *task = *it;
		// TODO 或许应允许自定义，比如改为调用 `->dispose()` 函数？
		VOXEL_DELETE(task);
	}
}

void AsyncDependencyTracker::set_count(int count) {
	VOXEL_ASSERT_MSG(_count_was_set == false, "Count must not be set twice");
	VOXEL_ASSERT_MSG(_tasks_have_started == false, "Count must not be set after scheduling tasks");
	_count = count;
	_count_was_set = true;
}

void AsyncDependencyTracker::post_complete() {
	_tasks_have_started = true;
	// 注意，该类只允许将该计数器递减到零
	VOXEL_ASSERT_RETURN_MSG(_count > 0, "post_complete() called more times than expected");
	VOXEL_ASSERT_RETURN_MSG(_aborted == false, "post_complete() called after abortion");
	--_count;
	if (_count == 0 && _next_tasks.size() > 0) {
		VOXEL_ASSERT_RETURN(_next_tasks_schedule_callback != nullptr);
		_next_tasks_schedule_callback(to_span(_next_tasks));
		// 清理任务，因为一旦它们被调度，我们就不再拥有其所有权。
		_next_tasks.clear();
	}
	// 将后续任务放进此类而不是直接放进任务中的想法是，
	// 因为这会要求那些任务自己完成该工作，而且当等待多个任务时，
	// 究竟哪个拥有所有权并不明确。可能是其中最后完成的任意一个。
	// 将后续任务放在追踪器中则具有明确且唯一的所有权。
}

} // namespace voxel
