#ifndef VOXEL_GODOT_VECTOR3_H
#define VOXEL_GODOT_VECTOR3_H

#if defined(VOXEL_GODOT)
#include <core/math/vector3.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/vector3.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_VECTOR3_H
