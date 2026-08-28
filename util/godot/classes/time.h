#ifndef VOXEL_GODOT_TIME_H
#define VOXEL_GODOT_TIME_H

#if defined(VOXEL_GODOT)
#include <core/os/time.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/time.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_TIME_H
