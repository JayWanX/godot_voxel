#ifndef VOXEL_TRANSVOXEL_MATERIALS_MIXEL4_H
#define VOXEL_TRANSVOXEL_MATERIALS_MIXEL4_H

#include "../../storage/mixel4.h"
#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_vector.h"
#include "transvoxel.h"
#include "transvoxel_materials_common.h"

namespace voxel::transvoxel::materials::mixel4 {

// 总共可以引用的纹理数量
static const unsigned int MAX_TEXTURES = 16;
// 一次可以混合的纹理数量
static const unsigned int MAX_TEXTURE_BLENDS = 4;

template <unsigned int NVoxels>
struct CellTextureDatas {
	uint32_t packed_indices = 0;
	FixedArray<uint8_t, MAX_TEXTURE_BLENDS> indices;
	FixedArray<FixedArray<uint8_t, MAX_TEXTURE_BLENDS>, NVoxels> weights;
};

template <unsigned int NVoxels, typename WeightSampler_T>
CellTextureDatas<NVoxels> select_textures_4_per_voxel(
		const FixedArray<unsigned int, NVoxels> &voxel_indices,
		const Span<const uint16_t> indices_data,
		const WeightSampler_T &weights_sampler,
		const unsigned int case_code
) {
	// TODO 优化：此函数在多边形化非空单元时几乎占用一半的时间。
	// 我想知道还能如何进一步优化？

	struct IndexAndWeight {
		unsigned int index;
		unsigned int weight;
	};

	FixedArray<FixedArray<uint8_t, MAX_TEXTURES>, NVoxels> cell_texture_weights_temp;
	FixedArray<IndexAndWeight, MAX_TEXTURES> indexed_weight_sums;

	// 找出体素中使用最多的 4 个索引
	for (unsigned int i = 0; i < indexed_weight_sums.size(); ++i) {
		indexed_weight_sums[i] = IndexAndWeight{ i, 0 };
	}
	for (unsigned int ci = 0; ci < voxel_indices.size(); ++ci) {
		// VOXEL_PROFILE_SCOPE();

		FixedArray<uint8_t, MAX_TEXTURES> &weights_temp = cell_texture_weights_temp[ci];
		fill(weights_temp, uint8_t(0));

		// 空气体素不应参与贡献
		if ((case_code & (1 << ci)) != 0) {
			continue;
		}

		const unsigned int data_index = voxel_indices[ci];

		const FixedArray<uint8_t, 4> indices =
				voxel::mixel4::decode_indices_from_packed_u16(indices_data[data_index]);
		const FixedArray<uint8_t, 4> weights = weights_sampler.get_weights(data_index);

		for (unsigned int j = 0; j < indices.size(); ++j) {
			const unsigned int ti = indices[j];
			indexed_weight_sums[ti].weight += weights[j];
			weights_temp[ti] = weights[j];
		}
	}
	struct IndexAndWeightComparator {
		inline bool operator()(const IndexAndWeight &a, const IndexAndWeight &b) const {
			return a.weight > b.weight;
		}
	};
	SortArray<IndexAndWeight, IndexAndWeightComparator> sorter;
	sorter.sort(indexed_weight_sums.data(), indexed_weight_sums.size());

	CellTextureDatas<NVoxels> cell_textures;

	// 分配索引
	for (unsigned int i = 0; i < cell_textures.indices.size(); ++i) {
		cell_textures.indices[i] = indexed_weight_sums[i].index;
	}

	// 对索引排序以避免混合时出现歧义的情况，如 1,2,3,4 和 2,1,3,4
	// TODO 也许我们可以要求预先完成这种排序？
	// 或者也可以在网格化之后进行，这样次数更少？
	math::sort(cell_textures.indices[0], cell_textures.indices[1], cell_textures.indices[2], cell_textures.indices[3]);

	cell_textures.packed_indices = pack_bytes(cell_textures.indices);

	// 重新映射权重以匹配我们选择的索引
	for (unsigned int ci = 0; ci < cell_texture_weights_temp.size(); ++ci) {
		// VOXEL_PROFILE_SCOPE();

		FixedArray<uint8_t, 4> &dst_weights = cell_textures.weights[ci];

		// 跳过空气体素
		if ((case_code & (1 << ci)) != 0) {
			fill(dst_weights, uint8_t(0));
			continue;
		}

		const FixedArray<uint8_t, MAX_TEXTURES> &src_weights = cell_texture_weights_temp[ci];

		for (unsigned int i = 0; i < cell_textures.indices.size(); ++i) {
			const unsigned int ti = cell_textures.indices[i];
			dst_weights[i] = src_weights[ti];
		}
	}

	return cell_textures;
}

struct TextureIndicesData {
	// 每个体素的纹理索引
	Span<const uint16_t> buffer;
	// 当缓冲区为空时使用
	FixedArray<uint8_t, 4> default_indices;
	uint32_t packed_default_indices;
};

template <unsigned int NVoxels, typename WeightSampler_T>
inline void get_cell_texture_data(
		CellTextureDatas<NVoxels> &cell_textures,
		const TextureIndicesData &texture_indices_data,
		const FixedArray<unsigned int, NVoxels> &voxel_indices,
		const WeightSampler_T &weights_data,
		// 用于剔除空气体素。可设为 0 以始终使用所有角。
		const unsigned int case_code
) {
	if (texture_indices_data.buffer.size() == 0) {
		// 整个数据块的索引已知，直接读取权重即可
		cell_textures.indices = texture_indices_data.default_indices;
		cell_textures.packed_indices = texture_indices_data.packed_default_indices;
		for (unsigned int ci = 0; ci < voxel_indices.size(); ++ci) {
			if ((case_code & (1 << ci)) != 0) {
				// 强制空气体素不参与贡献
				// TODO 这不太好，因为每个 Transvoxel 顶点都在实体与空气角之间插值。
				// 这意味着结果总是会向 0 插值。
				// 也许将来我们得采用不同的方法并移除该选项。
				fill(cell_textures.weights[ci], uint8_t(0));
			} else {
				const unsigned int wi = voxel_indices[ci];
				cell_textures.weights[ci] = weights_data.get_weights(wi);
			}
		}

	} else {
		// 索引可能多于 4 个或未知，因此我们必须自行选择
		cell_textures =
				select_textures_4_per_voxel(voxel_indices, texture_indices_data.buffer, weights_data, case_code);
	}
}

struct WeightSamplerPackedU16 {
	Span<const uint16_t> u16_data;
	inline FixedArray<uint8_t, 4> get_weights(unsigned int i) const {
		return voxel::mixel4::decode_weights_from_packed_u16(u16_data[i]);
	}
};

inline uint16_t reorder_transition_case_code(const uint16_t case_code) {
	// 重新排列过渡单元的 case code，使位与遍历单元角的 XYZ 迭代顺序对应。
	//
	// 过渡单元中选择的角顺序取决于 Transvoxel 表的布局（参见
	// 论文中的图 4.16 和 4.17），遗憾的是这与我们采样体素的顺序不同。
	// 这导致无法在纹理选择中复用它。选择该顺序的原因似乎是为了方便
	// 创建查找表。
	//                       |436785210|
	const uint16_t alt_case_code = 0 //
			| ((case_code & 0b000000111)) // 210
			| ((case_code & 0b110000000) >> 4) // 43
			| ((case_code & 0b000001000) << 2) // 5
			| ((case_code & 0b001000000)) // 6
			| ((case_code & 0b000100000) << 2) // 7
			| ((case_code & 0b000010000) << 4) // 8
			;
	// uint16_t alt_case_code = sign_f(cell_samples[0]);
	// alt_case_code |= (sign_f(cell_samples[1]) << 1);
	// alt_case_code |= (sign_f(cell_samples[2]) << 2);
	// alt_case_code |= (sign_f(cell_samples[3]) << 3);
	// alt_case_code |= (sign_f(cell_samples[4]) << 4);
	// alt_case_code |= (sign_f(cell_samples[5]) << 5);
	// alt_case_code |= (sign_f(cell_samples[6]) << 6);
	// alt_case_code |= (sign_f(cell_samples[7]) << 7);
	// alt_case_code |= (sign_f(cell_samples[8]) << 8);

	return alt_case_code;
}

template <unsigned int NVoxels>
struct Processor {
	const TextureIndicesData voxel_material_indices;
	const WeightSamplerPackedU16 voxel_material_weights;
	const bool textures_skip_air_voxels;
	CellTextureDatas<NVoxels> cell_textures;
	StdVector<Vector2f> &output_mesh_material_data;

