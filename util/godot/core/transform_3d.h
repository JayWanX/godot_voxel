#ifndef VOXEL_GODOT_TRANSFORM_3D_H
#define VOXEL_GODOT_TRANSFORM_3D_H

#include <core/math/transform_3d.h>

namespace voxel {

inline Vector3 get_forward(const Transform3D &t) {
	return -t.basis.get_column(Vector3::AXIS_Z);
}

} // namespace voxel

#endif // VOXEL_GODOT_TRANSFORM_3D_H
