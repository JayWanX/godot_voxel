#ifndef IMAGE_RANGE_GRID_H
#define IMAGE_RANGE_GRID_H

#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/macros.h"
#include "../../util/math/interval.h"

VOXEL_GODOT_FORWARD_DECLARE(class Image)

namespace voxel {

// 在多个细节层级上存储 2D 图像的最小值和最大值
class ImageRangeGrid {
public:
	~ImageRangeGrid();

	void clear();
	void generate(const Image &im);
	inline math::Interval get_range() const {
		return _total_range;
	}

	// 快速获取图像某个区域内值的范围上界。如果区域超出图像边界，
	// 将按图像无限重复的方式进行求值。
	math::Interval get_range_repeat(math::Interval xr, math::Interval yr) const;

private:
	static const int MAX_LODS = 16;

	struct Lod {
		// 块的网格，包含每个块覆盖的所有像素的最小值和最大值
		StdVector<math::Interval> data;
		// 以块为单位
		int size_x = 0;
		int size_y = 0;
	};

	// 原始尺寸
	int _pixels_x = 0;
	int _pixels_y = 0;
	bool _pixels_x_is_power_of_2 = true;
	bool _pixels_y_is_power_of_2 = true;

	int _lod_base = 0;
	int _lod_count = 0;

	math::Interval _total_range;

	FixedArray<Lod, MAX_LODS> _lods;
};

} // namespace voxel

#endif // IMAGE_RANGE_GRID_H
