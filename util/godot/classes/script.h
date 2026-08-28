#ifndef VOXEL_GODOT_SCRIPT_H
#define VOXEL_GODOT_SCRIPT_H

#if defined(VOXEL_GODOT)
#include <core/object/script_language.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/script.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_SCRIPT_H
