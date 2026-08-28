#ifndef VOXEL_VECTOR4F_H
#define VOXEL_VECTOR4F_H

#include "vector4t.h"

namespace voxel {
typedef Vector4T<float> Vector4f;
}

namespace voxel::math {

inline float length_squared(const Vector4f &v) {
	return v.x * v.x + v.y * v.y + v.z * v.z + v.w + v.w;
}

inline Vector4f normalized(const Vector4f &v) {
	const float lengthsq = length_squared(v);
	if (lengthsq == 0) {
		return Vector4f();
	} else {
		const float length = 1.f / Math::sqrt(lengthsq);
		return v * length;
	}
}

} // namespace voxel::math

#endif // VOXEL_VECTOR4F_H
