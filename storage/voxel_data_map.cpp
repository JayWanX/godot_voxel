#include "voxel_data_map.h"
#include "../constants/cube_tables.h"
#include "../edition/funcs.h"
#include "../generators/voxel_generator.h"
#include "../util/containers/dynamic_bitset.h"
#include "../util/macros.h"
#include "../util/memory/memory.h"
#include "../util/string/format.h"
#include "voxel_format.h"

#include <limits>

namespace voxel {

VoxelDataMap::VoxelDataMap() {
	// 目前不打算在运行时更改此项。
	// set_block_size_pow2(constants::DEFAULT_BLOCK_SIZE_PO2);
}

VoxelDataMap::~VoxelDataMap() {
	clear();
}

void VoxelDataMap::create(unsigned int lod_index) {
	VOXEL_ASSERT(lod_index < constants::MAX_LOD);
	clear();
	// set_block_size_pow2(block_size_po2);
	set_lod_index(lod_index);
}

void VoxelDataMap::set_format(const VoxelFormat format) {
	_format = format;
}

// void VoxelDataMap::set_block_size_pow2(unsigned int p) {
// 	VOXEL_ASSERT_RETURN_MSG(p >= 1, "Block size is too small");
// 	VOXEL_ASSERT_RETURN_MSG(p <= 8, "Block size is too big");

// 	_block_size_pow2 = p;
// 	_block_size = 1 << _block_size_pow2;
// 	_block_size_mask = _block_size - 1;
// }

void VoxelDataMap::set_lod_index(int lod_index) {
	VOXEL_ASSERT_RETURN_MSG(lod_index >= 0, "LOD index can't be negative");
	VOXEL_ASSERT_RETURN_MSG(lod_index < 32, "LOD index is too big");

	_lod_index = lod_index;
}

unsigned int VoxelDataMap::get_lod_index() const {
	return _lod_index;
}

int VoxelDataMap::get_voxel(Vector3i pos, unsigned int c) const {
	Vector3i bpos = voxel_to_block(pos);
	const VoxelDataBlock *block = get_block(bpos);
	if (block == nullptr || !block->has_voxels()) {
		return _format.get_default_raw_value(static_cast<VoxelBuffer::ChannelId>(c));
	}
	return block->get_voxels_const().get_voxel(to_local(pos), c);
}

VoxelDataBlock *VoxelDataMap::create_default_block(Vector3i bpos) {
	std::shared_ptr<VoxelBuffer> buffer = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
	buffer->create(Vector3iUtil::create(get_block_size()), &_format);
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT_RETURN_V(!has_block(bpos), nullptr);
#endif
	VoxelDataBlock &map_block = _blocks_map[bpos];
	map_block = VoxelDataBlock(buffer, _lod_index);
	return &map_block;
}

VoxelDataBlock *VoxelDataMap::get_or_create_block_at_voxel_pos(Vector3i pos) {
	Vector3i bpos = voxel_to_block(pos);
	VoxelDataBlock *block = get_block(bpos);
	if (block == nullptr) {
		block = create_default_block(bpos);
	}
	return block;
}

void VoxelDataMap::set_voxel(int value, Vector3i pos, unsigned int c) {
	VoxelDataBlock *block = get_or_create_block_at_voxel_pos(pos);
	// TODO 如果发现这是个问题，就使用写时复制（CoW）
	VoxelBuffer &voxels = block->get_voxels();
	voxels.set_voxel(value, to_local(pos), c);
}

float VoxelDataMap::get_voxel_f(Vector3i pos, unsigned int c) const {
	Vector3i bpos = voxel_to_block(pos);
	const VoxelDataBlock *block = get_block(bpos);
	// TODO 如果块没有体素，需要调用生成器
	if (block == nullptr || !block->has_voxels()) {
		return constants::SDF_FAR_OUTSIDE;
	}
	Vector3i lpos = to_local(pos);
	return block->get_voxels_const().get_voxel_f(lpos.x, lpos.y, lpos.z, c);
}

void VoxelDataMap::set_voxel_f(real_t value, Vector3i pos, unsigned int c) {
	VoxelDataBlock *block = get_or_create_block_at_voxel_pos(pos);
	Vector3i lpos = to_local(pos);
	// TODO 在这种情况下，必须调用生成器来填充块
	VOXEL_ASSERT_RETURN_MSG(block->has_voxels(), "Block not cached");
	VoxelBuffer &voxels = block->get_voxels();
	voxels.set_voxel_f(value, lpos.x, lpos.y, lpos.z, c);
}

VoxelDataBlock *VoxelDataMap::get_block(Vector3i bpos) {
	auto it = _blocks_map.find(bpos);
	if (it != _blocks_map.end()) {
		return &it->second;
	}
	return nullptr;
}

const VoxelDataBlock *VoxelDataMap::get_block(Vector3i bpos) const {
	auto it = _blocks_map.find(bpos);
	if (it != _blocks_map.end()) {
		return &it->second;
	}
	return nullptr;
}

VoxelDataBlock *VoxelDataMap::set_block_buffer(Vector3i bpos, std::shared_ptr<VoxelBuffer> &buffer, bool overwrite) {
	VOXEL_ASSERT_RETURN_V(buffer != nullptr, nullptr);

	VoxelDataBlock *block = get_block(bpos);

	if (block == nullptr) {
		VoxelDataBlock &map_block = _blocks_map[bpos];
		map_block = VoxelDataBlock(buffer, _lod_index);
		block = &map_block;

	} else if (overwrite) {
		block->set_voxels(buffer);

	} else {
		VOXEL_PROFILE_MESSAGE("Redundant data block");
		VOXEL_PRINT_VERBOSE(format(
				"Discarded block {} lod {}, there was already data and overwriting is not enabled", bpos, _lod_index
		));
	}

	return block;
}

void VoxelDataMap::set_block(Vector3i bpos, const VoxelDataBlock &block) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(block.get_lod_index() == _lod_index);
#endif
	_blocks_map[bpos] = block;
}

