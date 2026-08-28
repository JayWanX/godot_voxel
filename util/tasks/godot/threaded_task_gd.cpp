#include "threaded_task_gd.h"

#ifdef VOXEL_GODOT
#include "../../godot/core/class_db.h"
#endif

namespace voxel {

// Using a decoupled pattern so we can do a few more safety checks for scripters
class VOXEL_ThreadedTaskInternal : public IThreadedTask {
public:
	Ref<VOXEL_ThreadedTask> ref;

	const char *get_debug_name() const override {
		return "VOXEL_ThreadedTaskInternal";
	}

	void run(ThreadedTaskContext &ctx) override {
		ref->run(ctx.thread_index);
	}

	void apply_result() override {
		// Not exposed. Scripters may prefer to use a `completed` signal instead.
		ref->mark_completed();
	}

	TaskPriority get_priority() override {
		TaskPriority priority;
		priority.whole = ref->get_priority();
		return priority;
	}

	bool is_cancelled() override {
		return ref->is_cancelled();
	}
};

void VOXEL_ThreadedTask::run(int thread_index) {
	GDVIRTUAL_CALL(_run, thread_index);
}

int VOXEL_ThreadedTask::get_priority() {
	int priority = 0;
	if (GDVIRTUAL_CALL(_get_priority, priority)) {
		return priority;
	}
	return 0;
}

bool VOXEL_ThreadedTask::is_cancelled() {
	bool cancelled = false;
	if (GDVIRTUAL_CALL(_is_cancelled, cancelled)) {
		return cancelled;
	}
	return false;
}

bool VOXEL_ThreadedTask::is_scheduled() const {
	return _scheduled_task != nullptr;
}

void VOXEL_ThreadedTask::mark_completed() {
	_scheduled_task = nullptr;
	emit_signal("completed");
}

IThreadedTask *VOXEL_ThreadedTask::create_task() {
	CRASH_COND(_scheduled_task != nullptr);
	_scheduled_task = memnew(VOXEL_ThreadedTaskInternal);
	_scheduled_task->ref.reference_ptr(this);
	return _scheduled_task;
}

void VOXEL_ThreadedTask::_bind_methods() {
	ADD_SIGNAL(MethodInfo("completed"));

	GDVIRTUAL_BIND(_run, "thread_index");
	GDVIRTUAL_BIND(_get_priority);
	GDVIRTUAL_BIND(_is_cancelled);
}

} // namespace voxel
