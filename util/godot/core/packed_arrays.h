#include <core/variant/array.h>
#ifndef VOXEL_GODOT_PACKED_ARRAYS_H
#define VOXEL_GODOT_PACKED_ARRAYS_H

#include "../../containers/span.h"
#include "../../math/vector2f.h"
#include "../../math/vector3f.h"
#include "variant.h"
#include <cstdint>

#ifdef TOOLS_ENABLED
#include "../macros.h"
#include <core/variant/array.h>
#include "packed_string_array_fwd.h"
#endif

namespace voxel::godot {

// 向量的专用拷贝函数，因为它们使用 `real_t`，而它既可能是 `float` 也可能是 `double`
void copy_to(PackedVector3Array &dst, const Span<const Vector3f> src);
void copy_to(PackedVector2Array &dst, const Span<const Vector2f> src);

// 类型匹配时的拷贝函数。
// 如果我们想同时支持以模块和扩展方式编译，就不能使用模板 Vector 的代码。
// 所以下面为每种情况都定义了，而不是用模板。
void copy_to(PackedVector3Array &dst, Span<const Vector3> src);
void copy_to(PackedInt32Array &dst, Span<const int32_t> src);
void copy_to(PackedColorArray &dst, Span<const Color> src);
void copy_to(PackedFloat32Array &dst, Span<const float> src);
void copy_to(PackedByteArray &dst, Span<const uint8_t> src);

void copy_to(Span<uint8_t> dst, const PackedByteArray &src);
void copy_to(Span<float> dst, const PackedFloat32Array &src);

template <typename T>
inline void copy_bytes_to(PackedByteArray &dst, Span<const T> src) {
	const size_t bytes_count = src.size() * sizeof(T);
	dst.resize(bytes_count);
	uint8_t *dst_w = dst.ptrw();
	VOXEL_ASSERT(dst_w != nullptr);
	memcpy(dst_w, src.data(), bytes_count);
}

template <typename T>
inline void copy_bytes_to(PackedByteArray &dst, T src) {
	dst.resize(sizeof(T));
	uint8_t *dst_w = dst.ptrw();
	VOXEL_ASSERT(dst_w != nullptr);
	memcpy(dst_w, &src, sizeof(T));
}

#ifdef TOOLS_ENABLED
Array to_array(const PackedStringArray &src);
#endif

} // namespace voxel::godot

namespace voxel {

// template <typename T>
// Span<const T> to_span_const(const Vector<T> &a) {
// 	return Span<const T>(a.ptr(), 0, a.size());
// }

inline Span<const Vector2> to_span(const PackedVector2Array &a) {
	return Span<const Vector2>(a.ptr(), a.size());
}

inline Span<Vector2> to_span(PackedVector2Array &a) {
	return Span<Vector2>(a.ptrw(), a.size());
}

inline Span<const Vector3> to_span(const PackedVector3Array &a) {
	return Span<const Vector3>(a.ptr(), a.size());
}

inline Span<const float> to_span(const PackedFloat32Array &a) {
	return Span<const float>(a.ptr(), a.size());
}

inline Span<const int32_t> to_span(const PackedInt32Array &a) {
	return Span<const int32_t>(a.ptr(), a.size());
}

inline Span<const uint8_t> to_span(const PackedByteArray &a) {
	return Span<const uint8_t>(a.ptr(), a.size());
}

inline Span<String> to_span(PackedStringArray &a) {
	return Span<String>(a.ptrw(), a.size());
}

} // namespace voxel

#endif // VOXEL_GODOT_PACKED_ARRAYS_H
