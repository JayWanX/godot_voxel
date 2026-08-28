#ifndef VOXEL_GODOT_NOISE_H
#define VOXEL_GODOT_NOISE_H

#if defined(VOXEL_GODOT)
#include <modules/noise/noise.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/noise.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_NOISE_H
