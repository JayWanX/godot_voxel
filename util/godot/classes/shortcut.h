#ifndef VOXEL_GODOT_SHORTCUT_H
#define VOXEL_GODOT_SHORTCUT_H

#if defined(VOXEL_GODOT)
#include <core/input/shortcut.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/shortcut.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_SHORTCUT_H
