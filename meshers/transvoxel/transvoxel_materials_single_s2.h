#ifndef VOXEL_TRANSVOXEL_MATERIALS_SINGLE_S2_H
#define VOXEL_TRANSVOXEL_MATERIALS_SINGLE_S2_H

#include "../../util/math/funcs.h"
#include "transvoxel.h"
#include "transvoxel_materials_single_common.h"
#include <array>

namespace voxel::transvoxel::materials::single::s2 {

// 每个体素一个 8 位材质。着色器中最多混合 4 个。

template <unsigned int NVoxels>
struct CellMaterials {
	// 单元内将混合的 4 种最具代表性材质的选中索引
	std::array<uint8_t, 2> selected_indices;
	uint16_t packed_indices = 0;
	// 单元中每个体素处 2 种已选材质之一的索引
	std::array<uint8_t, NVoxels> component_indices;
};

inline uint16_t pack_bytes(std::array<uint8_t, 2> a) {
	return //
			(static_cast<uint16_t>(a[0]) << 0) | //
			(static_cast<uint16_t>(a[1]) << 8);
}

// 将新项插入到已排序项的固定集合中。如果放不下，则不插入。如果放得下，最后一个
// 项将被驱逐以腾出空间。
void insert_sort(std::array<WeightedIndex, 2> &sorted_items, const WeightedIndex new_item) {
	if (new_item.weight > sorted_items[0].weight) {
		sorted_items[1] = sorted_items[0];
		sorted_items[0] = new_item;
	} else if (new_item.weight > sorted_items[1].weight) {
		sorted_items[1] = new_item;
	}
}

inline uint8_t index_of_or_zero(std::array<uint8_t, 2> haystack, uint8_t needle) {
	return haystack[1] == needle ? 1 : 0;
}

template <unsigned int NVoxels>
inline void assign_component_indices(
		const std::array<uint8_t, 2> available_material_indices,
		const std::array<uint8_t, NVoxels> cell_voxel_material_indices,
		std::array<uint8_t, NVoxels> &component_indices
) {
	for (unsigned int i = 0; i < component_indices.size(); ++i) {
		const uint8_t mi = cell_voxel_material_indices[i];
		// 挑选对应的材质。如果该材质未被选中用于混合，0 将回退到
		// 最具代表性的材质。
		component_indices[i] = index_of_or_zero(available_material_indices, mi);
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
		// 填入 2 个不同的索引，但只有一个具有完整权重。
		for (uint8_t i = 1; i < cell.selected_indices.size(); ++i) {
			cell.selected_indices[i] = (material_index + i) & 255;
		}

		for (unsigned int i = 0; i < cell.component_indices.size(); ++i) {
			cell.component_indices[i] = 0;
		}

	} else {
		// 单元中最多可能有 NVoxels 种不同的材质，所以创建一个能容纳这么多的小数组
		std::array<WeightedIndex, NVoxels> distinct_materials;

		// TODO 这是毫无意义的初始化，但没有它编译器会抱怨...
		// 编译器看不到我们处理的每种情况都保证前 N 项由
		// `insert_combine` 初始化，因此会发出“可能使用未初始化”的警告，
		// 该警告被视为错误并阻止编译。这是浪费的周期。我想知道是否有更好的替代方案
		// 不涉及抑制该警告。
		for (WeightedIndex &wi : distinct_materials) {
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
		uint32_t distinct_material_count = 0;
		for (const uint8_t mi : cell_voxel_material_indices) {
			insert_combine(distinct_materials, distinct_material_count, mi);
		}

		// 选出 4 种最具代表性的材质
		switch (distinct_material_count) {
				// case 0:
				// 不应发生？

			case 1: {
				// 整个单元使用同一种材质。
				// 可能是最常见的情况。
				const uint8_t i0 = distinct_materials[0].index;
				// 第一个分量是选中的材质
				cell.selected_indices[0] = i0;
				// 在其他分量中填入不同的材质作为占位符
				cell.selected_indices[1] = (i0 + 1) & 0xff;
				// 整个单元使用分量 0
				for (unsigned int i = 0; i < cell.component_indices.size(); ++i) {
					cell.component_indices[i] = 0;
				}
			} break;

			case 2: {
				// 可能是第二常见的情况。
				// 将不同的材质分量按最具代表性排序。
				math::sort2_array(distinct_materials, WeightedIndex::compare_higher_weight);
				for (unsigned int i = 0; i < cell.selected_indices.size(); ++i) {
					cell.selected_indices[i] = distinct_materials[i].index;
				}
				assign_component_indices(cell.selected_indices, cell_voxel_material_indices, cell.component_indices);
			} break;

			case 3: {
				// 多于 2 种材质，权重最低的将不会被保留
				math::sort3_array(distinct_materials, WeightedIndex::compare_higher_weight);
				for (unsigned int i = 0; i < cell.selected_indices.size(); ++i) {
					cell.selected_indices[i] = distinct_materials[i].index;
				}
				assign_component_indices(cell.selected_indices, cell_voxel_material_indices, cell.component_indices);
			} break;

			case 4: {
				math::sort4_array(distinct_materials, WeightedIndex::compare_higher_weight);
				for (unsigned int i = 0; i < cell.selected_indices.size(); ++i) {
					cell.selected_indices[i] = distinct_materials[i].index;
				}
				assign_component_indices(cell.selected_indices, cell_voxel_material_indices, cell.component_indices);
			} break;

			default: {
				// 多于 4 种材质。
				// 通用排序。应该很少见。
				std::array<WeightedIndex, 2> selected_distinct_materials;
				for (WeightedIndex &it : selected_distinct_materials) {
					it = { 0, 0 };
				}
				for (const WeightedIndex wi : distinct_materials) {
					insert_sort(selected_distinct_materials, wi);
				}
				for (unsigned int i = 0; i < cell.selected_indices.size(); ++i) {
					cell.selected_indices[i] = selected_distinct_materials[i].index;
				}
				assign_component_indices(cell.selected_indices, cell_voxel_material_indices, cell.component_indices);
			} break;
		}
	}

	cell.packed_indices = pack_bytes(cell.selected_indices);
}

inline void add_2i8_1w8_texture_data(StdVector<float> &dst, const uint16_t packed_indices, const uint8_t weight) {
	union Reinterpreter {
		uint32_t i;
		float f;
	};
	Reinterpreter u;
	u.i = static_cast<uint32_t>(packed_indices) | (static_cast<uint32_t>(weight) << 16);
	dst.push_back(u.f);
}

template <unsigned int NVoxels>
struct Processor {
	const Span<const uint8_t> voxel_material_indices;
	CellMaterials<NVoxels> cell;
	StdVector<float> &output_mesh_material_data;

	Processor(
			Span<const uint8_t> p_voxel_material_indices,
			StdVector<float> &p_output_mesh_material_data
	) :
			voxel_material_indices(p_voxel_material_indices), //
			output_mesh_material_data(p_output_mesh_material_data) //
	{}

	inline uint32_t on_cell(const FixedArray<uint32_t, NVoxels> &corner_voxel_indices, const uint8_t case_code) {
		get_cell_materials<NVoxels>(voxel_material_indices, corner_voxel_indices, cell);
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
		std::array<float, 2> weights_f{ 0.f, 0.f };
		const uint8_t c0 = cell.component_indices[v0];
		const uint8_t c1 = cell.component_indices[v1];
		weights_f[c0] += 1.f - alpha;
		weights_f[c1] += alpha;
		const uint8_t weight = static_cast<uint8_t>(math::clamp(weights_f[1] * 255.f, 0.f, 255.f));
		add_2i8_1w8_texture_data(output_mesh_material_data, cell.packed_indices, weight);
	}
};

} // namespace voxel::transvoxel::materials::single::s2

#endif // VOXEL_TRANSVOXEL_MATERIALS_SINGLE_S2_H
