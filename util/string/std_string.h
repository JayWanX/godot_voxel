#ifndef VOXEL_STD_STRING_H
#define VOXEL_STD_STRING_H

#include "../memory/std_allocator.h"
#include <string>

#ifdef __GNUC__
#include <string_view>
#endif

namespace voxel {

using StdString = std::basic_string<char, std::char_traits<char>, StdDefaultAllocator<char>>;

struct FwdConstStdString {
	const StdString &s;
	FwdConstStdString(const StdString &p_s) : s(p_s) {}
};

struct FwdMutableStdString {
	StdString &s;
	FwdMutableStdString(StdString &p_s) : s(p_s) {}
};

class TextWriter;

TextWriter &operator<<(TextWriter &w, const StdString &s);
TextWriter &operator<<(TextWriter &w, const std::string_view s);

} // namespace voxel

#ifdef __GNUC__

// 尝试修复 GCC 处理 `unordered_map<StdString, V> map;` 时遇到的问题。
// 我不太理解为什么会发生这种情况，也不确定是不是 bug。在 Compiler Explorer 中，所有早于
// GCC 13.1 的版本编译这类代码都会失败，只有 13.1 及以后可以。手动为我们的
// 别名定义哈希特化似乎可以绕过它。
namespace std {
template <>
struct hash<voxel::StdString> {
	size_t operator()(const voxel::StdString &v) const {
		const std::string_view s(v);
		std::hash<std::string_view> hasher;
		return hasher(s);
	}
};
} // namespace std

#endif

#endif // VOXEL_STD_STRING_H
