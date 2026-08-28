#ifndef VOXEL_GODOT_RID_H
#define VOXEL_GODOT_RID_H

#if defined(VOXEL_GODOT)
#include <core/templates/rid.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/rid.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_RID_H
