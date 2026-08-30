#ifndef VOXEL_SAVE_COMPLETION_TRACKER_H
#define VOXEL_SAVE_COMPLETION_TRACKER_H

#include "../util/godot/classes/ref_counted.h"
#include "../util/tasks/async_dependency_tracker.h"
#include <memory>

namespace voxel {

// 异步保存函数的返回值，允许查询进度。
// 为 Godot 脚本 API 包装一个任务追踪器。如果将来其他地方需要，可能会变成通用的？
class VoxelSaveCompletionTracker : public RefCounted {
	GDCLASS(VoxelSaveCompletionTracker, RefCounted)
public:
	// 创建追踪器，包装一个异步任务依赖追踪器
	static Ref<VoxelSaveCompletionTracker> create(std::shared_ptr<AsyncDependencyTracker> tracker);

	// 所有任务是否已完成
	bool is_complete() const;
	// 是否已中止
	bool is_aborted() const;
	// 任务总数
	int get_total_tasks() const;
	// 剩余任务数
	int get_remaining_tasks() const;

private:
	static void _bind_methods();

	std::shared_ptr<AsyncDependencyTracker> _tracker;
	unsigned int _total_tasks = 0;
};

} // namespace voxel

#endif // VOXEL_SAVE_COMPLETION_TRACKER_H
