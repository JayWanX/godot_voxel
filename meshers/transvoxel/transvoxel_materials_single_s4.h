#ifndef VOXEL_TRANSVOXEL_MATERIALS_SINGLE_S4_H
#define VOXEL_TRANSVOXEL_MATERIALS_SINGLE_S4_H

#include "../../util/math/funcs.h"
#include "transvoxel.h"
#include "transvoxel_materials_single_common.h"
#include <array>

namespace voxel::transvoxel::materials::single::s4 {

// 每个体素一个 8 位材质。着色器中最多混合 4 个。

template <unsigned int NVoxels>
struct CellMaterials {
	// 单元内将混合的 4 种最具代表性材质的选中索引
	std::array<uint8_t, 4> selected_indices;
	uint32_t packed_indices = 0;
	// 单元中每个体素处 4 种已选材质之一的索引
	std::array<uint8_t, NVoxels> component_indices;
};

inline uint32_t pack_bytes(std::array<uint8_t, 4> a) {
	return //
			(static_cast<uint32_t>(a[0]) << 0) | //
			(static_cast<uint32_t>(a[1]) << 8) | //
			(static_cast<uint32_t>(a[2]) << 16) | //
			(static_cast<uint32_t>(a[3]) << 24);
}

// 将新项插入到已排序项的固定集合中。如果放不下，则不插入。如果放得下，最后一个
// 项将被驱逐以腾出空间。
void insert_sort(std::array<WeightedIndex, 4> &sorted_items, const WeightedIndex new_item) {
	if (new_item.weight > sorted_items[0].weight) {
		sorted_items[3] = sorted_items[2];
		sorted_items[2] = sorted_items[1];
		sorted_items[1] = sorted_items[0];
		sorted_items[0] = new_item;
	} else if (new_item.weight > sorted_items[1].weight) {
		sorted_items[3] = sorted_items[2];
		sorted_items[2] = sorted_items[1];
		sorted_items[1] = new_item;
	} else if (new_item.weight > sorted_items[2].weight) {
		sorted_items[3] = sorted_items[2];
		sorted_items[2] = new_item;
	} else if (new_item.weight > sorted_items[3].weight) {
		sorted_items[3] = new_item;
	}
}

template <unsigned int N>
uint8_t index_of_or_zero(const std::array<uint8_t, 4> haystack, const uint8_t needle) {
	for (unsigned int i = 0; i < N; ++i) {
		if (haystack[i] == needle) {
			return i;
		}
	}
	return 0;
}

template <unsigned int NVoxels, unsigned int NMaterials>
inline void assign_component_indices(
		const std::array<uint8_t, 4> available_material_indices,
		const std::array<uint8_t, NVoxels> cell_voxel_material_indices,
		std::array<uint8_t, NVoxels> &component_indices
) {
	for (unsigned int i = 0; i < component_indices.size(); ++i) {
		const uint8_t mi = cell_voxel_material_indices[i];
		// 挑选对应的材质。如果该材质未被选中用于混合，0 将回退到
		// 最具代表性的材质。
		component_indices[i] = index_of_or_zero<NMaterials>(available_material_indices, mi);
	}
}

template <unsigned int NVoxels>
void get_cell_materials(
		const Span<const uint8_t> voxel_material_indices,
		const FixedArray<uint32_t, NVoxels> &voxel_indices,
		CellMaterials<NVoxels> &cell
) {
	if (voxel_material_indices.size() == 1) {
		// 数据块中的所有索引都相同
		const uint8_t material_index = voxel_material_indices[0];
		cell.selected_indices[0] = material_index;
		// 填入 4 个不同的索引，但只有一个具有完整权重。
		for (uint8_t i = 1; i < 4; ++i) {
			cell.selected_indices[i] = (material_index + i) & 255;
		}

		for (unsigned int i = 0; i < cell.component_indices.size(); ++i) {
			cell.component_indices[i] = 0;
		}

	} else {
		std::array<WeightedIndex, NVoxels> items;
		// TODO 这是毫无意义的初始化，但没有它编译器会抱怨...
		// 编译器看不到我们处理的每种情况都保证前 N 项由
		// `insert_combine` 初始化，因此会发出“可能使用未初始化”的警告，
		// 该警告被视为错误并阻止编译。这是浪费的周期。我想知道是否有更好的替代方案
		// 不涉及抑制该警告。
		for (WeightedIndex &wi : items) {
			wi = { 0, 0 };
		}

		// 查找体素
		std::array<uint8_t, NVoxels> cell_voxel_material_indices;
		for (unsigned int cvi = 0; cvi < voxel_indices.size(); ++cvi) {
			const uint32_t vi = voxel_indices[cvi];
			const uint8_t mi = voxel_material_indices[vi];
			cell_voxel_material_indices[cvi] = mi;
		}

		// 统计材质
		uint32_t item_count = 0;
		for (const uint8_t mi : cell_voxel_material_indices) {
			insert_combine(items, item_count, mi);
		}

		// 选出 4 种最具代表性的材质
		switch (item_count) {
				// case 0: 不应发生

			case 1: {
				// 整个单元使用同一种材质。
				// 可能是最常见的情况。
				const uint8_t i0 = items[0].index;
				cell.selected_indices[0] = i0;
				// 在其他槽位填入不同的材质作为占位符
				for (uint8_t i = 1; i < 4; ++i) {
					cell.selected_indices[i] = (i0 + i) & 0xff;
				}
				for (unsigned int i = 0; i < cell.component_indices.size(); ++i) {
					cell.component_indices[i] = 0;
				}
			} break;
			case 2: {
				// 可能是第二常见的情况。
				math::sort2_array(items, WeightedIndex::compare_higher_weight);
				for (unsigned int i = 0; i < 2; ++i) {
					cell.selected_indices[i] = items[i].index;
				}
				const uint8_t i0 = items[1].index;
				for (uint8_t i = 2; i < 4; ++i) {
					cell.selected_indices[i] = (i0 + i) & 0xff;
				}
				assign_component_indices<NVoxels, 2>(
						cell.selected_indices, cell_voxel_material_indices, cell.component_indices
				);
			} break;
			case 3: {
				math::sort3_array(items, WeightedIndex::compare_higher_weight);
				for (unsigned int i = 0; i < 3; ++i) {
					cell.selected_indices[i] = items[i].index;
				}
				const uint8_t i0 = items[2].index;
				cell.selected_indices[3] = (i0 + 1) & 0xff;
				assign_component_indices<NVoxels, 3>(
						cell.selected_indices, cell_voxel_material_indices, cell.component_indices
				);
			} break;
			case 4:
				math::sort4_array(items, WeightedIndex::compare_higher_weight);
				for (unsigned int i = 0; i < cell.selected_indices.size(); ++i) {
					cell.selected_indices[i] = items[i].index;
				}
				assign_component_indices<NVoxels, 4>(
						cell.selected_indices, cell_voxel_material_indices, cell.component_indices
				);
				break;
			default: {
				// 多于 4 种材质。
				// 通用排序。应该很少见。
				std::array<WeightedIndex, 4> selected_items;
				for (WeightedIndex &it : selected_items) {
					it = { 0, 0 };
				}
				for (const WeightedIndex wi : items) {
					insert_sort(selected_items, wi);
				}
				for (unsigned int i = 0; i < selected_items.size(); ++i) {
					cell.selected_indices[i] = selected_items[i].index;
				}
				assign_component_indices<NVoxels, 4>(
						cell.selected_indices, cell_voxel_material_indices, cell.component_indices
				);
			} break;
		}
	}

	cell.packed_indices = pack_bytes(cell.selected_indices);
}

template <unsigned int NVoxels>
struct Processor {
	const Span<const uint8_t> voxel_material_indices;
	CellMaterials<NVoxels> cell;
	StdVector<Vector2f> &output_mesh_material_data;

