#ifndef VOXEL_GODOT_TIMER_H
#define VOXEL_GODOT_TIMER_H

#if defined(VOXEL_GODOT)
#include <scene/main/timer.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/timer.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_TIMER_H
