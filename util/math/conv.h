#ifndef VOXEL_CONV_H
#define VOXEL_CONV_H

#include "../godot/core/transform_3d.h"
#include "../godot/core/vector2.h"
#include "../godot/core/vector2i.h"
#include "../godot/core/vector3.h"
#include "../godot/core/vector3i.h"
#include "transform3f.h"
#include "vector2f.h"
#include "vector3d.h"
#include "vector3f.h"
#include "vector3i16.h"
#include <limits>

namespace voxel {

// 显式转换方法。不放在各自文件中，否则会引起循环依赖。

// Godot 格式 => Godot 格式
// 注意，Godot 模块中存在隐式转换。但我不喜欢隐式转换。

inline Vector2i to_vec2i(const Vector2 v) {
	return Vector2i(v.x, v.y);
}

inline Vector3i to_vec3i(Vector3 v) {
	return Vector3i(v.x, v.y, v.z);
}

inline Vector3 to_vec3(const Vector3i v) {
	return Vector3(v.x, v.y, v.z);
}

// 若找不到显式重载，则让 `to_vec3` 调用编译失败。
// 为了避免 Godot 在意外传入 `Vector3` 时，因编译器匹配到该重载而静默地将 Vector3 转换为 Vector3i，
// （我们的风格是显式向量转换，但出错时
// 会与 Godot 的风格冲突，产生难以察觉的 bug）。
template <typename T>
inline Vector3 to_vec3(T v) = delete;

// Godot 格式 => VOXEL 格式

inline Vector2f to_vec2f(Vector2 v) {
	return Vector2f(v.x, v.y);
}

inline Vector2f to_vec2f(Vector2i v) {
	return Vector2f(v.x, v.y);
}

inline Vector3f to_vec3f(Vector3i v) {
	return Vector3f(v.x, v.y, v.z);
}

inline Vector3f to_vec3f(Vector3 v) {
	return Vector3f(v.x, v.y, v.z);
}

inline Vector3i16 to_vec3i16(const Vector3i v) {
	return Vector3i16(v.x, v.y, v.z);
}

inline Basis3f to_basis3f(const Basis &src) {
	Basis3f dst;
	dst.rows[0] = to_vec3f(src.rows[0]);
	dst.rows[1] = to_vec3f(src.rows[1]);
	dst.rows[2] = to_vec3f(src.rows[2]);
	return dst;
}

inline Transform3f to_transform3f(const Transform3D &t) {
	return Transform3f(to_basis3f(t.basis), to_vec3f(t.origin));
}

// VOXEL 格式 => Godot 格式

template <typename T>
inline Vector2 to_vec2(const Vector2T<T> v) {
	return Vector2(v.x, v.y);
}

template <typename T>
inline Vector2i to_vec2i(const Vector2T<T> v) {
	return Vector2i(v.x, v.y);
}

template <typename T>
inline Vector3i to_vec3i(const Vector3T<T> v) {
	return Vector3i(v.x, v.y, v.z);
}

template <typename T>
inline Vector3 to_vec3(const Vector3T<T> v) {
	return Vector3(v.x, v.y, v.z);
}

inline Basis to_basis3(const Basis3f &src) {
	Basis dst;
	dst.rows[0] = to_vec3(src.rows[0]);
	dst.rows[1] = to_vec3(src.rows[1]);
	dst.rows[2] = to_vec3(src.rows[2]);
	return dst;
}

inline Transform3D to_transform3(const Transform3f &t) {
	return Transform3D(to_basis3(t.basis), to_vec3(t.origin));
}

// VOXEL 格式 => VOXEL 格式

template <typename T>
inline Vector3d to_vec3d(const Vector3T<T> v) {
	return Vector3d(v.x, v.y, v.z);
}

template <typename T>
inline Vector3f to_vec3f(const Vector3T<T> v) {
	return Vector3f(v.x, v.y, v.z);
}

inline bool can_convert_to_i16(Vector3i p) {
	return p.x >= std::numeric_limits<int16_t>::min() && p.x <= std::numeric_limits<int16_t>::max() &&
			p.y >= std::numeric_limits<int16_t>::min() && p.y <= std::numeric_limits<int16_t>::max() &&
			p.z >= std::numeric_limits<int16_t>::min() && p.z <= std::numeric_limits<int16_t>::max();
}

namespace math {

inline Vector3i floor_to_int(const Vector3 &f) {
	return Vector3i(Math::floor(f.x), Math::floor(f.y), Math::floor(f.z));
}

inline Vector3i floor_to_int(const Vector3f &f) {
	return Vector3i(Math::floor(f.x), Math::floor(f.y), Math::floor(f.z));
}

inline Vector3i round_to_int(const Vector3 &f) {
	return Vector3i(Math::round(f.x), Math::round(f.y), Math::round(f.z));
}

inline Vector3i ceil_to_int(const Vector3 &f) {
	return Vector3i(Math::ceil(f.x), Math::ceil(f.y), Math::ceil(f.z));
}

inline Vector3i ceil_to_int(const Vector3f &f) {
	return Vector3i(Math::ceil(f.x), Math::ceil(f.y), Math::ceil(f.z));
}

} // namespace math
} // namespace voxel

#endif // VOXEL_CONV_H
