#ifndef VOXEL_TRANSVOXEL_MATERIALS_NULL_H
#define VOXEL_TRANSVOXEL_MATERIALS_NULL_H

#include "transvoxel.h"

namespace voxel::transvoxel::materials {

struct NullProcessor {
	// 对每个包含三角形的 2x2x2 单元调用。
	// 返回的值用于确定下一个单元是否可以在相等时复用前面单元的顶点。
	inline uint32_t on_cell(const FixedArray<uint32_t, 8> &corner_voxel_indices, const uint8_t case_code) const {
		return 0;
	}
	// 对每个包含三角形的 2x3x3 过渡单元调用。
	// 此类单元在数据上实际上是二维的，因此角点值相同，所以只传入 9 个。
	// 返回的值用于确定下一个单元是否可以在相等时复用前面单元的顶点。
	inline uint32_t on_transition_cell(const FixedArray<uint32_t, 9> &corner_voxel_indices, const uint8_t case_code)
			const {
		return 0;
	}
	// 在每个 `on_cell` 之后为每个新顶点调用一次或多次，用于插值并添加材质数据
	inline void on_vertex(const unsigned int v0, const unsigned int v1, const float alpha) const {
		return;
	}
};

} // namespace voxel::transvoxel::materials

#endif // VOXEL_TRANSVOXEL_MATERIALS_NULL_H
