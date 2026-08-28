#ifndef VOXEL_GODOT_BOX_SHAPE_3D_H
#define VOXEL_GODOT_BOX_SHAPE_3D_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 2
#include <scene/resources/box_shape_3d.h>
#else
#include <scene/resources/3d/box_shape_3d.h>
#endif

#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/box_shape3d.h>
using namespace godot;
#endif

#endif // VOXEL_GODOT_BOX_SHAPE_3D_H
