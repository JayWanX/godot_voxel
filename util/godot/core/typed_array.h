#ifndef VOXEL_GODOT_TYPED_ARRAY_H
#define VOXEL_GODOT_TYPED_ARRAY_H

#if defined(VOXEL_GODOT)
#include <core/variant/typed_array.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/typed_array.hpp>
#endif

#include "../../containers/span.h"
#include <vector>

namespace voxel::godot {

template <typename T>
inline void copy_to(TypedArray<T> &dst, Span<const T> src) {
	dst.resize(src.size());
	for (unsigned int i = 0; i < src.size(); ++i) {
		dst[i] = src[i];
	}
}

template <typename T>
inline void copy_to(TypedArray<T> &dst, Span<const Ref<T>> src) {
	dst.resize(src.size());
	for (unsigned int i = 0; i < src.size(); ++i) {
		dst[i] = src[i];
	}
}

template <typename T, typename TAllocator>
inline void copy_to(std::vector<T, TAllocator> &dst, const TypedArray<T> &src) {
	dst.resize(src.size());
	for (int i = 0; i < src.size(); ++i) {
		dst[i] = src[i];
	}
}

template <typename T, typename TAllocator>
inline void copy_to(std::vector<Ref<T>, TAllocator> &dst, const TypedArray<T> &src) {
	dst.resize(src.size());
	for (int i = 0; i < src.size(); ++i) {
		dst[i] = src[i];
	}
}

template <typename T, typename TAllocator>
inline void copy_range_to(
		std::vector<Ref<T>, TAllocator> &dst,
		const TypedArray<T> &src,
		const int from,
		const int count
) {
	if (count == 0) {
		dst.clear();
		return;
	}
	VOXEL_ASSERT_RETURN(from >= 0 && from < src.size());
	VOXEL_ASSERT_RETURN(count >= 0 && from + count <= src.size());
	dst.resize(count);
	const int to = from + count;
	for (int i = from; i < to; ++i) {
		dst[i - from] = src[i];
	}
}

template <typename T>
inline TypedArray<T> to_typed_array(Span<const T> src) {
	TypedArray<T> array;
	copy_to(array, src);
	return array;
}

template <typename T>
inline TypedArray<T> to_typed_array(Span<const Ref<T>> src) {
	TypedArray<T> array;
	copy_to(array, src);
	return array;
}

#if defined(VOXEL_GODOT)

template <typename T>
Vector<Ref<T>> to_ref_vector(const TypedArray<T> &typed_array) {
	Vector<Ref<T>> refs;
	refs.resize(typed_array.size());
	for (int i = 0; i < typed_array.size(); ++i) {
		refs.write[i] = typed_array[i];
	}
	return refs;
}

#endif

} // namespace voxel::godot

#endif // VOXEL_GODOT_TYPED_ARRAY_H
