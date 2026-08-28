#ifndef VOXEL_GODOT_PROJECT_SETTINGS_H
#define VOXEL_GODOT_PROJECT_SETTINGS_H

#if defined(VOXEL_GODOT)
#include <core/config/project_settings.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/project_settings.hpp>
using namespace godot;
#endif

namespace voxel::godot {

void add_custom_project_setting(
		Variant::Type type,
		const char *name,
		PropertyHint hint,
		const char *hint_string,
		Variant default_value,
		bool requires_restart
);

} // namespace voxel::godot

#endif // VOXEL_GODOT_PROJECT_SETTINGS_H
