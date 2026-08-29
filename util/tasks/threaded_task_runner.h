#ifndef VOXEL_THREADED_TASK_RUNNER_H
#define VOXEL_THREADED_TASK_RUNNER_H

#include "../containers/container_funcs.h"
#include "../containers/fixed_array.h"
#include "../containers/span.h"
#include "../containers/std_queue.h"
#include "../containers/std_vector.h"
#include "../profiling.h"
#include "../string/std_string.h"
#include "../thread/mutex.h"
#include "../thread/semaphore.h"
#include "../thread/thread.h"
#include "threaded_task.h"

// 用于调试
// #define VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS

#ifdef VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS
#include "../containers/std_unordered_map.h"
#endif

#include <atomic>

namespace voxel {

// 基于动态优先级执行批量任务的通用线程池
class ThreadedTaskRunner {
public:
	static constexpr uint32_t MAX_THREADS = 128;

	enum State { //
		STATE_RUNNING = 0,
		STATE_PICKING,
		STATE_WAITING,
		STATE_STOPPED
	};

	ThreadedTaskRunner();
	~ThreadedTaskRunner();

	// 设置名称前缀，以便在调试工具中识别该池的线程。
	// 必须在配置线程数之前调用。
	void set_name(const char *name);

	// TODO 增加在运行时修改它而不跳过任务的能力
	// 任务入队后无法更改
	void set_thread_count(uint32_t count);
	uint32_t get_thread_count() const {
		return _thread_count;
	}

	// TODO 增加在运行时修改它的能力
	// 任务优先级会随时间改变，但在任务很多时过于频繁地计算代价很高，
	// 因此会被缓存。这里设置轮询任务优先级的频率。
	// 任务入队后无法更改。
	void set_priority_update_period(uint32_t milliseconds);

	// TODO 是否应期望任务为唯一指针？

	// 调度一个任务。
	// 所有权不会传递给线程池，因此若想删除任务，请确保在完成时取回它们。
	// 所有以 `serial=true` 调度的任务将依次运行，每次只使用一个线程。
	// 以 `serial=false` 调度的任务可使用多个线程并行运行。
	// 当这类任务因锁定共享资源而无法并行时，串行执行很有用。这能避免
	// 所有线程都被等待中的任务占满。
	void enqueue(IThreadedTask *task, bool serial);
	// 一次性调度多个任务。涉及更少内部加锁。
	void enqueue(Span<IThreadedTask *> new_tasks, bool serial);

	template <typename F>
	void dequeue_completed_tasks(F f) {
		VOXEL_PROFILE_SCOPE();
		StdVector<IThreadedTask *> &temp = get_completed_tasks_temp_tls();
		VOXEL_ASSERT(temp.size() == 0);
		{
			MutexLock lock(_completed_tasks_mutex);
			append_array(temp, _completed_tasks);
			_completed_tasks.clear();
			// std::move 不保证保留 vector 的容量
			// temp = std::move(_completed_tasks);
		}
		for (IThreadedTask *task : temp) {
#ifdef VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS
			debug_remove_owned_task(task);
#endif
			f(task);
		}
		temp.clear();
	}

	// 阻塞并等待所有任务完成（假设不会再有新任务加入！）
	void wait_for_all_tasks();

	State get_thread_debug_state(uint32_t i) const;
	const char *get_thread_debug_task_name(unsigned int thread_index) const;
	unsigned int get_debug_remaining_tasks() const;

private:
	static StdVector<IThreadedTask *> &get_completed_tasks_temp_tls();

	struct TaskItem {
		IThreadedTask *task = nullptr;
		TaskPriority cached_priority;
		bool is_serial = false;
		ThreadedTaskContext::Status status = ThreadedTaskContext::STATUS_COMPLETE;
	};

	struct ThreadData {
		Thread thread;
		ThreadedTaskRunner *pool = nullptr;
		uint32_t index = 0;
		bool stop = false;
		bool waiting = false;
		State debug_state = STATE_STOPPED;
		StdString name;
		std::atomic<const char *> debug_running_task_name = { nullptr };

		void wait_to_finish_and_reset() {
			thread.wait_to_finish();
			pool = nullptr;
			index = 0;
			stop = false;
			waiting = false;
			debug_state = STATE_STOPPED;
			name.clear();
		}
	};

	static void thread_func_static(void *p_data);
	void thread_func(ThreadData &data);

	void create_thread(ThreadData &d, uint32_t i);
	void destroy_all_threads();

#ifdef VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS
	void debug_add_owned_task(IThreadedTask *task);
	void debug_remove_owned_task(IThreadedTask *task);
#endif

	FixedArray<ThreadData, MAX_THREADS> _threads;
	uint32_t _thread_count = 0;

	// 被调度的任务先放在这里。下一个空闲线程会将其移动到主等待队列。
	// 这是因为主等待队列可能因动态优先级排序而被锁定更久。
	StdVector<TaskItem> _staged_tasks;
	Mutex _staged_tasks_mutex;

	// 主等待列表。任务按优先级从中选取。任务在该列表中时优先级也可能改变，
	// 因此我们不能使用简单队列或在插入时排序。每个空闲线程都必须找到它，并可能更新
	// 它，偶尔为之。
	StdVector<TaskItem> _tasks;
	Mutex _tasks_mutex;
	Semaphore _tasks_semaphore;

	// 可能耗时超过一次迭代的进行中任务
	StdQueue<TaskItem> _spinning_tasks;
	Mutex _spinning_tasks_mutex;

	StdVector<IThreadedTask *> _completed_tasks;
	Mutex _completed_tasks_mutex;

	uint32_t _priority_update_period_ms = 32;
	uint64_t _last_priority_update_time_ms = 0;

	// 该布尔值同样由 `_tasks_mutex` 保护。
	// "被标记为"serial"的任务一次只能由一个线程执行。"
	bool _is_serial_task_running = false;

	StdString _name;

	unsigned int _debug_received_tasks = 0;
	unsigned int _debug_completed_tasks = 0;
	unsigned int _debug_taken_out_tasks = 0;

#ifdef VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS
	StdUnorderedMap<IThreadedTask *, StdString> _debug_owned_tasks;
	Mutex _debug_owned_tasks_mutex;
#endif
};

} // namespace voxel

#endif // VOXEL_THREADED_TASK_RUNNER_H
