#include <core/version.h>
#ifndef VOXEL_GODOT_STRING_H
#define VOXEL_GODOT_STRING_H

#include <core/string/ustring.h>

#include "../../string/std_string.h"
#include <string_view>

#ifdef TOOLS_ENABLED
#include "../../containers/std_vector.h"
#include "variant.h"
#endif

#include "../../containers/span.h"
#include <core/version.h>
#include "../macros.h"

namespace voxel {
class TextWriter;
}

namespace voxel::godot {

inline String to_godot(const std::string_view sv) {
	return String::utf8(sv.data(), sv.size());
}

// 事实证明这些函数目前只在编辑器中使用。
// 它们是通用的，但我必须包装它们，否则 GCC 会把"未使用"当作错误警告抛出。
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
	return s.append_utf8(utf8.data(), utf8.size());
}

inline String ptr2s(const void *p) {
	return String::num_uint64((uint64_t)p, 16);
}

} // namespace voxel::godot

// `TTR` 表示 "tools translate"（工具翻译），用于仅限编辑器的本地化消息。
// Godot 在发布构建中不定义用于消息翻译的 TTR 宏。不过，本模块中有一些非编辑器
// 代码会产生错误，而我们仍然希望它们能正常编译。
#ifdef TOOLS_ENABLED
#define VOXEL_TTR(msg) TTR(msg)
#else
#define VOXEL_TTR(msg) String(msg)
#endif


// `voxel::format()` 需要用到。
// 我放弃了在这里漂亮地转换 Godot 的 String……它带有非显式的 `const char*` 构造函数，那会让
// 其他重载产生歧义……
// StdStringStream &operator<<(StdStringStream &ss, const String &s);
struct GodotStringWrapper {
	GodotStringWrapper(const String &p_s) : s(p_s) {}
	const String &s;
};
voxel::TextWriter &operator<<(voxel::TextWriter &ss, GodotStringWrapper s);


namespace std {

// 供 String 作为 std::unordered_map 的键使用
template <>
struct hash<String> {
	inline size_t operator()(const String &v) const {
		return v.hash();
	}
};

} // namespace std

#endif // VOXEL_GODOT_STRING_H
