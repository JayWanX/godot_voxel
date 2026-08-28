#ifndef VOXEL_GODOT_STRING_NAME_H
#define VOXEL_GODOT_STRING_NAME_H

#if defined(VOXEL_GODOT)
#include "../core/version.h"
#include <core/string/string_name.h>
#endif

namespace voxel::godot {
inline bool is_empty(const StringName &sn) {
#ifdef VOXEL_GODOT

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 3
	return sn == StringName();
#else
	return sn.is_empty();
#endif

#endif
}
} // namespace voxel::godot

#endif // VOXEL_GODOT_STRING_NAME_H
