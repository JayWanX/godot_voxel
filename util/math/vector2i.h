#ifndef VOXEL_MATH_VECTOR2I_H
#define VOXEL_MATH_VECTOR2I_H

#include "../errors.h"
#include "../godot/core/vector2i.h"
#include "../godot/macros.h"
#include "../hash_funcs.h"
#include "../macros.h"
#include "funcs.h"
#include <functional> // 用于 std::hash

VOXEL_GODOT_NAMESPACE_BEGIN

inline Vector2i operator&(const Vector2i &a, int b) {
	return Vector2i(a.x & b, a.y & b);
}

VOXEL_GODOT_NAMESPACE_END

namespace voxel {

namespace Vector2iUtil {

inline Vector2i create(int xy) {
	return Vector2i(xy, xy);
}

inline int64_t get_area(const Vector2i v) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT_RETURN_V(v.x >= 0 && v.y >= 0, 0);
#endif
	return v.x * v.y;
}

inline unsigned int get_yx_index(const Vector2i v, const Vector2i area_size) {
	return v.x + v.y * area_size.x;
}

} // namespace Vector2iUtil

namespace math {

inline Vector2i floordiv(const Vector2i v, const Vector2i d) {
	return Vector2i(math::floordiv(v.x, d.x), math::floordiv(v.y, d.y));
}

inline Vector2i floordiv(const Vector2i v, const int d) {
	return Vector2i(math::floordiv(v.x, d), math::floordiv(v.y, d));
}

inline Vector2i ceildiv(const Vector2i v, const int d) {
	return Vector2i(math::ceildiv(v.x, d), math::ceildiv(v.y, d));
}

inline Vector2i ceildiv(const Vector2i v, const Vector2i d) {
	return Vector2i(math::ceildiv(v.x, d.x), math::ceildiv(v.y, d.y));
}

inline int chebyshev_distance(const Vector2i &a, const Vector2i &b) {
	// 在切比雪夫距离下，正方形各边上的点到其中心距离相等
	return math::max(Math::abs(a.x - b.x), Math::abs(a.y - b.y));
}

inline Vector2i min(const Vector2i a, const Vector2i b) {
	return Vector2i(min(a.x, b.x), min(a.y, b.y));
}

} // namespace math

class TextWriter;
TextWriter &operator<<(TextWriter &w, const Vector2i &v);

} // namespace voxel

// For STL
namespace std {
template <>
struct hash<Vector2i> {
	size_t operator()(const Vector2i &v) const {
		// TODO 这是 32 位的，如果改成 64 位是否更好？
		uint32_t h = voxel::hash_murmur3_one_32(v.x);
		h = voxel::hash_murmur3_one_32(v.y, h);
		return voxel::hash_fmix32(h);
	}
};
} // namespace std

#endif // VOXEL_MATH_VECTOR2I_H
