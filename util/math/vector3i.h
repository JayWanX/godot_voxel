#include <core/math/vector3.h>
#include <core/math/vector3i.h>
#ifndef VOXEL_MATH_VECTOR3I_H
#define VOXEL_MATH_VECTOR3I_H

#include "../containers/span.h"
#include <core/math/vector3.h>
#include <core/math/vector3i.h>
#include "../godot/macros.h"
#include "../hash_funcs.h"
#include "../macros.h"
#include "funcs.h"
#include <functional> // 用于 std::hash

namespace voxel {
namespace Vector3iUtil {

constexpr int AXIS_COUNT = 3;

inline Vector3i create(int xyz) {
	return Vector3i(xyz, xyz, xyz);
}

inline void sort_min_max(Vector3i &a, Vector3i &b) {
	math::sort(a.x, b.x);
	math::sort(a.y, b.y);
	math::sort(a.z, b.z);
}

// 返回 64 位整数，因为体积很容易溢出 INT_MAX（例如 1300^3），
// 尽管本模块中很少会遇到那么大的密集体积。
inline uint64_t get_volume_u64(const Vector3i &v) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT_RETURN_V(v.x >= 0 && v.y >= 0 && v.z >= 0, 0);
#endif
	return math::multiply_check_overflow_u64(
			static_cast<uint64_t>(v.x),
			math::multiply_check_overflow_u64(static_cast<uint64_t>(v.y), static_cast<uint64_t>(v.z))
	);
}

inline unsigned int get_zxy_index(const Vector3i &v, const Vector3i area_size) {
	return v.y + area_size.y * (v.x + area_size.x * v.z); // ZXY
}

inline unsigned int get_zxy_index(int x, int y, int z, int sx, int sy) {
	return y + sy * (x + sx * z); // ZXY
}

inline unsigned int get_zyx_index(const Vector3i &v, const Vector3i area_size) {
	return v.x + area_size.x * (v.y + area_size.y * v.z);
}

inline Vector3i from_zxy_index(unsigned int i, const Vector3i area_size) {
	Vector3i pos;
	pos.y = i % area_size.y;
	pos.x = (i / area_size.y) % area_size.x;
	pos.z = i / (area_size.y * area_size.x);
	return pos;
}

inline bool all_members_equal(const Vector3i v) {
	return v.x == v.y && v.y == v.z;
}

inline bool is_unit_vector(const Vector3i v) {
	return Math::abs(v.x) + Math::abs(v.y) + Math::abs(v.z) == 1;
}

inline bool is_valid_size(const Vector3i &s) {
	return s.x >= 0 && s.y >= 0 && s.z >= 0;
}

inline bool is_empty_size(const Vector3i &s) {
	return s.x == 0 || s.y == 0 || s.z == 0;
}

} // namespace Vector3iUtil