	Processor(
			const TextureIndicesData p_voxel_material_indices,
			const WeightSamplerPackedU16 p_voxel_material_weights,
			StdVector<Vector2f> &p_output_mesh_material_data,
			const bool p_textures_skip_air_voxels
	) :
			voxel_material_indices(p_voxel_material_indices),
			voxel_material_weights(p_voxel_material_weights),
			textures_skip_air_voxels(p_textures_skip_air_voxels),
			output_mesh_material_data(p_output_mesh_material_data) {}

	inline uint32_t on_cell(const FixedArray<uint32_t, NVoxels> &corner_voxel_indices, const uint8_t case_code) {
		get_cell_texture_data(
				cell_textures,
				voxel_material_indices,
				corner_voxel_indices,
				voxel_material_weights,
				textures_skip_air_voxels ? case_code : 0
		);

		return cell_textures.packed_indices;
	}

	inline uint32_t on_transition_cell(const FixedArray<uint32_t, 9> &corner_voxel_indices, const uint8_t case_code) {
		const uint16_t alt_case_code = textures_skip_air_voxels ? reorder_transition_case_code(case_code) : 0;

		// 从 9 个关键角获取值
		CellTextureDatas<9> cell_textures_partial;
		get_cell_texture_data(
				cell_textures_partial,
				voxel_material_indices,
				corner_voxel_indices,
				voxel_material_weights,
				alt_case_code
		);

		// 填充只是重复其它槽位的值

		cell_textures.indices = cell_textures_partial.indices;
		cell_textures.packed_indices = cell_textures_partial.packed_indices;

		fill_redundant_transition_cell_values(cell_textures_partial.weights, cell_textures.weights);

		return cell_textures.packed_indices;
	}

