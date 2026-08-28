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

// Default allocator matching standard library requirements.
// When compiling with Godot or GDExtension, it will use Godot's default allocator.
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

	// Note: defining a `rebind` struct is optional as long as the allocator is a template class. It is therefore
	// provided by `allocator_traits`. `rebind` is used by containers to obtain the same allocator with a different T,
	// in order to allocate internal data structures (nodes of linked list, buckets of unordered_map...)
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
