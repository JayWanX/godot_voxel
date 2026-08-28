#include "threaded_task_gd.h"

#ifdef VOXEL_GODOT
#include "../../godot/core/class_db.h"
#endif

namespace voxel {

// Using a decoupled pattern so we can do a few more safety checks for scripters
class Voxel_ThreadedTaskInternal : public IThreadedTask {
public:
	Ref<Voxel_ThreadedTask> ref;

	const char *get_debug_name() const override {
		return "Voxel_ThreadedTaskInternal";
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

void Voxel_ThreadedTask::run(int thread_index) {
	GDVIRTUAL_CALL(_run, thread_index);
}

int Voxel_ThreadedTask::get_priority() {
	int priority = 0;
	if (GDVIRTUAL_CALL(_get_priority, priority)) {
		return priority;
	}
	return 0;
}

bool Voxel_ThreadedTask::is_cancelled() {
	bool cancelled = false;
	if (GDVIRTUAL_CALL(_is_cancelled, cancelled)) {
		return cancelled;
	}
	return false;
}

bool Voxel_ThreadedTask::is_scheduled() const {
	return _scheduled_task != nullptr;
}

void Voxel_ThreadedTask::mark_completed() {
	_scheduled_task = nullptr;
	emit_signal("completed");
}

IThreadedTask *Voxel_ThreadedTask::create_task() {
	CRASH_COND(_scheduled_task != nullptr);
	_scheduled_task = memnew(Voxel_ThreadedTaskInternal);
	_scheduled_task->ref.reference_ptr(this);
	return _scheduled_task;
}

void Voxel_ThreadedTask::_bind_methods() {
	ADD_SIGNAL(MethodInfo("completed"));

	GDVIRTUAL_BIND(_run, "thread_index");
	GDVIRTUAL_BIND(_get_priority);
	GDVIRTUAL_BIND(_is_cancelled);
}

} // namespace voxel
