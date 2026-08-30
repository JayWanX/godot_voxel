#ifndef VOXEL_COLOR_PALETTE_H
#define VOXEL_COLOR_PALETTE_H

#include "../../util/containers/fixed_array.h"
#include "../../util/godot/classes/resource.h"
#include "../../util/math/color8.h"

namespace voxel {

// 将小数字与颜色关联起来，这样彩色体素可以用更少的内存来表示。
class VoxelColorPalette : public Resource {
	GDCLASS(VoxelColorPalette, Resource)
public:
	static const unsigned int MAX_COLORS = 256;

	VoxelColorPalette();

	// 设置或获取指定索引的颜色
	void set_color(int index, Color color);
	Color get_color(int index) const;

	// 获取或设置全部颜色
	PackedColorArray get_colors() const;
	void set_colors(PackedColorArray colors);

	// 清空调色板
	void clear();

	// 内部

	// 以紧凑格式设置或获取颜色
	inline void set_color8(uint8_t i, Color8 c) {
		_colors[i] = c;
	}

	inline Color8 get_color8(uint8_t i) const {
		return _colors[i];
	}

private:
	PackedInt32Array _b_get_data() const;
	void _b_set_data(PackedInt32Array colors);

	static void _bind_methods();

	FixedArray<Color8, MAX_COLORS> _colors;
};

} // namespace voxel

#endif // VOXEL_COLOR_PALETTE_H
