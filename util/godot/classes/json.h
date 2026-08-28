#ifndef VOXEL_GODOT_JSON_H
#define VOXEL_GODOT_JSON_H

#if defined(VOXEL_GODOT)
#include <core/io/json.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/json.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_JSON_H
