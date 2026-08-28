#ifndef VOXEL_GODOT_TRANSFORM_3D_H
#define VOXEL_GODOT_TRANSFORM_3D_H

#if defined(VOXEL_GODOT)
#include <core/math/transform_3d.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/transform3d.hpp>
using namespace godot;
#endif

namespace voxel {

inline Vector3 get_forward(const Transform3D &t) {
	return -t.basis.get_column(Vector3::AXIS_Z);
}

} // namespace voxel

#endif // VOXEL_GODOT_TRANSFORM_3D_H