VoxelDataBlock *VoxelDataMap::set_empty_block(Vector3i bpos, bool overwrite) {
	VoxelDataBlock *block = get_block(bpos);

	if (block == nullptr) {
		VoxelDataBlock &map_block = _blocks_map[bpos];
		map_block = VoxelDataBlock(_lod_index);
		block = &map_block;

	} else if (overwrite) {
		block->clear_voxels();

	} else {
		VOXEL_PROFILE_MESSAGE("Redundant data block");
		VOXEL_PRINT_VERBOSE(format(
				"Discarded block {} lod {}, there was already data and overwriting is not enabled", bpos, _lod_index
		));
	}

	return block;
}

bool VoxelDataMap::has_block(Vector3i pos) const {
	return _blocks_map.find(pos) != _blocks_map.end();
}

bool VoxelDataMap::is_block_surrounded(Vector3i pos) const {
	// TODO 如果该检查在我们处理的所有块上被证明过于昂贵，就在 VoxelBlock 中缓存它
	for (unsigned int i = 0; i < Cube::MOORE_NEIGHBORING_3D_COUNT; ++i) {
		Vector3i bpos = pos + Cube::g_moore_neighboring_3d[i];
		if (!has_block(bpos)) {
			return false;
		}
	}
	return true;
}

void VoxelDataMap::copy(
		const Vector3i min_pos,
		VoxelBuffer &dst_buffer,
		const unsigned int channels_mask,
		void *callback_data,
		void (*gen_func)(void *, VoxelBuffer &, Vector3i),
		const bool with_metadata
) const {
	// TODO 使用 `copy_from_chunked_storage` 重新实现？

	VOXEL_ASSERT_RETURN_MSG(Vector3iUtil::get_volume_u64(dst_buffer.get_size()) > 0, "The area to copy is empty");
	const Vector3i max_pos = min_pos + dst_buffer.get_size();

	const Vector3i min_block_pos = voxel_to_block(min_pos);
	const Vector3i max_block_pos = voxel_to_block(max_pos - Vector3i(1, 1, 1)) + Vector3i(1, 1, 1);

	const Vector3i block_size_v(get_block_size(), get_block_size(), get_block_size());

	const SmallVector<uint8_t, VoxelBuffer::MAX_CHANNELS> channels = VoxelBuffer::mask_to_channels_list(channels_mask);

	Vector3i bpos;
	for (bpos.z = min_block_pos.z; bpos.z < max_block_pos.z; ++bpos.z) {
		for (bpos.x = min_block_pos.x; bpos.x < max_block_pos.x; ++bpos.x) {
			for (bpos.y = min_block_pos.y; bpos.y < max_block_pos.y; ++bpos.y) {
				const VoxelDataBlock *block = get_block(bpos);
				const Vector3i src_block_origin = block_to_voxel(bpos);

				if (block != nullptr && block->has_voxels()) {
					const VoxelBuffer &src_buffer = block->get_voxels_const();

					for (const uint8_t channel : channels) {
						dst_buffer.set_channel_depth(channel, src_buffer.get_channel_depth(channel));
						// 注意：如果区域在边缘，copy_from 会负责裁剪
						dst_buffer.copy_channel_from(
								src_buffer, min_pos - src_block_origin, src_buffer.get_size(), Vector3i(), channel
						);
					}

					if (with_metadata) {
						dst_buffer.copy_voxel_metadata_in_area(
								src_buffer,
								Box3i::from_min_max(min_pos - src_block_origin, src_buffer.get_size()),
								Vector3i()
						);
					}

				} else if (gen_func != nullptr) {
					const Box3i box = Box3i(bpos << get_block_size_pow2(), block_size_v)
											  .clipped(Box3i(min_pos, dst_buffer.get_size()));

					VoxelBuffer temp(VoxelBuffer::ALLOCATOR_POOL);
					temp.copy_format(dst_buffer);
					temp.create(box.size);
					gen_func(callback_data, temp, box.position);

					for (const uint8_t channel : channels) {
						dst_buffer.copy_channel_from(
								temp, Vector3i(), temp.get_size(), box.position - min_pos, channel
						);
					}

					if (with_metadata) {
					// 不确定让按需生成的工作流同时生成体素元数据是否合理？
					dst_buffer.copy_voxel_metadata_in_area(
							temp, Box3i(Vector3i(), temp.get_size()), box.position - min_pos
					);
				}

				} else {
					for (const uint8_t channel : channels) {
						// 目前，不存在的块默认使用硬编码的默认值，对应"空白区域"。
						// 如果想改变这一点，可能需要为此添加一个 API。
						dst_buffer.fill_area(
								_format.get_default_raw_value(static_cast<VoxelBuffer::ChannelId>(channel)),
								src_block_origin - min_pos,
								src_block_origin - min_pos + block_size_v,
								channel
						);
					}
				}
			}
		}
	}
}

