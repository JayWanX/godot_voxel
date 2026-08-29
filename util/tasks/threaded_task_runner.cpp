#include "threaded_task_runner.h"
#include "../dstack.h"
#include "../godot/classes/time.h"
#include "../profiling.h"
#include "../string/format.h"

namespace voxel {

ThreadedTaskRunner::ThreadedTaskRunner() {}

ThreadedTaskRunner::~ThreadedTaskRunner() {
	destroy_all_threads();

	// 我们不拥有任务的所有权，因此在不处理它们的情况下销毁线程池是错误的
	if (_staged_tasks.size() != 0) {
		VOXEL_PRINT_ERROR("There are staged tasks remaining!");
	}
	if (_tasks.size() != 0) {
		VOXEL_PRINT_ERROR("There are tasks remaining!");
	}
	if (_spinning_tasks.size() != 0) {
		VOXEL_PRINT_ERROR("There are spinning tasks remaining!");
	}
	if (_completed_tasks.size() != 0) {
		VOXEL_PRINT_ERROR("There are completed tasks remaining!");
	}
}

void ThreadedTaskRunner::create_thread(ThreadData &d, uint32_t i) {
	d.pool = this;
	d.stop = false;
	d.waiting = false;
	d.index = i;
	if (!_name.empty()) {
		d.name = format("{} {}", _name, i);
	}
	d.thread.start(thread_func_static, &d);
}

void ThreadedTaskRunner::destroy_all_threads() {
	// 我们只有一个信号量用于通知线程恢复，且一次 `post()` 只允许一个线程通过。
	// 我们无法让某个特定线程停止，因为当发出 post 且其他线程在等待时，我们无法保证
	// 通过的会是我们想要的那个。
	// 因此我们只能在需要调节数量时选择停止所有线程，然后重新启动它们。
	// 此外，不应丢弃任务。线程正在处理的任何任务仍应正常完成。
	for (size_t i = 0; i < _thread_count; ++i) {
		ThreadData &d = _threads[i];
		d.stop = true;
	}
	for (size_t i = 0; i < _thread_count; ++i) {
		_tasks_semaphore.post();
	}
	for (size_t i = 0; i < _thread_count; ++i) {
		ThreadData &d = _threads[i];
		d.wait_to_finish_and_reset();
	}
}

#ifdef VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS

void ThreadedTaskRunner::debug_add_owned_task(IThreadedTask *task) {
	StdString s;
	dstack::Info info;
	info.to_string(s);
	println(format("Own {} t {}", uint64_t(task), Thread::get_caller_id()));
	MutexLock mlock(_debug_owned_tasks_mutex);
	auto p = _debug_owned_tasks.insert({ task, s });
	if (p.second == false) {
		flush_log_file();
		VOXEL_CRASH();
	}
}

void ThreadedTaskRunner::debug_remove_owned_task(IThreadedTask *task) {
	{
		MutexLock mlock(_debug_owned_tasks_mutex);
		VOXEL_ASSERT(_debug_owned_tasks.erase(task) == 1);
	}
	println(format("Unowned {}", uint64_t(task)));
}

#endif // VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS

void ThreadedTaskRunner::set_name(const char *name) {
	_name = name;
}

void ThreadedTaskRunner::set_thread_count(uint32_t count) {
	if (count > MAX_THREADS) {
		count = MAX_THREADS;
	}
	destroy_all_threads();
	_thread_count = count;
	for (uint32_t i = 0; i < _thread_count; ++i) {
		ThreadData &d = _threads[i];
		create_thread(d, i);
	}
}

void ThreadedTaskRunner::set_priority_update_period(uint32_t milliseconds) {
	_priority_update_period_ms = milliseconds;
}

void ThreadedTaskRunner::enqueue(IThreadedTask *task, bool serial) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(task != nullptr);
	TaskItem t;
	t.task = task;
	t.is_serial = serial;
	{
		MutexLock lock(_staged_tasks_mutex);
		_staged_tasks.push_back(t);
		++_debug_received_tasks;

#ifdef VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS
		debug_add_owned_task(task);
#endif
	}
	// TODO 我是否需要 post 一定次数？
	// 我感觉这会在任务变空时导致信号量被过多地通过
	_tasks_semaphore.post();
}

