#ifndef VOXEL_MIXEL4_H
#define VOXEL_MIXEL4_H

#include "../util/containers/fixed_array.h"
#include "../util/math/funcs.h"
#include <cstdint>

// 使用 4 个索引和 4 个权重来编码、解码和混合体素材质的函数。

namespace voxel {

class VoxelBuffer;

namespace mixel4 {

inline FixedArray<uint8_t, 4> decode_weights_from_packed_u16(uint16_t packed_weights) {
	FixedArray<uint8_t, 4> weights;
	// 可 SIMD 化？
	// weights[0] = ((packed_weights >> 0) & 0x0f) << 4;
	// weights[1] = ((packed_weights >> 4) & 0x0f) << 4;
	// weights[2] = ((packed_weights >> 8) & 0x0f) << 4;
	// weights[3] = ((packed_weights >> 12) & 0x0f) << 4;

	// 已简化但不可 SIMD 化
	weights[0] = (packed_weights & 0x0f) << 4;
	weights[1] = packed_weights & 0xf0;
	weights[2] = (packed_weights >> 4) & 0xf0;
	weights[3] = (packed_weights >> 8) & 0xf0;
	// 上面的代码使得权重的最大 uint8_t 值为 240，而不是 255。
	// 我们可以增加额外的计算来完全匹配该范围，
	// 但作为折衷我没有这样做，因为它会破坏双射性并且更慢。
	// 如果这是个问题，那么它可能成为改用 3 个通道的 8 位表示的理由。
	// weights[0] |= weights[0] >> 4;
	// weights[1] |= weights[1] >> 4;
	// weights[2] |= weights[2] >> 4;
	// weights[3] |= weights[3] >> 4;
	return weights;
}

inline FixedArray<uint8_t, 4> decode_indices_from_packed_u16(uint16_t packed_indices) {
	FixedArray<uint8_t, 4> indices;
	// 可 SIMD 化？
	indices[0] = (packed_indices >> 0) & 0x0f;
	indices[1] = (packed_indices >> 4) & 0x0f;
	indices[2] = (packed_indices >> 8) & 0x0f;
	indices[3] = (packed_indices >> 12) & 0x0f;
	return indices;
}

inline constexpr uint16_t encode_indices_to_packed_u16(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
	return (a & 0xf) | ((b & 0xf) << 4) | ((c & 0xf) << 8) | ((d & 0xf) << 12);
}

// 将 0..255 的值编码为打包在 16 位中的 0..15 值。输入值的低 4 位将不会被保留。
inline constexpr uint16_t encode_weights_to_packed_u16_lossy(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
	return (a >> 4) | ((b >> 4) << 4) | ((c >> 4) << 8) | ((d >> 4) << 12);
}

// 检查任意体素中是否没有重复的索引
inline void debug_check_texture_indices(FixedArray<uint8_t, 4> indices) {
	FixedArray<bool, 16> checked;
	fill(checked, false);
	for (unsigned int i = 0; i < indices.size(); ++i) {
		unsigned int ti = indices[i];
		VOXEL_ASSERT(!checked[ti]);
		checked[ti] = true;
	}
}

inline void _normalize_weights_preserving(
		FixedArray<float, 4> &weights,
		const unsigned int preserved_index,
		const unsigned int other0,
		const unsigned int other1,
		const unsigned int other2
) {
	const float part_sum = weights[other0] + weights[other1] + weights[other2];
	// 假定保留的通道已经夹取到 [0, 1]
	const float expected_part_sum = 1.f - weights[preserved_index];

	if (part_sum < 0.0001f) {
		weights[other0] = expected_part_sum / 3.f;
		weights[other1] = expected_part_sum / 3.f;
		weights[other2] = expected_part_sum / 3.f;

	} else {
		const float scale = expected_part_sum / part_sum;
		weights[other0] *= scale;
		weights[other1] *= scale;
		weights[other2] *= scale;
	}
}

inline void normalize_weights_preserving(FixedArray<float, 4> &weights, unsigned int preserved_index) {
	switch (preserved_index) {
		case 0:
			_normalize_weights_preserving(weights, 0, 1, 2, 3);
			break;
		case 1:
			_normalize_weights_preserving(weights, 1, 0, 2, 3);
			break;
		case 2:
			_normalize_weights_preserving(weights, 2, 1, 0, 3);
			break;
		default:
			_normalize_weights_preserving(weights, 3, 1, 2, 0);
			break;
	}
}

/*inline void normalize_weights(FixedArray<float, 4> &weights) {
	float sum = 0;
	for (unsigned int i = 0; i < weights.size(); ++i) {
		sum += weights[i];
	}
	const float sum_inv = 255.f / sum;
	for (unsigned int i = 0; i < weights.size(); ++i) {
		weights[i] = clamp(weights[i] * sum_inv, 0.f, 255.f);
	}
}*/

inline void blend_texture_packed_u16(
		const int texture_index,
		const float target_weight,
		uint16_t &encoded_indices,
		uint16_t &encoded_weights
) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT_RETURN(target_weight >= 0.f && target_weight <= 1.f);
#endif

