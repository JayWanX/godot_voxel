#ifndef VOXEL_GODOT_WINDOW_H
#define VOXEL_GODOT_WINDOW_H

#if defined(VOXEL_GODOT)
#include <scene/main/window.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/window.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_WINDOW_H
