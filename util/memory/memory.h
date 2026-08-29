#ifndef VOXEL_MEMORY_H
#define VOXEL_MEMORY_H

#include <memory>

// 默认的 new 和 delete 运算符。
#if defined(VOXEL_GODOT)

#include <core/os/memory.h>

// 使用 Godot 的分配器。
#define VOXEL_NEW(t) memnew(t)
#define VOXEL_DELETE(t) memdelete(t)
#define VOXEL_ALLOC(size) memalloc(size)
#define VOXEL_REALLOC(p, size) memrealloc(p, size)
#define VOXEL_FREE(p) memfree(p)

#endif

namespace voxel {

// 本项目默认的、与引擎无关的独有指针实现。允许在同一个地方修改它。
// 注意：目前不使用数组分配。更倾向于使用容器。

template <typename T>
struct DefaultObjectDeleter {
	constexpr DefaultObjectDeleter() noexcept = default;

	// 这是必要的，这样我们就可以从 `UniquePtr<Derived>` 隐式转换为 `UniquePtr<Base>`。
	// 这是在 MSVC 的 STL 实现中查到的。
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

// 本项目默认的、与引擎无关的共享指针实现。

template <class T, class... Types, std::enable_if_t<!std::is_array_v<T>, int> = 0>
inline std::shared_ptr<T> make_shared_instance(Types &&...args) {
	return std::shared_ptr<T>(VOXEL_NEW(T(std::forward<Types>(args)...)), [](T *p) { VOXEL_DELETE(p); });
}

} // namespace voxel

#endif // VOXEL_MEMORY_H
