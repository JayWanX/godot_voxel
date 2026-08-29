#include "string.h"
// #include <sstream>
#include "../../io/text_writer.h"

namespace voxel::godot {

#ifdef TOOLS_ENABLED

PackedStringArray to_godot(const StdVector<std::string_view> &svv) {
	PackedStringArray psa;
	// 不预先调整大小。
	for (unsigned int i = 0; i < svv.size(); ++i) {
		psa.append(to_godot(svv[i]));
	}
	return psa;
}

PackedStringArray to_godot(const StdVector<StdString> &sv) {
	PackedStringArray psa;
	// 不预先调整大小。
	for (unsigned int i = 0; i < sv.size(); ++i) {
		psa.append(to_godot(sv[i]));
	}
	return psa;
}

#endif

} // namespace voxel::godot

VOXEL_GODOT_NAMESPACE_BEGIN

voxel::TextWriter &operator<<(voxel::TextWriter &w, GodotStringWrapper s) {
	const CharString cs = s.s.utf8();
	// String 有来自多种类型的非显式构造函数，导致这里产生歧义
	w.write_chars(voxel::Span<const char>(cs.get_data(), cs.length()));
	return w;
}

VOXEL_GODOT_NAMESPACE_END
