#ifndef VOXEL_THREADED_TASK_GD
#define VOXEL_THREADED_TASK_GD

#include "../../godot/classes/ref_counted.h"
#include <core/object/gdvirtual.gen.h>
#include "../threaded_task.h"

namespace voxel {

class Voxel_ThreadedTaskInternal;

class Voxel_ThreadedTask : public RefCounted {
	GDCLASS(Voxel_ThreadedTask, RefCounted)
public:
	// 在指定线程上运行任务
	void run(int thread_index);
	// 获取任务优先级
	int get_priority();
	// 任务是否已取消
	bool is_cancelled();

	// 内部
	bool is_scheduled() const;
	void mark_completed();
	IThreadedTask *create_task();

private:
	GDVIRTUAL1(_run, int);
	GDVIRTUAL0R(int, _get_priority);
	GDVIRTUAL0R(bool, _is_cancelled);

	static void _bind_methods();

	// 在调度时创建，由任务运行器拥有
	Voxel_ThreadedTaskInternal *_scheduled_task = nullptr;
	bool _completed = false;
};

} // namespace voxel

#endif // VOXEL_THREADED_TASK_GD
