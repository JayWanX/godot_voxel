#ifndef VOXEL_GODOT_DICTIONARY_H
#define VOXEL_GODOT_DICTIONARY_H

#include <core/variant/dictionary.h>

namespace voxel::godot {

template <typename T>
inline bool try_get(const Dictionary &d, const Variant &key, T &out_value) {
	const Variant *v = d.getptr(key);
	if (v == nullptr) {
		return false;
	}
	// TODO 如果值类型不符，没有简单的方法返回 `false`……
	// 因为多个 C++ 类型匹配同一个 Variant 类型，而 Variant 类型又匹配多个 C++ 类型，并且会
	// 在它们之间静默转换。
	out_value = *v;
	return true;
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_DICTIONARY_H
