#ifndef VOXEL_GODOT_VIEWPORT_H
#define VOXEL_GODOT_VIEWPORT_H

#if defined(VOXEL_GODOT)
#include <scene/main/viewport.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/viewport.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_VIEWPORT_H
