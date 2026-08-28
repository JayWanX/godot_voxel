#ifndef VOXEL_GODOT_POINT_MESH_H
#define VOXEL_GODOT_POINT_MESH_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 2
#include <scene/resources/primitive_meshes.h>
#else
#include <scene/resources/3d/primitive_meshes.h>
#endif

#endif

#endif // VOXEL_GODOT_POINT_MESH_H