void ThreadedTaskRunner::enqueue(Span<IThreadedTask *> new_tasks, bool serial) {
#ifdef DEBUG_ENABLED
	for (size_t i = 0; i < new_tasks.size(); ++i) {
		VOXEL_ASSERT(new_tasks[i] != nullptr);
	}
#endif
	{
		MutexLock lock(_staged_tasks_mutex);
		const size_t dst_begin = _staged_tasks.size();
		_staged_tasks.resize(_staged_tasks.size() + new_tasks.size());
		for (size_t i = 0; i < new_tasks.size(); ++i) {
			IThreadedTask *new_task = new_tasks[i];
			TaskItem t;
			t.task = new_task;
			t.is_serial = serial;
			_staged_tasks[dst_begin + i] = t;

#ifdef VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS
			debug_add_owned_task(new_task);
#endif
		}
		_debug_received_tasks += new_tasks.size();
	}
	// TODO 我是否需要 post 一定次数？
	// 它应该是线程数而非任务数吗？
	for (size_t i = 0; i < new_tasks.size(); ++i) {
		_tasks_semaphore.post();
	}
}

void ThreadedTaskRunner::thread_func_static(void *p_data) {
	ThreadData &data = *static_cast<ThreadData *>(p_data);
	ThreadedTaskRunner &pool = *data.pool;

	if (!data.name.empty()) {
		Thread::set_name(data.name.c_str());

#ifdef VOXEL_PROFILER_ENABLED
		VOXEL_PROFILE_SET_THREAD_NAME(data.name.c_str());
#endif
	}

	pool.thread_func(data);
}

