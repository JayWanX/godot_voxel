#ifndef VOXEL_GODOT_PRINT_STRING_H
#define VOXEL_GODOT_PRINT_STRING_H

// Access to `print_line` the same as defined in core

#if defined(VOXEL_GODOT)
#include <core/string/print_string.h>

#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/utility_functions.hpp>

inline void print_line(const godot::Variant &v) {
	godot::UtilityFunctions::print(v);
}

#endif

#endif // VOXEL_GODOT_PRINT_STRING_H