	FixedArray<uint8_t, 4> indices = decode_indices_from_packed_u16(encoded_indices);
	FixedArray<uint8_t, 4> weights = decode_weights_from_packed_u16(encoded_weights);

	// 查找我们的纹理索引是否已经存在
	unsigned int component_index = 4;
	for (unsigned int i = 0; i < indices.size(); ++i) {
		if (indices[i] == texture_index) {
			component_index = i;
			break;
		}
	}

	bool index_was_changed = false;
	if (component_index >= indices.size()) {
		// 我们的纹理索引不存在，将替换权重最低的组件
		uint8_t lowest_weight = 255;
		// 在所有权重都取最大值这一假设情况下，默认为 0
		component_index = 0;
		for (unsigned int i = 0; i < weights.size(); ++i) {
			if (weights[i] < lowest_weight) {
				lowest_weight = weights[i];
				component_index = i;
			}
		}
		indices[component_index] = texture_index;
		index_was_changed = true;
	}

	// TODO 优化：target_weight 为 1 时的情况？
	FixedArray<float, 4> weights_f;
	for (unsigned int i = 0; i < weights.size(); ++i) {
		weights_f[i] = weights[i] / 255.f;
	}

	if (weights_f[component_index] < target_weight || index_was_changed) {
		weights_f[component_index] = target_weight;

		normalize_weights_preserving(weights_f, component_index);

		for (unsigned int i = 0; i < weights_f.size(); ++i) {
			weights[i] = math::clamp(weights_f[i] * 255.f, 0.f, 255.f);
		}

		encoded_indices = encode_indices_to_packed_u16(indices[0], indices[1], indices[2], indices[3]);
		encoded_weights = encode_weights_to_packed_u16_lossy(weights[0], weights[1], weights[2], weights[3]);
	}
}

void debug_check_texture_indices_packed_u16(const VoxelBuffer &voxels);

constexpr inline uint16_t make_encoded_weights_for_single_texture() {
	return encode_weights_to_packed_u16_lossy(255, 0, 0, 0);
}

constexpr inline uint16_t make_encoded_indices_for_single_texture(uint8_t index) {
	// 确保其他索引不同，这样与它们关联的权重不会覆盖第一个
	// 索引的权重。
	const uint8_t index1 = (index + 1) & 0xf;
	const uint8_t index2 = (index + 2) & 0xf;
	const uint8_t index3 = (index + 3) & 0xf;
	const uint16_t encoded_indices = encode_indices_to_packed_u16(index, index1, index2, index3);
	return encoded_indices;
	// 注意：另一种做法是将索引对齐，使第一个索引是 4 的倍数且后续索引连续。
	// 这会最小化索引布局的变化，同时保持它们有序，从而可能减少网格器需要制作的接缝数量。
	// 但它也需要把权重考虑进去，而不是假定相关槽位总是第一个。目前还没有做，
	// 因为优先级不高，而且格式很可能会变得更简单
}

} // namespace mixel4
} // namespace voxel

#endif // VOXEL_MIXEL4_H
