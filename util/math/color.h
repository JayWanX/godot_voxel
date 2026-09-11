#ifndef VOXEL_MATH_COLOR_H
#define VOXEL_MATH_COLOR_H

#include <core/math/color.h>

namespace voxel::math {

inline Color lerp(const Color a, const Color b, float t) {
	return Color(Math::lerp(a.r, b.r, t), Math::lerp(a.g, b.g, t), Math::lerp(a.b, b.b, t), Math::lerp(a.a, b.a, t));
}

} // namespace voxel::math

#endif // VOXEL_MATH_COLOR_H
