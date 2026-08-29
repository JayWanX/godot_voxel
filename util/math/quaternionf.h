#ifndef VOXEL_MATH_QUATERNION_F_H
#define VOXEL_MATH_QUATERNION_F_H

#include "funcs.h"

namespace voxel {

// 32 位浮点精度四元数。
struct Quaternionf {
	union {
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		float components[4] = { 0, 0, 0, 1.0 };
	};

	inline Quaternionf() {}

	inline Quaternionf(float p_x, float p_y, float p_z, float p_w) : x(p_x), y(p_y), z(p_z), w(p_w) {}

	inline Quaternionf operator/(const Quaternionf &q) const {
		return Quaternionf(x / q.x, y / q.y, z / q.z, w / q.w);
	}

	inline Quaternionf operator/(float d) const {
		return Quaternionf(x / d, y / d, z / d, w / d);
	}
};

namespace math {

inline float length_squared(const Quaternionf &q) {
	return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

inline float length(const Quaternionf &q) {
	return Math::sqrt(length_squared(q));
}

inline Quaternionf normalized(const Quaternionf &q) {
	return q / length(q);
}

} // namespace math
} // namespace voxel

#endif // VOXEL_MATH_QUATERNION_F_H
