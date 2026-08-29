#ifndef PRIORITY_DEPENDENCY_H
#define PRIORITY_DEPENDENCY_H

#include "../util/containers/std_vector.h"
#include "../util/math/vector3f.h"
#include "../util/tasks/task_priority.h"
#include <atomic>
#include <memory>

namespace voxel {

// 用于计算位于特定位置的体素任务优先级的信息
struct PriorityDependency {
	struct ViewersData {
		// 这些位置由主线程写入，由数据块处理线程读取。
		// 顺序无关紧要。
		// 它仅用于调整任务优先级，因此使用锁并不划算。在最坏情况下，
		// 任务会远比预期更早或更晚执行，但无论如何它都会执行。
		// 实例创建后此向量不会被调整大小。它的大小刚好足够容纳所有观察者。
		StdVector<Vector3f> viewers;
		// 使用此计数而非 `viewers.size()`。它可能变化，但始终 <= `viewers.size()`
		std::atomic_uint32_t viewers_count;
		float highest_view_distance = 999999;
	};

	// TODO 如果观察者与本次会话中第一个地形的创建发生在同一时刻，那么该帧创建的加载任务
	// 会看到这个列表为空。原因在于该列表没有专门在那一帧更新，又因为 VoxelEngine 单例当时无法
	// 运行更新逻辑，因为它在主场景进行第一次处理之前无法注册到主循环，因为 Godot 没有暴露
	// 相应途径，因此我们不得不在地形被处理时创建一个根级节点作为变通，而这一操作无法在 enter_tree
	// 中更早完成，因为 Godot 禁止这么做。当然，另一种做法是在用户的工程设置中添加自动加载
	// （autoload），但当 Godot 的单例无需改动 ProjectSettings 就能开箱即用地正常更新时，这样做就太糟糕了。
	// 变通方案：
	// - 在添加观察者时同步该列表，而不只是在处理时
	// - 对超出范围的任务改用显式的取消令牌
	std::shared_ptr<ViewersData> shared;
	// 与观察者处于同一空间坐标系中的位置。
	// TODO 在队列中不会更新。这会有什么问题吗？
	Vector3f world_position;

	// 如果最近的观察者距离超过此距离，则该请求可被取消，因为不再值得处理
	// TODO 放弃这种方式，改用取消令牌，
	// 它并不总是可靠，而且需要处理恼人的“任务丢弃”情况
	float drop_distance_squared;

	TaskPriority evaluate(uint8_t lod_index, uint8_t band2_priority, float *out_closest_distance_sq);
};

} // namespace voxel

#endif // PRIORITY_DEPENDENCY_H
