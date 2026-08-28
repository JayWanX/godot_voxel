#ifndef VOXEL_GODOT_BASIS_H
#define VOXEL_GODOT_BASIS_H

#if defined(VOXEL_GODOT)
#include <core/math/basis.h>
#endif

namespace voxel::godot::BasisUtility {

inline Vector3 get_forward(const Basis &basis) {
	return -basis.get_column(Vector3::AXIS_Z);
}

inline Vector3 get_up(const Basis &basis) {
	return basis.get_column(Vector3::AXIS_Y);
}

} // namespace voxel::godot::BasisUtility

#endif // VOXEL_GODOT_BASIS_H
