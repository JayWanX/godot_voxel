#include "progressive_task_runner.h"
#include "../errors.h"
#include <core/os/time.h>
#include "../math/funcs.h"
#include "../memory/memory.h"

namespace voxel {

ProgressiveTaskRunner::~ProgressiveTaskRunner() {
	flush();
	VOXEL_ASSERT_RETURN_MSG(_tasks.size() == 0, "Tasks got created in destructors?");
}

void ProgressiveTaskRunner::push(IProgressiveTask *task) {
	VOXEL_ASSERT_RETURN(task != nullptr);
	_tasks.push(task);
}

void ProgressiveTaskRunner::process() {
	const int64_t now_msec = Time::get_singleton()->get_ticks_msec();
	const int64_t delta_msec = now_msec - _last_process_time_msec;
	_last_process_time_msec = now_msec;
	VOXEL_ASSERT_RETURN(delta_msec >= 0);

	// 目标是在 S 秒内将所有任务出队。
	// 因此若我们有 N 个任务，且每秒调用 `process` F 次，则每次必须出队 N / (S * F) 个任务。
	// 换句话说，若每 D 秒调用一次 `process`，则必须出队 (D * N) / S 个任务。
	// 我们确保至少运行一定数量，使其不会卡在 0。
	// 随着待处理任务数量减少，我们希望保持运行我们计算出的最大数量。
	// 完成后我们将其重置。

	_dequeue_count = math::max(int64_t(_dequeue_count), (int64_t(_tasks.size()) * delta_msec) / COMPLETION_TIME_MSEC);
	_dequeue_count = math::min(_dequeue_count, math::max(MIN_COUNT, static_cast<unsigned int>(_tasks.size())));

	unsigned int count = _dequeue_count;
	while (_tasks.size() > 0 && count > 0) {
		IProgressiveTask *task = _tasks.front();
		_tasks.pop();
		task->run();
		// TODO 改为调用回收函数？
		VOXEL_DELETE(task);
		--count;
	}
}

void ProgressiveTaskRunner::flush() {
	while (!_tasks.empty()) {
		IProgressiveTask *task = _tasks.front();
		_tasks.pop();
		task->run();
		VOXEL_DELETE(task);
	}
}

unsigned int ProgressiveTaskRunner::get_pending_count() const {
	return _tasks.size();
}

} // namespace voxel
