#ifndef VOXEL_TRANSVOXEL_MATERIALS_COMMON_H
#define VOXEL_TRANSVOXEL_MATERIALS_COMMON_H

#include "transvoxel.h"

namespace voxel::transvoxel::materials {

inline uint32_t pack_bytes(const FixedArray<uint8_t, 4> &a) {
	return (a[0] | (a[1] << 8) | (a[2] << 16) | (a[3] << 24));
}

inline void add_4i8_4w8_texture_data(
		StdVector<Vector2f> &uv,
		const uint32_t packed_indices,
		const FixedArray<uint8_t, 4> weights
) {
	struct IntUV {
		uint32_t x;
		uint32_t y;
	};
	static_assert(sizeof(IntUV) == sizeof(Vector2f), "Expected same binary size");
	uv.push_back(Vector2f());
	IntUV &iuv = *(reinterpret_cast<IntUV *>(&uv.back()));
	// print_line(String("{0}, {1}, {2}, {3}").format(varray(weights[0], weights[1], weights[2], weights[3])));
	iuv.x = packed_indices;
	iuv.y = pack_bytes(weights);
}

// 过渡单元在一侧包含 2x2 个值，在另一侧包含 3x3 个值。
// 2x2 值的那一侧只是重复 3x3 一侧角上的值。
// 此函数从 3x3 “部分”一侧填充所有值的数组，并补齐冗余的 2x2 值。
template <typename TPartialArray9, typename TFullArray13>
void fill_redundant_transition_cell_values(const TPartialArray9 &src, TFullArray13 &dst) {
	for (unsigned int i = 0; i < src.size(); ++i) {
		dst[i] = src[i];
	}
	dst[0x9] = src[0];
	dst[0xA] = src[2];
	dst[0xB] = src[6];
	dst[0xC] = src[8];
}

} // namespace voxel::transvoxel::materials

#endif // VOXEL_TRANSVOXEL_MATERIALS_COMMON_H
