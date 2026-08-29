#ifndef VOXEL_THREADED_TASK_GD
#define VOXEL_THREADED_TASK_GD

#include "../../godot/classes/ref_counted.h"
#include "../../godot/core/gdvirtual.h"
#include "../threaded_task.h"

namespace voxel {

class Voxel_ThreadedTaskInternal;

class Voxel_ThreadedTask : public RefCounted {
	GDCLASS(Voxel_ThreadedTask, RefCounted)
public:
	void run(int thread_index);
	int get_priority();
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
