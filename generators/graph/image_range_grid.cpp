#include "image_range_grid.h"
#include "../../util/godot/classes/image.h"
#include "../../util/string/format.h"
#include "image_utility.h"

namespace voxel {

using namespace math;

ImageRangeGrid::~ImageRangeGrid() {
	clear();
}

void ImageRangeGrid::clear() {
	_pixels_x = 0;
	_pixels_y = 0;
	_lod_base = 0;
	_lod_count = 0;
}

void ImageRangeGrid::generate(const Image &im) {
	VOXEL_ASSERT_RETURN_MSG(!im.is_compressed(), format("Image format not supported: {}", im.get_format()));

	clear();

	if (im.is_empty()) {
		return;
	}

	const int lod_base = 4; // 从 16 开始

	// 计算第一个 LOD
	{
		const int chunk_size = 1 << lod_base;

		Lod &lod = _lods[0];
		lod.size_x = math::ceildiv(im.get_width(), chunk_size);
		lod.size_y = math::ceildiv(im.get_height(), chunk_size);
		lod.data.resize(lod.size_x * lod.size_y);
		lod.data.shrink_to_fit();

		for (int cy = 0; cy < lod.size_y; ++cy) {
			for (int cx = 0; cx < lod.size_x; ++cx) {
				const int min_x = cx * chunk_size;
				const int min_y = cy * chunk_size;
				const int max_x = min(min_x + chunk_size, im.get_width());
				const int max_y = min(min_y + chunk_size, im.get_height());

				const Interval r = voxel::get_heightmap_range(im, Rect2i(min_x, min_y, max_x - min_x, max_y - min_y));

				lod.data[cx + cy * lod.size_x] = r;
			}
		}
	}

	int lod_count = 1;

	// 基于前一个 LOD 计算后续 LOD
	for (int lod_index = 1; lod_index < MAX_LODS; ++lod_index, ++lod_count) {
		const Lod &prev_lod = _lods[lod_index - 1];

		if (prev_lod.size_x == 1 && prev_lod.size_y == 1) {
			// 无法继续缩小
			break;
		}

		Lod &lod = _lods[lod_index];

		lod.size_x = max(ceildiv(prev_lod.size_x, 2), 1);
		lod.size_y = max(ceildiv(prev_lod.size_y, 2), 1);

		lod.data.resize(lod.size_x * lod.size_y);
		lod.data.shrink_to_fit();

		int dst_i = 0;

		for (int cy = 0; cy < lod.size_y; ++cy) {
			const int src_y = min(cy * 2, prev_lod.size_y);

			for (int cx = 0; cx < lod.size_x; ++cx) {
				const int src_x = min(cx * 2, prev_lod.size_x);
				// 当前块所覆盖的 2x2 区域中，前一个 LOD 内角落块的索引
				const int src_i = src_x + src_y * prev_lod.size_x;

				// 如果可用，累加当前块所覆盖的前一个 LOD 中的 2x2 块

				// X, Y
				Interval r = prev_lod.data[src_i];

				// X+1, Y
				if (src_x + 1 < prev_lod.size_x) {
					r.add_interval(prev_lod.data[src_i + 1]);
				}
				// X, Y+1
				if (src_y + 1 < prev_lod.size_y) {
					r.add_interval(prev_lod.data[src_i + prev_lod.size_x]);
				}
				// X+1, Y+1
				if (src_x + 1 < prev_lod.size_x && src_y + 1 < prev_lod.size_y) {
					r.add_interval(prev_lod.data[src_i + prev_lod.size_x + 1]);
				}

				lod.data[dst_i++] = r;
			}
		}
	}

	{
		VOXEL_ASSERT(lod_count > 0);
		const int last_lod_index = lod_count - 1;
		const Lod &lod = _lods[last_lod_index];
		Interval r = lod.data[0];
		for (int cy = 0; cy < lod.size_y; ++cy) {
			for (int cx = 0; cx < lod.size_x; ++cx) {
				r.add_interval(lod.data[cx + cy * lod.size_x]);
			}
		}
		_total_range = r;
	}

	_pixels_x = im.get_width();
	_pixels_y = im.get_height();
	_pixels_x_is_power_of_2 = math::is_power_of_two(_pixels_x);
	_pixels_y_is_power_of_2 = math::is_power_of_two(_pixels_y);
	_lod_base = lod_base;
	_lod_count = lod_count;
}

namespace {

void interval_to_pixels_repeat(Interval i, int &out_min, int &out_max, int image_len) {
	// 将范围转换到正整数坐标空间，
	// 其中坐标最多延伸到图像长度的两倍。
	// 假设图像重复，区间会在该空间内被环绕。

	int imin = static_cast<int>(Math::floor(i.min));
	int imax = static_cast<int>(Math::ceil(i.max));

	const int interval_len = imax - imin;

	if (interval_len >= image_len) {
		// 区间覆盖了整个长度
		out_min = 0;
		out_max = image_len;
		return;
	}

	imin = math::wrap(imin, image_len);
	imax = math::wrap(imax, image_len);

	if (imin > imax) {
		imax = imin + interval_len;
	}

	// 保持为正
	if (imin < 0) {
		imin += image_len;
		imax += image_len;
	}

	out_min = imin;
	out_max = imax;
}

} // namespace

Interval ImageRangeGrid::get_range_repeat(Interval xr, Interval yr) const {
	VOXEL_ASSERT(_lod_count > 0);

	int pixel_min_x, pixel_max_x, pixel_min_y, pixel_max_y;
	interval_to_pixels_repeat(xr, pixel_min_x, pixel_max_x, _pixels_x);
	interval_to_pixels_repeat(yr, pixel_min_y, pixel_max_y, _pixels_y);

	// 寻找要使用的最佳 LOD。
	// 根据最大范围的长度，我们可能会评估不同的 LOD 以节省迭代次数
	int lod_index = 0; // 相对于 _lod_base
	{
		int pixel_len = max(pixel_max_x - pixel_min_x, pixel_max_y - pixel_min_y);
		const int cs = 1 << _lod_base;
		const int max_overlapping_chunks = 2;
		while (pixel_len > cs * max_overlapping_chunks && lod_index + 1 < _lod_count) {
			++lod_index;
			pixel_len >>= 1;
		}
	}

	VOXEL_ASSERT(lod_index < _lod_count);

	// 计算块区域
	const int absolute_lod = _lod_base + lod_index;
	const int chunk_x_min = arithmetic_rshift(pixel_min_x, absolute_lod);
	const int chunk_y_min = arithmetic_rshift(pixel_min_y, absolute_lod);
	int chunk_x_max = arithmetic_rshift(pixel_max_x, absolute_lod);
	int chunk_y_max = arithmetic_rshift(pixel_max_y, absolute_lod);

	// 如果图像大小不是 2 的幂，且区间跨越了图像的重复边界，我们不得不
	// 再向远处查找一个数据块，因为数据块按图像的向上取整大小平铺，而最后一个数据块
	// 实际上更短，因此图像的下一次重复会更早开始。
	// 注意：`pixel_min_x` 不可能 >= `pixels_x`，因为一旦达到该值，
	// 我们就会将其回绕到图像的开头。
	if (!_pixels_x_is_power_of_2 && pixel_max_x >= _pixels_x) {
		++chunk_x_max;
	}
	if (!_pixels_y_is_power_of_2 && pixel_max_y >= _pixels_y) {
		++chunk_y_max;
	}

	const Lod &lod = _lods[lod_index];

	// 累加重叠的块
	Interval r;
	{
		const unsigned int loc = math::wrap(chunk_x_min, lod.size_x) + math::wrap(chunk_y_min, lod.size_y) * lod.size_x;
		r = lod.data[loc];
	}
	for (int chunk_y = chunk_y_min; chunk_y <= chunk_y_max; ++chunk_y) {
		for (int chunk_x = chunk_x_min; chunk_x <= chunk_x_max; ++chunk_x) {
			const unsigned int loc = math::wrap(chunk_x, lod.size_x) + math::wrap(chunk_y, lod.size_y) * lod.size_x;
			r.add_interval(lod.data[loc]);
		}
	}

	return r;
}

} // namespace voxel
