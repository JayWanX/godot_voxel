#ifndef VOXEL_VECTOR3F_H
#define VOXEL_VECTOR3F_H

#include "../errors.h"
#include "vector3t.h"

namespace voxel {

// 32 位浮点精度三维向量。
// 因为 Godot 的 `Vector3` 使用 `real_t`，所以当 `real_t` 为 `double` 时，会强制某些东西使用双精度
// 向量，而它们并不需要那么高的精度。这对部分第三方库也是个问题，
// 因为它们不支持 `double` 作为结果类型。
typedef Vector3T<float> Vector3f;

namespace math {

inline Vector3f floor(const Vector3f a) {
	return Vector3f(Math::floor(a.x), Math::floor(a.y), Math::floor(a.z));
}

inline Vector3f ceil(const Vector3f a) {
	return Vector3f(Math::ceil(a.x), Math::ceil(a.y), Math::ceil(a.z));
}

inline Vector3f lerp(const Vector3f a, const Vector3f b, const float t) {
	return Vector3f(Math::lerp(a.x, b.x, t), Math::lerp(a.y, b.y, t), Math::lerp(a.z, b.z, t));
}

inline bool has_nan(const Vector3f &v) {
	return is_nan(v.x) || is_nan(v.y) || is_nan(v.z);
}

inline Vector3f normalized(const Vector3f &v) {
	const float lengthsq = length_squared(v);
	if (lengthsq == 0) {
		return Vector3f();
	} else {
		const float length = Math::sqrt(lengthsq);
		return v / length;
	}
}

inline Vector3f normalized(Vector3f v, float &out_length) {
	const float lengthsq = length_squared(v);
	if (lengthsq == 0) {
		out_length = 0.f;
		return Vector3f();
	} else {
		const float length = Math::sqrt(lengthsq);
		out_length = length;
		return v / length;
	}
}

inline bool is_normalized(const Vector3f &v) {
	// 使用 length_squared() 而非 length() 以避免 sqrt()，使判断更严格。
	return Math::is_equal_approx(length_squared(v), 1, float(UNIT_EPSILON));
}

inline bool is_equal_approx(const Vector3f a, const Vector3f b) {
	return Math::is_equal_approx(a.x, b.x) && Math::is_equal_approx(a.y, b.y) && Math::is_equal_approx(a.z, b.z);
}

} // namespace math

class TextWriter;
TextWriter &operator<<(TextWriter &w, const Vector3f &v);

} // namespace voxel

#endif // VOXEL_VECTOR3F_H
