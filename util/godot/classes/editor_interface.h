#ifndef VOXEL_GODOT_EDITOR_INTERFACE_H
#define VOXEL_GODOT_EDITOR_INTERFACE_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR == 0
#include <editor/editor_plugin.h>
#else
#include <editor/editor_interface.h>
#endif

#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/editor_interface.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_EDITOR_INTERFACE_H
