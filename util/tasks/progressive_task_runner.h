#ifndef VOXEL_PROGRESSIVE_TASK_RUNNER_H
#define VOXEL_PROGRESSIVE_TASK_RUNNER_H

#include "../containers/std_queue.h"
#include <cstdint>

namespace voxel {

// TODO 如果 Godot4 的 Vulkan 缓冲区释放能优化得更好就太好了。
// 这最初是为了规避 Godot4 中极其缓慢的 Vulkan 缓冲区释放。
// 它在主线程上发生，当地形包含大量区块且
// 摄像机移动较快时，会导致延迟卡顿。
// 我讨厌这个变通方案，因为感觉我们几乎无法掌控稳定的帧率。
// “减少网格数量”并不足够，如果无法动态处理的话。

class IProgressiveTask {
public:
	virtual ~IProgressiveTask() {}
	virtual void run() = 0;
};

// 每帧运行一定数量的任务，使得所有任务应在 N 秒内完成。
// 这能将负载随时间分摊，并趋于平滑 CPU 峰值。
// 当任务的直接耗时无法作为代价指标时，可用它替代时间切片运行器。
// 这类任务会将工作负载委托给另一个稍后运行的、不可直达的系统（说的就是你，
// Godot）。尽管它远非完美，且是在优化与多线程都不可行时的
// 最后手段。这类任务最好不要要求低延迟，因为它们很可能会稍晚运行
// 于时间切片任务。
class ProgressiveTaskRunner {
public:
	~ProgressiveTaskRunner();

	void push(IProgressiveTask *task);
	void process();
	void flush();
	unsigned int get_pending_count() const;

private:
	static const unsigned int MIN_COUNT = 4;
	static const unsigned int COMPLETION_TIME_MSEC = 500;

	StdQueue<IProgressiveTask *> _tasks;
	unsigned int _dequeue_count = MIN_COUNT;
	int64_t _last_process_time_msec = 0;
};

} // namespace voxel

#endif // VOXEL_PROGRESSIVE_TASK_RUNNER_H
