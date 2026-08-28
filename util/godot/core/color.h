#ifndef VOXEL_COLOR_H
#define VOXEL_COLOR_H

#if defined(VOXEL_GODOT)
#include <core/math/color.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/color.hpp>
using namespace godot;
#endif

#endif // VOXEL_COLOR_H
