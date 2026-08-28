#ifndef VOXEL_GODOT_LABEL_H
#define VOXEL_GODOT_LABEL_H

#if defined(VOXEL_GODOT)
#include <scene/gui/label.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/label.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_LABEL
