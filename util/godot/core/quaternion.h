#ifndef VOXEL_GODOT_QUATERNION_H
#define VOXEL_GODOT_QUATERNION_H

#if defined(VOXEL_GODOT)
#include <core/math/quaternion.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/quaternion.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_QUATERNION_H
