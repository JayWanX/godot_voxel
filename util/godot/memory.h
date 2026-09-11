#ifndef VOXEL_GODOT_MEMORY_H
#define VOXEL_GODOT_MEMORY_H

#include <core/os/memory.h>

#include <memory>

namespace voxel::godot {

/*// 创建始终使用 Godot 分配函数的 shared_ptr
template <typename T>
inline std::shared_ptr<T> gd_make_shared() {
	// std::make_shared() 显然不允许我们指定自定义的 new 和 delete
	return std::shared_ptr<T>(memnew(T), memdelete<T>);
}*/

// 供智能指针（如 std::unique_ptr）使用
template <typename T>
struct ObjectDeleter {
	inline void operator()(T *obj) {
		memdelete(obj);
	}
};

// `std::unique_ptr` 的特化版本，始终使用 Godot 的 `memdelete()` 作为删除器。
template <typename T>
using ObjectUniquePtr = std::unique_ptr<T, ObjectDeleter<T>>;

// 用 `memnew()` 在内部构造对象，创建 `GodotObjectUniquePtr<T>`。
template <typename T>
ObjectUniquePtr<T> make_unique() {
	return ObjectUniquePtr<T>(memnew(T));
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_MEMORY_H
