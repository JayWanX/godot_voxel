#ifndef VOXEL_GODOT_RIGID_BODY_3D_H
#define VOXEL_GODOT_RIGID_BODY_3D_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 2
#include <scene/3d/physics_body_3d.h>
#else
#include <scene/3d/physics/rigid_body_3d.h>
#endif

#endif

#endif // VOXEL_GODOT_RIGID_BODY_3D_H
