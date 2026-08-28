#ifndef VOXEL_THREADED_TASK_GD
#define VOXEL_THREADED_TASK_GD

#include "../../godot/classes/ref_counted.h"
#include "../../godot/core/gdvirtual.h"
#include "../threaded_task.h"

namespace voxel {

class VOXEL_ThreadedTaskInternal;

class VOXEL_ThreadedTask : public RefCounted {
	GDCLASS(VOXEL_ThreadedTask, RefCounted)
public:
	void run(int thread_index);
	int get_priority();
	bool is_cancelled();

	// Internal
	bool is_scheduled() const;
	void mark_completed();
	IThreadedTask *create_task();

private:
	GDVIRTUAL1(_run, int);
	GDVIRTUAL0R(int, _get_priority);
	GDVIRTUAL0R(bool, _is_cancelled);

	static void _bind_methods();

	// Created upon scheduling, owned by the task runner
	VOXEL_ThreadedTaskInternal *_scheduled_task = nullptr;
	bool _completed = false;
};

} // namespace voxel

#endif // VOXEL_THREADED_TASK_GD
