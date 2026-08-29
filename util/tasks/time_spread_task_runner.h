#ifndef VOXEL_TIME_SPREAD_TASK_RUNNER_H
#define VOXEL_TIME_SPREAD_TASK_RUNNER_H

#include "../containers/fixed_array.h"
#include "../containers/span.h"
#include "../containers/std_queue.h"
#include "../thread/mutex.h"
#include <cstdint>

namespace voxel {

struct TimeSpreadTaskContext {
	// 若任务将其设为 `true`，
	// 它将在运行器下次被处理时重新调度再次运行。
	// 否则，任务在运行后会被销毁。
	bool postpone = false;
};

class ITimeSpreadTask {
public:
	virtual ~ITimeSpreadTask() {}
	virtual void run(TimeSpreadTaskContext &ctx) = 0;
};

// 在调用者线程中运行任务，每次调用有时间预算。有点类似于协程。
class TimeSpreadTaskRunner {
public:
	enum Priority { //
		PRIORITY_NORMAL = 0,
		PRIORITY_LOW = 1,
		PRIORITY_COUNT
	};

	~TimeSpreadTaskRunner();

	// 入队是线程安全的。
	void push(ITimeSpreadTask *task, Priority priority = PRIORITY_NORMAL);
	void push(Span<ITimeSpreadTask *> tasks, Priority priority = PRIORITY_NORMAL);

	void process(uint64_t time_budget_usec);
	void flush();
	unsigned int get_pending_count() const;

private:
	struct Queue {
		StdQueue<ITimeSpreadTask *> tasks;
		// TODO 优化：简单的线程安全。目前应已足够。
		BinaryMutex tasks_mutex;
	};
	FixedArray<Queue, PRIORITY_COUNT> _queues;
};

} // namespace voxel

#endif // VOXEL_TIME_SPREAD_TASK_RUNNER_H