	Processor(
			Span<const uint8_t> p_voxel_material_indices,
			StdVector<Vector2f> &p_output_mesh_material_data
	) :
			voxel_material_indices(p_voxel_material_indices), //
			output_mesh_material_data(p_output_mesh_material_data) //
	{}

	inline uint32_t on_cell(const FixedArray<uint32_t, NVoxels> &corner_voxel_indices, const uint8_t case_code) {
		get_cell_materials<NVoxels>(voxel_material_indices, corner_voxel_indices, cell);
#ifdef TOOLS_ENABLED
		for (unsigned int i = 0; i < cell.component_indices.size(); ++i) {
			VOXEL_ASSERT(cell.component_indices[i] < cell.selected_indices.size());
		}
#endif
		return cell.packed_indices;
	}

	inline uint32_t on_transition_cell(const FixedArray<uint32_t, 9> &corner_voxel_indices, const uint8_t case_code) {
		// const uint16_t alt_case_code = textures_skip_air_voxels ? reorder_transition_case_code(case_code) : 0;

		// 从 9 个关键角获取值
		CellMaterials<9> cell_materials_partial;
		get_cell_materials<9>(voxel_material_indices, corner_voxel_indices, cell_materials_partial);

		// 填充只是重复其它槽位的值

		cell.selected_indices = cell_materials_partial.selected_indices;
		cell.packed_indices = cell_materials_partial.packed_indices;

		fill_redundant_transition_cell_values(cell_materials_partial.component_indices, cell.component_indices);

		return cell.packed_indices;
	}

	inline void on_vertex(const unsigned int v0, const unsigned int v1, const float alpha) {
		std::array<float, 4> weights_f{ 0.f, 0.f, 0.f, 0.f };
		const uint8_t c0 = cell.component_indices[v0];
		const uint8_t c1 = cell.component_indices[v1];
		weights_f[c0] += 1.f - alpha;
		weights_f[c1] += alpha;
		FixedArray<uint8_t, 4> weights;
		for (unsigned int i = 0; i < weights.size(); ++i) {
			weights[i] = static_cast<uint8_t>(math::clamp(weights_f[i] * 255.f, 0.f, 255.f));
		}
		add_4i8_4w8_texture_data(output_mesh_material_data, cell.packed_indices, weights);
	}
};

} // namespace voxel::transvoxel::materials::single::s4

#endif // VOXEL_TRANSVOXEL_MATERIALS_SINGLE_S4_H
