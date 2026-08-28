#ifndef VOXEL_GODOT_STRING_H
#define VOXEL_GODOT_STRING_H

#if defined(VOXEL_GODOT)
#include <core/string/ustring.h>
#endif

#include "../../string/std_string.h"
#include <string_view>

#ifdef TOOLS_ENABLED
#include "../../containers/std_vector.h"
#include "variant.h"
#endif

#include "../../containers/span.h"
#include "../core/version.h"
#include "../macros.h"

namespace voxel {
class TextWriter;
}

namespace voxel::godot {

inline String to_godot(const std::string_view sv) {
	return String::utf8(sv.data(), sv.size());
}

// Turns out these functions are only used in editor for now.
// They are generic, but I have to wrap them, otherwise GCC throws warnings-as-errors for them being unused.
#ifdef TOOLS_ENABLED

PackedStringArray to_godot(const StdVector<std::string_view> &svv);
PackedStringArray to_godot(const StdVector<StdString> &sv);

template <typename T>
String join_comma_separated(Span<const T> items) {
	String str;
	for (unsigned int i = 0; i < items.size(); ++i) {
		if (i > 0) {
			str += ", ";
		}
		str += Variant(items[i]).stringify();
	}
	return str;
}

#endif

inline bool is_resource_file(const String &path) {
	return path.begins_with("res://") && path.find("::") == -1;
}

inline StdString to_std_string(const String &godot_string) {
	const CharString cs = godot_string.utf8();
	StdString s = cs.get_data();
	return s;
}

inline Error parse_utf8(String &s, Span<const char> utf8) {
#if defined(VOXEL_GODOT)
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 5
	return s.append_utf8(utf8.data(), utf8.size());
#else
	return s.parse_utf8(utf8.data(), utf8.size());
#endif
#endif
}

inline String ptr2s(const void *p) {
	return String::num_uint64((uint64_t)p, 16);
}

} // namespace voxel::godot

// `TTR` means "tools translate", which is for editor-only localized messages.
// Godot does not define the TTR macro for translation of messages in release builds. However, there are some non-editor
// code that can produce errors in this module, and we still want them to compile properly.
#if defined(VOXEL_GODOT) && defined(TOOLS_ENABLED)
#define VOXEL_TTR(msg) TTR(msg)
#else
#define VOXEL_TTR(msg) String(msg)
#endif

VOXEL_GODOT_NAMESPACE_BEGIN

// Needed for `voxel::format()`.
// I gave up trying to nicely convert Godot's String here... it has non-explicit `const char*` constructor, that makes
// other overloads ambiguous...
// StdStringStream &operator<<(StdStringStream &ss, const String &s);
struct GodotStringWrapper {
	GodotStringWrapper(const String &p_s) : s(p_s) {}
	const String &s;
};
voxel::TextWriter &operator<<(voxel::TextWriter &ss, GodotStringWrapper s);

VOXEL_GODOT_NAMESPACE_END

namespace std {

// For String keys in std::unordered_map
template <>
struct hash<String> {
	inline size_t operator()(const String &v) const {
		return v.hash();
	}
};

} // namespace std

#endif // VOXEL_GODOT_STRING_H
