#include <core/version.h>
#ifndef VOXEL_GODOT_CURVE_H
#define VOXEL_GODOT_CURVE_H

#include <scene/resources/curve.h>

#include "../../math/interval.h"
#include <core/version.h>

namespace voxel::godot {

inline math::Interval get_curve_domain(const Curve &curve) {
	return math::Interval(curve.get_min_domain(), curve.get_max_domain());
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_CURVE_H
