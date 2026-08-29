#ifndef VOXEL_STD_VECTOR_H
#define VOXEL_STD_VECTOR_H

#include "../memory/std_allocator.h"
#include "span.h"
#include <vector>

namespace voxel {

// 使用我们自己的默认分配器的便捷别名。在使用 Godot 时，它会使用 Godot 的默认分配器。
// （相比之下，直接使用 std::vector 始终使用标准库的默认分配器）
template <typename TValue, typename TAllocator = StdDefaultAllocator<TValue>>
using StdVector = std::vector<TValue, TAllocator>;

template <typename TValue, typename TAllocator>
Span<TValue> to_span(std::vector<TValue, TAllocator> &vec) {
	return Span<TValue>(vec.data(), 0, vec.size());
}

template <typename TValue, typename TAllocator>
Span<const TValue> to_span(const std::vector<TValue, TAllocator> &vec) {
	return Span<const TValue>(vec.data(), 0, vec.size());
}

template <typename TValue, typename TAllocator>
Span<TValue> to_span_from_position_and_size(std::vector<TValue, TAllocator> &vec, unsigned int pos, unsigned int size) {
	VOXEL_ASSERT(pos + size <= vec.size());
	return Span<TValue>(vec.data(), pos, pos + size);
}

template <typename TValue, typename TAllocator>
Span<const TValue> to_span_from_position_and_size(
		const std::vector<TValue, TAllocator> &vec,
		unsigned int pos,
		unsigned int size
) {
	VOXEL_ASSERT(pos + size <= vec.size());
	return Span<const TValue>(vec.data(), pos, pos + size);
}

// TODO 弃用，现在 Span 拥有转换构造函数可以实现这一点
template <typename TValue, typename TAllocator>
Span<const TValue> to_span_const(const std::vector<TValue, TAllocator> &vec) {
	return Span<const TValue>(vec.data(), 0, vec.size());
}

} // namespace voxel

#endif // VOXEL_STD_VECTOR_H
