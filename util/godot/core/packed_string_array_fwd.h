#ifndef VOXEL_GODOT_PACKED_STRING_ARRAY_FWD_H
#define VOXEL_GODOT_PACKED_STRING_ARRAY_FWD_H

#if defined(VOXEL_GODOT)
class String;

template <typename T>
class Vector;
typedef Vector<String> PackedStringArray;

#elif defined(VOXEL_GODOT_EXTENSION)
#include "../macros.h"
VOXEL_GODOT_FORWARD_DECLARE(class PackedStringArray);

#endif

#endif // VOXEL_GODOT_PACKED_STRING_ARRAY_FWD_H