namespace math {

inline Vector3i floordiv(const Vector3i v, const Vector3i d) {
	return Vector3i(math::floordiv(v.x, d.x), math::floordiv(v.y, d.y), math::floordiv(v.z, d.z));
}

inline Vector3i floordiv(const Vector3i v, const int d) {
	return Vector3i(math::floordiv(v.x, d), math::floordiv(v.y, d), math::floordiv(v.z, d));
}

inline Vector3i ceildiv(const Vector3i v, const int d) {
	return Vector3i(math::ceildiv(v.x, d), math::ceildiv(v.y, d), math::ceildiv(v.z, d));
}

inline Vector3i ceildiv(const Vector3i v, const Vector3i d) {
	return Vector3i(math::ceildiv(v.x, d.x), math::ceildiv(v.y, d.y), math::ceildiv(v.z, d.z));
}

inline Vector3i wrap(const Vector3i v, const Vector3i d) {
	return Vector3i(math::wrap(v.x, d.x), math::wrap(v.y, d.y), math::wrap(v.z, d.z));
}

inline Vector3i clamp(const Vector3i a, const Vector3i minv, const Vector3i maxv) {
	return Vector3i(
			math::clamp(a.x, minv.x, maxv.x), math::clamp(a.y, minv.y, maxv.y), math::clamp(a.z, minv.z, maxv.z)
	);
}

inline Vector3i abs(const Vector3i v) {
	return Vector3i(Math::abs(v.x), Math::abs(v.y), Math::abs(v.z));
}

inline Vector3i min(const Vector3i a, const Vector3i b) {
	return Vector3i(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

inline Vector3i max(const Vector3i a, const Vector3i b) {
	return Vector3i(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

// 旋转：CW（顺时针）与 CCW（逆时针）均指旋转轴指向观察者。
// 与 Godot Basis 使用的约定相同。CCW 为正角，CW 为负角。

inline Vector3i rotate_x_90_ccw(Vector3i v) {
	return Vector3i(v.x, -v.z, v.y);
}

inline Vector3i rotate_x_90_cw(Vector3i v) {
	return Vector3i(v.x, v.z, -v.y);
}

inline Vector3i rotate_y_90_ccw(Vector3i v) {
	return Vector3i(v.z, v.y, -v.x);
}

inline Vector3i rotate_y_90_cw(Vector3i v) {
	return Vector3i(-v.z, v.y, v.x);
}

inline Vector3i rotate_z_90_ccw(Vector3i v) {
	return Vector3i(-v.y, v.x, v.z);
}

inline Vector3i rotate_z_90_cw(Vector3i v) {
	return Vector3i(v.y, -v.x, v.z);
}

Vector3i rotate_90(Vector3i v, Axis axis, bool clockwise);
void rotate_90(Span<Vector3i> vecs, const Axis axis, const bool clockwise);

inline int manhattan_distance(const Vector3i &a, const Vector3i &b) {
	return Math::abs(a.x - b.x) + Math::abs(a.y - b.y) + Math::abs(a.z - b.z);
}

inline int chebyshev_distance(const Vector3i &a, const Vector3i &b) {
	// 在切比雪夫距离下，立方体表面上的点到其中心距离相等
	return math::max(math::max(Math::abs(a.x - b.x), Math::abs(a.y - b.y)), Math::abs(a.z - b.z));
}

inline int dot(const Vector3i &a, const Vector3i &b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

} // namespace math

class TextWriter;
TextWriter &operator<<(TextWriter &w, const Vector3i &v);

} // namespace voxel

// 为避免不直观的重载解析编译错误，运算符重载应
// 定义在其操作类型所在的同一命名空间内……即 Godot 的命名空间。
// 编译器只会在参数的命名空间内查找重载（也就是 Koenig 查找，是吗？）。
// https://stackoverflow.com/questions/5195512/namespaces-and-operator-resolution

inline Vector3i operator<<(const Vector3i &a, int b) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(b >= 0);
#endif
	return Vector3i(a.x << b, a.y << b, a.z << b);
}

inline Vector3i operator>>(const Vector3i &a, int b) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(b >= 0);
#endif
	using namespace voxel::math;
	return Vector3i(arithmetic_rshift(a.x, b), arithmetic_rshift(a.y, b), arithmetic_rshift(a.z, b));
}

inline Vector3i operator&(const Vector3i &a, uint32_t b) {
	return Vector3i(a.x & b, a.y & b, a.z & b);
}

inline Vector3i operator%(const Vector3i &a, int b) {
	return Vector3i(a.x % b, a.y % b, a.z % b);
}


// 用于 Godot
struct Vector3iHasher {
	static inline uint32_t hash(const Vector3i &v) {
		uint32_t hash = voxel::hash_djb2_one_32(v.x);
		hash = voxel::hash_djb2_one_32(v.y, hash);
		return voxel::hash_djb2_one_32(v.z, hash);

		// Godot 所用的实现。结果似乎更慢？
		// uint32_t h = voxel::hash_murmur3_one_32(v.x);
		// h = voxel::hash_murmur3_one_32(v.y, h);
		// h = voxel::hash_murmur3_one_32(v.z, h);
		// return voxel::hash_fmix32(h);
	}
};

// For STL
namespace std {
template <>
struct hash<Vector3i> {
	size_t operator()(const Vector3i &v) const {
		return Vector3iHasher::hash(v);
	}
};
} // namespace std

#endif // VOXEL_MATH_VECTOR3I_H
