#ifndef VOXEL_GODOT_STD_ALLOCATOR_H
#define VOXEL_GODOT_STD_ALLOCATOR_H

#include <limits>
// #include <new>

#include "../errors.h"
#include "memory.h"

#ifdef DEBUG_ENABLED
#include <atomic>
#endif

namespace voxel {

#ifdef DEBUG_ENABLED
namespace StdDefaultAllocatorCounters {
extern std::atomic_uint64_t g_allocated;
extern std::atomic_uint64_t g_deallocated;
} // namespace StdDefaultAllocatorCounters
#endif

// 满足标准库要求、由 Godot 分配器支撑的默认分配器。
template <class T>
struct StdDefaultAllocator {
	typedef T value_type;

	StdDefaultAllocator() = default;

	template <class U>
	constexpr StdDefaultAllocator(const StdDefaultAllocator<U> &) noexcept {}

	[[nodiscard]] T *allocate(std::size_t n) {
		VOXEL_ASSERT(n <= std::numeric_limits<std::size_t>::max() / sizeof(T));
		// if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
		// 	throw std::bad_array_new_length();
		// }

		if (T *p = static_cast<T *>(VOXEL_ALLOC(n * sizeof(T)))) {
#ifdef DEBUG_ENABLED
			StdDefaultAllocatorCounters::g_allocated += n * sizeof(T);
#endif
			return p;
		}

		// throw std::bad_alloc();
		VOXEL_CRASH_MSG("Bad alloc");
		return nullptr;
	}

	void deallocate(T *p, std::size_t n) noexcept {
#ifdef DEBUG_ENABLED
		StdDefaultAllocatorCounters::g_deallocated += n * sizeof(T);
#endif
		VOXEL_FREE(p);
	}

	// 注意：只要分配器是模板类，定义 `rebind` 结构体就是可选的。它由
	// `allocator_traits` 提供。容器使用 `rebind` 获取带不同 T 的同一分配器，
	// 以便分配内部数据结构（链表的节点、unordered_map 的桶……）
};

template <class T, class U>
bool operator==(const StdDefaultAllocator<T> &, const StdDefaultAllocator<U> &) {
	return true;
}

template <class T, class U>
bool operator!=(const StdDefaultAllocator<T> &, const StdDefaultAllocator<U> &) {
	return false;
}

} // namespace voxel

#endif // VOXEL_GODOT_STD_ALLOCATOR_H
