#ifndef VOXEL_GODOT_TREE_H
#define VOXEL_GODOT_TREE_H

#if defined(VOXEL_GODOT)
#include <scene/gui/tree.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/tree.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_TREE_H
