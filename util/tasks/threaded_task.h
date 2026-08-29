#ifndef THREADED_TASK_H
#define THREADED_TASK_H

#include "task_priority.h"
#include <cstdint>

namespace voxel {

struct ThreadedTaskContext {
	enum Status : uint8_t {
		// 任务已完成，将由 TaskRunner 放入已完成任务列表。稍后会被删除。
		// 这是默认状态。
		STATUS_COMPLETE = 0,
		// 任务未完成，稍后将由 TaskRunner 重新运行
		STATUS_POSTPONED = 1,
		// 任务未完成，将由另一个自定义任务重新调度。
		// TaskRunner 会直接丢弃其指针，不会将其放入已完成任务列表。
		// 最初加入该功能是为了能从任务 A 调度任务 B，并让 B 重新调度 A，以使用 A 中计算的结果
		STATUS_TAKEN_OUT = 2
	};

	// 线程在运行器线程池中的索引。可作为 thread_local 的替代方案用于数组索引。
	// 存储。
	const uint8_t thread_index;
	// 可由任务在运行后设置，以标识其状态
	Status status;
	// 当前任务的缓存优先级。若当前任务产生其他相关任务，复制它可能有用。
	const TaskPriority task_priority;
	// 若将其设为非空任务，它将在同一线程上紧接当前任务之后运行。
	// 这样做会将所有权交给 ThreadedTaskRunner。这些任务此前一定不能被运行器
	// 拥有过。此类任务的优先级无关紧要。
	// IThreadedTask *next_immediate_task;

	ThreadedTaskContext(uint8_t p_thread_index, TaskPriority p_priority) :
			thread_index(p_thread_index),
			// 默认情况下，若任务未设置该状态，则运行后被视为已完成
			status(STATUS_COMPLETE),
			task_priority(p_priority) {}

	// 允许在任务内部调度任务，而无需将其传入或使用全局变量
	// ThreadedTaskRunner &runner;
};

// 将在 `ThreadedTaskRunner` 中运行的任务的接口。
// 该任务将在另一个线程中运行。
class IThreadedTask {
public:
	virtual ~IThreadedTask() {}

	// 从线程池内部调用
	virtual void run(ThreadedTaskContext &ctx) = 0;

	// 便利方法，可由任务的调度器（通常在主线程上）调用，
	// 以应用结果。它不由线程池调用。
	virtual void apply_result(){};

	// 提示该任务在被调度后多久会执行。当任务很多时这一点很重要。
	// 数值越小表示优先级越高。
	// 两次调用之间可能改变。线程池会在一定时间间隔内定期轮询该值。
	virtual TaskPriority get_priority() {
		// 默认取最高优先级，因为这是最常见的期望。
		return TaskPriority::max();
	}

	// 可返回 `true`，使线程池跳过该任务
	virtual bool is_cancelled() {
		return false;
	}

	// 出于调试目的获取任务名称。返回的名称的生命周期必须覆盖引擎的执行期
	// （通常是字符串字面量）。
	virtual const char *get_debug_name() const {
		return "<unnamed>";
	}
};

} // namespace voxel

#endif // THREADED_TASK_H
