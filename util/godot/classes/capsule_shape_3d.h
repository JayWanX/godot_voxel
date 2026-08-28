#ifndef VOXEL_GODOT_CAPSULE_SHAPE_3D_H
#define VOXEL_GODOT_CAPSULE_SHAPE_3D_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 2
#include <scene/resources/capsule_shape_3d.h>
#else
#include <scene/resources/3d/capsule_shape_3d.h>
#endif

#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/capsule_shape_3d.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_CAPSULE_SHAPE_3D_H