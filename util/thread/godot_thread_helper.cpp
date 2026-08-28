#include "godot_thread_helper.h"
#include <godot_cpp/core/class_db.hpp>

namespace voxel {

void VOXEL_GodotThreadHelper::run() {
	VOXEL_ASSERT_RETURN(_callback != nullptr);
	_callback(_callback_data);
}

void VOXEL_GodotThreadHelper::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("run"), &VOXEL_GodotThreadHelper::run);
}

} // namespace voxel
