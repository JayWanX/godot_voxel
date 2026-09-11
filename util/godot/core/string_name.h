#include <core/version.h>
#ifndef VOXEL_GODOT_STRING_NAME_H
#define VOXEL_GODOT_STRING_NAME_H

#include <core/version.h>
#include <core/string/string_name.h>

namespace voxel::godot {
inline bool is_empty(const StringName &sn) {

	return sn.is_empty();

}
} // namespace voxel::godot

#endif // VOXEL_GODOT_STRING_NAME_H