void VoxelDataMap::paste(
		const Vector3i min_pos,
		const VoxelBuffer &src_buffer,
		const unsigned int channels_mask,
		const bool create_new_blocks,
		const bool with_metadata
) {
	paste_masked(
			min_pos,
			src_buffer,
			channels_mask,
			false,
			0,
			0,
			false,
			0,
			Span<const int32_t>(),
			create_new_blocks,
			with_metadata
	);
}

void VoxelDataMap::paste_masked(
		const Vector3i min_pos,
		const VoxelBuffer &src_buffer,
		const unsigned int channels_mask,
		const bool use_src_mask,
		const uint8_t src_mask_channel,
		const uint64_t src_mask_value,
		const bool use_dst_mask,
		const uint8_t dst_mask_channel,
		const Span<const int32_t> dst_writable_values,
		const bool create_new_blocks,
		const bool with_metadata
) {
	if (use_dst_mask && !use_src_mask) {
		VOXEL_PRINT_ERROR("Destination mask without source mask is not implemented");
		return;
	}

	// TODO 使用 `copy_to_chunked_storage` 重新实现？
	//
	const Vector3i max_pos = min_pos + src_buffer.get_size();

	const Vector3i min_block_pos = voxel_to_block(min_pos);
	const Vector3i max_block_pos = voxel_to_block(max_pos - Vector3i(1, 1, 1)) + Vector3i(1, 1, 1);

	const SmallVector<uint8_t, VoxelBuffer::MAX_CHANNELS> channel_indices =
			VoxelBuffer::mask_to_channels_list(channels_mask);

	DynamicBitset bitarray;
	if (dst_writable_values.size() > 1) {
		VOXEL_ASSERT_RETURN(indices_to_bitarray_u16(dst_writable_values, bitarray));
	}

	Vector3i bpos;
	for (bpos.z = min_block_pos.z; bpos.z < max_block_pos.z; ++bpos.z) {
		for (bpos.x = min_block_pos.x; bpos.x < max_block_pos.x; ++bpos.x) {
			for (bpos.y = min_block_pos.y; bpos.y < max_block_pos.y; ++bpos.y) {
				VoxelDataBlock *block = get_block(bpos);

				if (block == nullptr) {
					if (create_new_blocks) {
						block = create_default_block(bpos);
					} else {
						continue;
					}
				}

				// TODO 在这种情况下，必须调用生成器来填充空白
				VOXEL_ASSERT_CONTINUE_MSG(block->has_voxels(), "Area not cached");

				const Vector3i dst_block_origin = block_to_voxel(bpos);

				VoxelBuffer &dst_buffer = block->get_voxels();
				const Vector3i dst_base_pos = min_pos - dst_block_origin;

				if (use_src_mask) {
					if (use_dst_mask) {
						if (dst_writable_values.size() == 1) {
							voxel::paste_src_masked_dst_writable_value(
									to_span(channel_indices),
									src_buffer,
									src_mask_channel,
									src_mask_value,
									dst_buffer,
									dst_base_pos,
									dst_mask_channel,
									dst_writable_values[0],
									with_metadata
							);

						} else {
							voxel::paste_src_masked_dst_writable_bitarray(
									to_span(channel_indices),
									src_buffer,
									src_mask_channel,
									src_mask_value,
									dst_buffer,
									dst_base_pos,
									dst_mask_channel,
									bitarray,
									with_metadata
							);
						}

					} else {
						voxel::paste_src_masked(
								to_span(channel_indices),
								src_buffer,
								src_mask_channel,
								src_mask_value,
								dst_buffer,
								dst_base_pos,
								with_metadata
						);
					}

				} else {
					voxel::paste(to_span(channel_indices), src_buffer, dst_buffer, dst_base_pos, with_metadata);
				}
			}
		}
	}
}

void VoxelDataMap::clear() {
	_blocks_map.clear();
}

int VoxelDataMap::get_block_count() const {
	return _blocks_map.size();
}

bool VoxelDataMap::is_area_fully_loaded(const Box3i voxels_box) const {
	Box3i block_box = voxels_box.downscaled(get_block_size());
	return block_box.all_cells_match([this](Vector3i pos) { //
		return has_block(pos);
	});
}

} // namespace voxel
