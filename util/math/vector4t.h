#ifndef VOXEL_VECTOR4T_H
#define VOXEL_VECTOR4T_H

#include "../errors.h"
#include "funcs.h"

namespace voxel {

// 模板四维向量。仅包含字段与标准运算符。
// 数学函数独立出来，以便实现更统一的重载，并与其他数学库（如
// 着色器）保持一致。
template <typename T>
struct Vector4T {
	static const unsigned int AXIS_COUNT = 4;

	union {
		struct {
			T x;
			T y;
			T z;
			T w;
		};
		T coords[4];
	};

	Vector4T() : x(0), y(0), z(0), w(0) {}

	// 建议使用 `explicit`，否则会引入大量隐式转换，
	// 导致许多情况产生二义性。
	explicit Vector4T(T p_v) : x(p_v), y(p_v), z(p_v), w(p_v) {}

	Vector4T(T p_x, T p_y, T p_z, T p_w) : x(p_x), y(p_y), z(p_z), w(p_w) {}

	inline const T &operator[](const unsigned int p_axis) const {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(p_axis < AXIS_COUNT);
#endif
		return coords[p_axis];
	}

	inline T &operator[](const unsigned int p_axis) {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(p_axis < AXIS_COUNT);
#endif
		return coords[p_axis];
	}

	inline Vector4T operator+(const Vector4T &p_v) const {
		return Vector4T( //
				x + p_v.x, //
				y + p_v.y, //
				z + p_v.z, //
				w + p_v.w //
		);
	}

	inline Vector4T operator*(const Vector4T &p_v) const {
		return Vector4T( //
				x * p_v.x, //
				y * p_v.y, //
				z * p_v.z, //
				w * p_v.w //
		);
	}

	inline Vector4T operator*(const T p_scalar) const {
		return Vector4T( //
				x * p_scalar, //
				y * p_scalar, //
				z * p_scalar, //
				w * p_scalar //
		);
	}
};

} // namespace voxel

#endif // VOXEL_VECTOR4T_H