void ThreadedTaskRunner::thread_func(ThreadData &data) {
	data.debug_state = STATE_RUNNING;

	StdVector<TaskItem> tasks;
	StdVector<TaskItem> postponed_tasks;
	StdVector<IThreadedTask *> cancelled_tasks;

	while (!data.stop) {
		bool is_running_serial_task = false;
		bool task_queue_was_empty = false;
		{
			VOXEL_PROFILE_SCOPE_NAMED("Task pickup");

			data.debug_state = STATE_PICKING;

			VOXEL_ASSERT(tasks.size() == 0);

			// 若有，则选取一个被推迟的任务。
			// 我们仍然会从主优先队列运行一个任务，这样被推迟的任务不会
			// 独占执行。
			//
			// TODO 如果一个大任务正锁住被推迟任务需要访问的资源，而它们仍残留，会怎样？
			// 那些被推迟的任务会某种程度地自旋等待而不睡眠。这有问题吗？
			{
				MutexLock lock2(_spinning_tasks_mutex);
				if (_spinning_tasks.size() > 0) {
					tasks.push_back(_spinning_tasks.front());
					_spinning_tasks.pop();
				}
			}

			{
				// TODO 当任务非常短且数量很多时，一个线程可能独占该互斥量。
				//
				MutexLock lock(_tasks_mutex);

				// 从暂存队列移动任务。
				// 加锁时尽量减小阻塞主线程的风险，加锁时间应当非常短。
				if (_staged_tasks_mutex.try_lock()) {
					append_array(_tasks, _staged_tasks);
					_staged_tasks.clear();
					_staged_tasks_mutex.unlock();
				}

				// 从优先队列中选取最佳任务
				if (_tasks.size() != 0) {
					// 定期排序。
					// 在任务插入后继续排序，是为了应对存在大量待处理
					// 任务、且处理它们可能需要数秒的情况。玩家可能移动很快，而
					// 优先级位置可能改变。某些任务甚至在运行前就变得无关紧要，因此
					// 我们可能将它们从列表中移除，以免拖慢处理。
					const uint64_t now = Time::get_singleton()->get_ticks_msec();
					if (now - _last_priority_update_time_ms > _priority_update_period_ms) {
						VOXEL_PROFILE_SCOPE_NAMED("Sorting");

						{
							VOXEL_PROFILE_SCOPE_NAMED("Update priorities");
							for (unsigned int i = 0; i < _tasks.size();) {
								TaskItem &item = _tasks[i];
								item.cached_priority = item.task->get_priority();

								if (item.task->is_cancelled()) {
									cancelled_tasks.push_back(item.task);
									_tasks[i] = _tasks.back();
									_tasks.pop_back();
									continue;
								}

								++i;
							}
						}

						struct TaskComparator {
							inline bool operator()(const TaskItem &a, const TaskItem &b) const {
								// 优先级最高的任务放在最后（便于从尾部弹出）
								return a.cached_priority < b.cached_priority;
							}
						};
						SortArray<TaskItem, TaskComparator> sorter;
						sorter.sort(_tasks.data(), _tasks.size());

						_last_priority_update_time_ms = Time::get_singleton()->get_ticks_msec();
					}

					// 尽可能选取优先级最高的任务
					// for (int i = int(_tasks.size()) - 1; i >= 0; --i) {
					for (unsigned int i = _tasks.size(); i-- > 0;) {
						const TaskItem item = _tasks[i];
						// 在这方面串行任务有点麻烦……
						// 我们可以让保存/加载任务接受多于一项的工作，这是处理
						// 串行工作的最佳方式，但在某些情况下难以提前知晓……
						if (item.is_serial && _is_serial_task_running) {
							// 尝试上一个任务
							continue;
						}

						tasks.push_back(item);
						// 由于串行任务的处理，我们不只是弹出最后一个元素。但有序移除
						// 应该足够快，因为串行任务并不常见。
						_tasks.erase(_tasks.begin() + i);
						break;
					}

				} // 对于要选取的每个任务

				// 如果我们选取了串行任务，必须将共享布尔值设为 `true`。
				// 当前线程选取的任务列表中可能包含多个串行任务，
				// 因此我们在全部选取完后再更新该布尔值。
				// 这必须是唯一能将其设为 `true` 的地方，且由互斥量保护。
				if (_is_serial_task_running == false) { // 仅是一种优化，并不真正提供线程安全
					for (unsigned int i = 0; i < tasks.size(); ++i) {
						if (tasks[i].is_serial) {
							// 写入成员变量，以便所有线程都能检查
							_is_serial_task_running = true;
							// 写入线程局部变量，以便我们知道这是当前线程
							is_running_serial_task = true;
							break;
						}
					}
				}

				task_queue_was_empty = _tasks.size() == 0;

			} // 任务队列互斥量加锁
		}

		if (cancelled_tasks.size() > 0) {
			MutexLock lock(_completed_tasks_mutex);
			const size_t count = cancelled_tasks.size();
			append_array(_completed_tasks, cancelled_tasks);
			_debug_completed_tasks += count;
			cancelled_tasks.clear();
		}

		// print_line(String("Processing {0} tasks").format(varray(tasks.size())));

		if (tasks.empty()) {
			if (task_queue_was_empty) {
				// 任务队列为空，将等待直到有更多任务被提交。
				// 若在我们上次检查队列与此刻之间有任务被提交，
				// 信号量会有一个计数可供递减，我们就不会停在这里。

				data.debug_state = STATE_WAITING;

				// 等待更多任务
				data.waiting = true;
				_tasks_semaphore.wait();
				data.waiting = false;

			} else {
				// 当前线程不被允许选取队列中现有的任务（它们可能是串行的，且
				// 已有任务在运行）。因此我们将等待极短时间后重试。
				// （替代方案是在每个串行任务后 post 信号量？）
				Thread::sleep_usec(1000);
			}

		} else {
			data.debug_state = STATE_RUNNING;

			// 运行每个任务
			for (size_t i = 0; i < tasks.size(); ++i) {
				TaskItem &item = tasks[i];

				if (!item.task->is_cancelled()) {
					ThreadedTaskContext ctx(data.index, item.cached_priority);
					data.debug_running_task_name = item.task->get_debug_name();
					item.task->run(ctx);
#ifdef VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS
					if (ctx.status == ThreadedTaskContext::STATUS_TAKEN_OUT) {
						debug_remove_owned_task(item.task);
					}
#endif
					item.status = ctx.status;
					data.debug_running_task_name = nullptr;

					/*
					if (ctx.next_immediate_task != nullptr) {
						TaskItem next;
						next.task = ctx.next_immediate_task;
#ifdef VOXEL_THREADED_TASK_RUNNER_CHECK_DUPLICATE_TASKS
						debug_add_owned_task(next.task);
#endif
						tasks.push_back(next);
					}
					*/
				}
			}

			// 若当前线程刚刚运行过串行任务
			if (is_running_serial_task) {
				VOXEL_ASSERT(_is_serial_task_running);
				// 将布尔值重置，以便任何线程现在都能选取串行任务。
				// 这是唯一将其设为 `false` 的地方，且发生该情况时它必定已是 `true`，
				// 因此无需加锁互斥量。
				_is_serial_task_running = false;
			}

			{
				MutexLock lock(_completed_tasks_mutex);
				for (size_t i = 0; i < tasks.size(); ++i) {
					const TaskItem &item = tasks[i];
					switch (item.status) {
						case ThreadedTaskContext::STATUS_COMPLETE:
							_completed_tasks.push_back(item.task);
							++_debug_completed_tasks;
							break;

						case ThreadedTaskContext::STATUS_POSTPONED:
							postponed_tasks.push_back(item);
							break;

						case ThreadedTaskContext::STATUS_TAKEN_OUT:
							// 丢弃任务指针，其所有权可能已传递给另一个任务
							++_debug_taken_out_tasks;
							break;

						default:
							VOXEL_PRINT_ERROR("Unknown task status");
							break;
					}
				}
			}

			tasks.clear();

			{
				MutexLock lock(_spinning_tasks_mutex);
				for (const TaskItem &item : postponed_tasks) {
					_spinning_tasks.push(item);
				}
			}

			postponed_tasks.clear();
		}
	}

	data.debug_state = STATE_STOPPED;
}

