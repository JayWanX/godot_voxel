#ifndef VOXEL_GODOT_COLLISION_SHAPE_3D_H
#define VOXEL_GODOT_COLLISION_SHAPE_3D_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 2
#include <scene/3d/collision_shape_3d.h>
#else
#include <scene/3d/physics/collision_shape_3d.h>
#endif

#endif

#endif // VOXEL_GODOT_COLLISION_SHAPE_3D_H
