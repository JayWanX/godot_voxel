#ifndef VOXEL_GODOT_CONTROL_H
#define VOXEL_GODOT_CONTROL_H

#if defined(VOXEL_GODOT)
#include <scene/gui/control.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/control.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_CONTROL_H
