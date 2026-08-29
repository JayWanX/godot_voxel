#ifndef VOXEL_VECTOR2F_H
#define VOXEL_VECTOR2F_H

#include "../errors.h"
#include "vector2t.h"

namespace voxel {

// 32 位浮点精度二维向量。
// 因为 Godot 的 `Vector2` 使用 `real_t`，所以当 `real_t` 为 `double` 时，会强制某些东西使用双精度
// 向量，而它们并不需要那么高的精度。
typedef Vector2T<float> Vector2f;

namespace math {

inline Vector2f floor(const Vector2f a) {
	return Vector2f(Math::floor(a.x), Math::floor(a.y));
}

inline Vector2f lerp(const Vector2f a, const Vector2f b, const float t) {
	return Vector2f(Math::lerp(a.x, b.x, t), Math::lerp(a.y, b.y, t));
}

inline bool is_equal_approx(const Vector2f a, const Vector2f b) {
	return Math::is_equal_approx(a.x, b.x) && Math::is_equal_approx(a.y, b.y);
}

} // namespace math

class TextWriter;
TextWriter &operator<<(TextWriter &w, const Vector2f &v);

} // namespace voxel

#endif // VOXEL_VECTOR2F_H
