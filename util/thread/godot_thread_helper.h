#ifndef VOXEL_GODOT_THREAD_HELPER_H
#define VOXEL_GODOT_THREAD_HELPER_H

#ifndef VOXEL_GODOT_EXTENSION
#error "This class is exclusive to Godot Extension"
#endif

#include "../errors.h"
#include "../thread/thread.h"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace voxel {

// Proxy-object to run a C-style callback using GDExtension threads.
// This class isn't intented to be exposed.
//
// It is recommended to use Godot threads instead of vanilla std::thread,
// because Godot sets up additional stuff in `Thread` (like script debugging and platform-specific stuff to set
// priority). To use Godot threads in GDExtension, you are FORCED to send an object method as callback. And to do that,
// the object must be registered.
class VOXEL_GodotThreadHelper : public ::godot::Object {
	GDCLASS(VOXEL_GodotThreadHelper, ::godot::Object)
public:
	VOXEL_GodotThreadHelper() {}

	void set_callback(Thread::Callback callback, void *data) {
		_callback = callback;
		_callback_data = data;
	}

private:
	void run();

	static void _bind_methods();

	Thread::Callback _callback = nullptr;
	void *_callback_data = nullptr;
};

} // namespace voxel

#endif // VOXEL_GODOT_THREAD_HELPER_H