	inline void on_vertex(const unsigned int v0, const unsigned int v1, const float alpha) {
		const FixedArray<uint8_t, MAX_TEXTURE_BLENDS> weights0 = cell_textures.weights[v0];
		const FixedArray<uint8_t, MAX_TEXTURE_BLENDS> weights1 = cell_textures.weights[v1];
		FixedArray<uint8_t, MAX_TEXTURE_BLENDS> weights;
		for (unsigned int i = 0; i < MAX_TEXTURE_BLENDS; ++i) {
			weights[i] = static_cast<uint8_t>(math::clamp(Math::lerp(weights0[i], weights1[i], alpha), 0.f, 255.f));
		}
		add_4i8_4w8_texture_data(output_mesh_material_data, cell_textures.packed_indices, weights);
	}
};

TextureIndicesData get_texture_indices_data(
		const VoxelBuffer &voxels,
		const unsigned int indices_channel,
		DefaultTextureIndicesData &out_default_texture_indices_data
) {
	VOXEL_ASSERT_RETURN_V(voxels.get_channel_depth(indices_channel) == VoxelBuffer::DEPTH_16_BIT, TextureIndicesData());

	TextureIndicesData data;

	if (voxels.is_uniform(indices_channel)) {
		const uint16_t encoded_indices = voxels.get_voxel(Vector3i(), indices_channel);
		data.default_indices = voxel::mixel4::decode_indices_from_packed_u16(encoded_indices);
		data.packed_default_indices = pack_bytes(data.default_indices);

		out_default_texture_indices_data.indices = data.default_indices;
		out_default_texture_indices_data.packed_indices = data.packed_default_indices;
		out_default_texture_indices_data.use = true;

	} else {
		Span<const uint8_t> data_bytes;
		VOXEL_ASSERT(voxels.get_channel_as_bytes_read_only(indices_channel, data_bytes) == true);
		data.buffer = data_bytes.reinterpret_cast_to<const uint16_t>();

		out_default_texture_indices_data.use = false;
	}

	return data;
}

} // namespace voxel::transvoxel::materials::mixel4

#endif // VOXEL_TRANSVOXEL_MATERIALS_MIXEL4_H