void ThreadedTaskRunner::wait_for_all_tasks() {
	const uint32_t suspicious_delay_msec = 10'000;

	uint32_t before = Time::get_singleton()->get_ticks_msec();
	bool error1_reported = false;

	// 等待直到所有任务都被取走
	while (true) {
		// TODO 这并不十分精确，因为正在运行的任务可能调度更多任务。不确定是否需要它？
		// 等待所有线程进入等待状态是更确定的解决方案。
		bool any_staged_tasks = false;
		{
			MutexLock lock3(_staged_tasks_mutex);
			any_staged_tasks = _staged_tasks.size() > 0;
		}
		if (!any_staged_tasks) {
			MutexLock lock(_tasks_mutex);
			if (_tasks.size() == 0) {
				MutexLock lock2(_spinning_tasks_mutex);
				if (_spinning_tasks.size() == 0) {
					break;
				}
			}
		}

		Thread::sleep_usec(2000);

		if (!error1_reported && Time::get_singleton()->get_ticks_msec() - before > suspicious_delay_msec) {
			VOXEL_PRINT_WARNING("Waiting for all tasks to be picked is taking a long time");
			error1_reported = true;
		}
	}

	before = Time::get_singleton()->get_ticks_msec();
	bool error2_reported = false;

	// 等待直到所有线程都完成了它们的全部任务
	bool any_working_thread = true;
	while (any_working_thread) {
		any_working_thread = false;
		for (size_t i = 0; i < _thread_count; ++i) {
			const ThreadData &t = _threads[i];
			if (t.waiting == false) {
				any_working_thread = true;
				break;
			}
		}

		Thread::sleep_usec(2000);

		if (!error2_reported && Time::get_singleton()->get_ticks_msec() - before > suspicious_delay_msec) {
			VOXEL_PRINT_WARNING("Waiting for all tasks to be completed is taking a long time");
			error2_reported = true;
		}
	}
}

// 调试信息在极少数情况下可能不正确。
// 这些变量应当被安全更新，但计算或读取它们并非线程安全。
// 认为为了调试而加锁并不值得。

ThreadedTaskRunner::State ThreadedTaskRunner::get_thread_debug_state(uint32_t i) const {
	return _threads[i].debug_state;
}

const char *ThreadedTaskRunner::get_thread_debug_task_name(unsigned int thread_index) const {
	return _threads[thread_index].debug_running_task_name;
}

unsigned int ThreadedTaskRunner::get_debug_remaining_tasks() const {
	return _debug_received_tasks - _debug_completed_tasks - _debug_taken_out_tasks;
}

StdVector<IThreadedTask *> &ThreadedTaskRunner::get_completed_tasks_temp_tls() {
	static thread_local StdVector<IThreadedTask *> tls_temp;
	return tls_temp;
}

} // namespace voxel
