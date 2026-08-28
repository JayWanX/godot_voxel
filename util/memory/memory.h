#ifndef VOXEL_MEMORY_H
#define VOXEL_MEMORY_H

#include <memory>

// Default new and delete operators.
#if defined(VOXEL_GODOT)

#include <core/os/memory.h>

// Use Godot's allocator.
#define VOXEL_NEW(t) memnew(t)
#define VOXEL_DELETE(t) memdelete(t)
#define VOXEL_ALLOC(size) memalloc(size)
#define VOXEL_REALLOC(p, size) memrealloc(p, size)
#define VOXEL_FREE(p) memfree(p)

#endif

namespace voxel {

// Default, engine-agnostic implementation of unique pointers for this project. Allows to change it in one place.
// Note: array allocations are not used at the moment. Containers are preferred.

template <typename T>
struct DefaultObjectDeleter {
	constexpr DefaultObjectDeleter() noexcept = default;

	// This is required so we can implicitely convert from `UniquePtr<Derived>` to `UniquePtr<Base>`.
	// Looked it up from inside MSVC's STL implementation.
	template <class U, std::enable_if_t<std::is_convertible_v<U *, T *>, int> = 0>
	DefaultObjectDeleter(const DefaultObjectDeleter<U> &) noexcept {}

	inline void operator()(T *obj) {
		VOXEL_DELETE(obj);
	}
};

template <typename T>
using UniquePtr = std::unique_ptr<T, DefaultObjectDeleter<T>>;

template <class T, class... Types, std::enable_if_t<!std::is_array_v<T>, int> = 0>
UniquePtr<T> make_unique_instance(Types &&...args) {
	return UniquePtr<T>(VOXEL_NEW(T(std::forward<Types>(args)...)));
}

// Default, engine-agnostic implementation of shared pointers for this project.

template <class T, class... Types, std::enable_if_t<!std::is_array_v<T>, int> = 0>
inline std::shared_ptr<T> make_shared_instance(Types &&...args) {
	return std::shared_ptr<T>(VOXEL_NEW(T(std::forward<Types>(args)...)), [](T *p) { VOXEL_DELETE(p); });
}

} // namespace voxel

#endif // VOXEL_MEMORY_H
