#ifndef VOXEL_ASYNC_DEPENDENCY_TRACKER_H
#define VOXEL_ASYNC_DEPENDENCY_TRACKER_H

#include "../containers/span.h"
#include "../containers/std_vector.h"
#include <atomic>

namespace voxel {

class IThreadedTask;

// 追踪一个或多个任务的状态。
// 任务应通过共享指针引用它。
class AsyncDependencyTracker {
public:
	// 创建一个尚未追踪任何已知任务的追踪器。你必须在调度任务前使用 `set_count`。
	AsyncDependencyTracker();

	// 创建一个将追踪 `initial_count` 个任务的追踪器。
	AsyncDependencyTracker(int initial_count);

	typedef void (*ScheduleNextTasksCallback)(Span<IThreadedTask *> tasks);

	// 备用构造函数，其中一组任务会在完成时一并被调度。
	// 所有后续任务将并行运行。
	// 若某个依赖被中止，这些任务将被销毁。
	AsyncDependencyTracker(int initial_count, Span<IThreadedTask *> next_tasks, ScheduleNextTasksCallback scheduler_cb);

	~AsyncDependencyTracker();

	// 设置依赖数量。仅当你难以预先知道要创建的任务数量时才使用它，但
	// 必须在这些任务被调度之前调用。
	void set_count(int count);

	// 当某个被追踪的依赖完成时调用
	void post_complete();

	// 当某个被追踪的依赖中止时调用
	void abort() {
		_aborted = true;
		_tasks_have_started = true;
	}

	// 若任何被追踪的任务被中止，返回 `true`。
	// 这通常意味着依赖于该追踪器的任务也可能被中止。
	bool is_aborted() const {
		return _aborted;
	}

	// 当所有被追踪的任务都已完成时返回 `true`
	bool is_complete() const {
		return _count == 0;
	}

	int get_remaining_count() const {
		return _count;
	}

	bool has_next_tasks() const {
		return _next_tasks.size() > 0;
	}

private:
	std::atomic_int _count;
	std::atomic_bool _aborted;
	std::atomic_bool _tasks_have_started;
	bool _count_was_set = false;
	StdVector<IThreadedTask *> _next_tasks;
	ScheduleNextTasksCallback _next_tasks_schedule_callback = nullptr;
};

} // namespace voxel

#endif // VOXEL_ASYNC_DEPENDENCY_TRACKER_H
