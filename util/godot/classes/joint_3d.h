#ifndef VOXEL_GODOT_JOINT_3D_H
#define VOXEL_GODOT_JOINT_3D_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 2
#include <scene/3d/joint_3d.h>
#else
#include <scene/3d/physics/joints/joint_3d.h>
#endif

#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/item_list.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_JOINT_3D_H
