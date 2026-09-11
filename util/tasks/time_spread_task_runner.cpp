#include "time_spread_task_runner.h"
#include "../containers/std_vector.h"
#include <core/os/time.h>
#include "../memory/memory.h"
#include "../profiling.h"

namespace voxel {

TimeSpreadTaskRunner::~TimeSpreadTaskRunner() {
	flush();
}

void TimeSpreadTaskRunner::push(ITimeSpreadTask *task, Priority priority) {
	Queue &queue = _queues[priority];
	MutexLock lock(queue.tasks_mutex);
	queue.tasks.push(task);
}

void TimeSpreadTaskRunner::push(Span<ITimeSpreadTask *> tasks, Priority priority) {
	Queue &queue = _queues[priority];
	MutexLock lock(queue.tasks_mutex);
	for (unsigned int i = 0; i < tasks.size(); ++i) {
		queue.tasks.push(tasks[i]);
	}
}

void TimeSpreadTaskRunner::process(uint64_t time_budget_usec) {
	VOXEL_PROFILE_SCOPE();
	const Time &time = *Time::get_singleton();

	static thread_local FixedArray<StdVector<ITimeSpreadTask *>, PRIORITY_COUNT> tls_postponed_tasks;
	for (unsigned int i = 0; i < tls_postponed_tasks.size(); ++i) {
		VOXEL_ASSERT(tls_postponed_tasks[i].size() == 0);
	}

	const uint64_t time_before = time.get_ticks_usec();

	// 至少执行一个任务
	do {
		ITimeSpreadTask *task = nullptr;

		// 优先从高优先级队列取用
		unsigned int queue_index;
		for (queue_index = 0; queue_index < _queues.size(); ++queue_index) {
			Queue &queue = _queues[queue_index];
			MutexLock lock(queue.tasks_mutex);
			if (queue.tasks.size() != 0) {
				task = queue.tasks.front();
				queue.tasks.pop();
				break;
			}
		}

		if (task == nullptr) {
			break;
		}

		TimeSpreadTaskContext ctx;
		task->run(ctx);

		if (ctx.postpone) {
			tls_postponed_tasks[queue_index].push_back(task);
		} else {
			// TODO 改为调用回收函数？
			VOXEL_DELETE(task);
		}

	} while (time.get_ticks_usec() - time_before < time_budget_usec);

	// 将被推迟的任务推回队列
	for (unsigned int queue_index = 0; queue_index < tls_postponed_tasks.size(); ++queue_index) {
		StdVector<ITimeSpreadTask *> &tasks = tls_postponed_tasks[queue_index];
		if (tasks.size() > 0) {
			push(to_span(tasks), Priority(queue_index));
			tasks.clear();
		}
	}
}

void TimeSpreadTaskRunner::flush() {
	// 注意，这里假设已没有其他线程能再推送任务。
	// 调用方负责在刷新前停止它们。
	while (get_pending_count() != 0) {
		process(100);
		// Sleep?
	}
}

unsigned int TimeSpreadTaskRunner::get_pending_count() const {
	unsigned int count = 0;
	for (unsigned int queue_index = 0; queue_index < _queues.size(); ++queue_index) {
		const Queue &queue = _queues[queue_index];
		MutexLock lock(queue.tasks_mutex);
		count += queue.tasks.size();
	}
	return count;
}

} // namespace voxel
