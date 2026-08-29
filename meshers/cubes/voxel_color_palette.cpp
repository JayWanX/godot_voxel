#include "voxel_color_palette.h"
#include "../../util/containers/span.h"
#ifdef VOXEL_GODOT
#include "../../util/godot/core/class_db.h"
#endif

namespace voxel {

VoxelColorPalette::VoxelColorPalette() {
	// 默认调色板
	_colors[0] = Color8(0, 0, 0, 0);
	_colors[1] = Color8(255, 255, 255, 255);
	for (unsigned int i = 2; i < _colors.size(); ++i) {
		_colors[i] = Color8(0, 0, 0, 255);
	}
}

void VoxelColorPalette::set_color(int index, Color color) {
	ERR_FAIL_INDEX(index, static_cast<int>(_colors.size()));
	_colors[index] = Color8(color);
}

Color VoxelColorPalette::get_color(int index) const {
	ERR_FAIL_INDEX_V(index, static_cast<int>(_colors.size()), Color());
	return _colors[index];
}

PackedColorArray VoxelColorPalette::get_colors() const {
	PackedColorArray dst;
	// 不预先调整大小。
	for (unsigned int i = 0; i < _colors.size(); ++i) {
		dst.push_back(_colors[i]);
	}
	return dst;
}

void VoxelColorPalette::set_colors(PackedColorArray colors) {
	// 颜色数量是固定的，但我们无法轻易阻止 Godot 允许用户设置动态数组
	ERR_FAIL_COND(colors.size() != static_cast<int>(_colors.size()));
	for (unsigned int i = 0; i < _colors.size(); ++i) {
		_colors[i] = Color8(colors[i]);
	}
}

void VoxelColorPalette::clear() {
	for (size_t i = 0; i < _colors.size(); ++i) {
		_colors[i] = Color8();
	}
}

PackedInt32Array VoxelColorPalette::_b_get_data() const {
	PackedInt32Array colors;
	// 不预先调整大小。
	for (size_t i = 0; i < _colors.size(); ++i) {
		colors.push_back(_colors[i].to_u32());
	}
	return colors;
}

void VoxelColorPalette::_b_set_data(PackedInt32Array colors) {
	ERR_FAIL_COND(colors.size() > static_cast<int>(_colors.size()));
	for (int i = 0; i < colors.size(); ++i) {
		_colors[i] = Color8::from_u32(colors[i]);
	}
}

void VoxelColorPalette::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_color", "index", "color"), &VoxelColorPalette::set_color);
	ClassDB::bind_method(D_METHOD("get_color", "index"), &VoxelColorPalette::get_color);

	ClassDB::bind_method(D_METHOD("set_colors", "colors"), &VoxelColorPalette::set_colors);
	ClassDB::bind_method(D_METHOD("get_colors"), &VoxelColorPalette::get_colors);

	ClassDB::bind_method(D_METHOD("set_data", "d"), &VoxelColorPalette::_b_set_data);
	ClassDB::bind_method(D_METHOD("get_data"), &VoxelColorPalette::_b_get_data);

	// 这只是为了允许在编辑器中编辑颜色
	ADD_PROPERTY(
			PropertyInfo(Variant::PACKED_COLOR_ARRAY, "colors", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR),
			"set_colors",
			"get_colors"
	);

	ADD_PROPERTY(
			PropertyInfo(Variant::PACKED_INT32_ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
			"set_data",
			"get_data"
	);

	BIND_CONSTANT(MAX_COLORS);
}

} // namespace voxel
