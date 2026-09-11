#ifndef VOXEL_GODOT_REF_COUNTED_H
#define VOXEL_GODOT_REF_COUNTED_H

#include <functional>

#include <core/object/ref_counted.h>

namespace voxel::godot {

// `(ref1 = ref2).is_valid()` 不起作用，因为 Ref<T> 没有实现返回值的 `operator=`。
// 所以我们可以改写为 `try_get_as(ref2, ref1)`
template <typename From_T, typename To_T>
inline bool try_get_as(const Ref<From_T> &from, Ref<To_T> &to) {
	to = from;
	return to.is_valid();
}

// 允许把 Ref<T> 用作 Godot HashMap 的键
template <typename T>
struct RefHasher {
	static _FORCE_INLINE_ uint32_t hash(const Ref<T> &v) {
		return uint32_t(uint64_t(v.ptr())) * (0x9e3779b1L);
	}
};

} // namespace voxel::godot

namespace std {

// 供 Ref<T> 作为 std::unordered_map 的键使用，按指针而非内容哈希
template <typename T>
struct hash<Ref<T>> {
	inline size_t operator()(const Ref<T> &v) const {
		return std::hash<const T *>{}(v.ptr());
	}
};

} // namespace std

#endif // VOXEL_GODOT_REF_COUNTED_H
