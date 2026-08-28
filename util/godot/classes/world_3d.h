#ifndef VOXEL_GODOT_WORLD_3D_H
#define VOXEL_GODOT_WORLD_3D_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 2
#include <scene/resources/world_3d.h>
#else
#include <scene/resources/3d/world_3d.h>
#endif

#endif

#endif // VOXEL_GODOT_WORLD_3D_H
