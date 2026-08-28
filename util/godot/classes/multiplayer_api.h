#ifndef VOXEL_GODOT_MULTIPLAYER_API_H
#define VOXEL_GODOT_MULTIPLAYER_API_H

#if defined(VOXEL_GODOT)
#include <scene/main/multiplayer_api.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/multiplayer_api.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_MULTIPLAYER_API_H
