#ifndef VOXEL_GODOT_DICTIONARY_H
#define VOXEL_GODOT_DICTIONARY_H

#if defined(VOXEL_GODOT)
#include <core/variant/dictionary.h>
#endif

namespace voxel::godot {

template <typename T>
inline bool try_get(const Dictionary &d, const Variant &key, T &out_value) {
#if defined(VOXEL_GODOT)
	const Variant *v = d.getptr(key);
	if (v == nullptr) {
		return false;
	}
	// TODO There is no easy way to return `false` if the value doesn't have the right type...
	// Because multiple C++ types match Variant types, and Variant types match multiple C++ types, and silently convert
	// between them.
	out_value = *v;
	return true;
#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_DICTIONARY_H
