#ifndef VOXEL_DATA_MAP_H
#define VOXEL_DATA_MAP_H

#include "../constants/voxel_constants.h"
#include "../util/containers/fixed_array.h"
#include "../util/containers/span.h"
#include "../util/containers/std_unordered_map.h"
#include "../util/math/box3i.h"
#include "../util/profiling.h"
#include "voxel_buffer.h" // 在模板方法中使用
#include "voxel_data_block.h"
#include "voxel_format.h"

namespace voxel {

class VoxelGenerator;

// 在恒定 LOD 内，通过立方体 chunk 实现的稀疏体素存储。
//
// 进行数据流式传输时，体积是*部分*加载的。如果在某些坐标找不到块，
// 说明我们不知道它是否包含编辑。了解这一点很重要，可以避免在空白区域
// 写入或缓存体素数据，因为那些区域一旦加载可能完全不同。
// 使用"完整加载"编辑时，这无关紧要。如果所有编辑都已加载，我们预先知道
// 其他所有内容都未编辑（这也意味着我们可能找不到没有数据的块）。
//
class VoxelDataMap {
public:
	// 这是以体素为单位的块大小。要转换为空间单位，请使用 `block_size << lod_index`。
	static const unsigned int BLOCK_SIZE_PO2 = constants::DEFAULT_BLOCK_SIZE_PO2;
	static const unsigned int BLOCK_SIZE = 1 << BLOCK_SIZE_PO2;
	static const unsigned int BLOCK_SIZE_MASK = BLOCK_SIZE - 1;

	// 将体素坐标转换为块坐标。
	// 不要使用除法，因为它会在负坐标中引入偏移。
	static inline Vector3i voxel_to_block_b(Vector3i pos, int block_size_pow2) {
		return pos >> block_size_pow2;
	}

	inline Vector3i voxel_to_block(Vector3i pos) const {
		return voxel_to_block_b(pos, BLOCK_SIZE_PO2);
	}

	inline Vector3i to_local(Vector3i pos) const {
		return Vector3i(pos.x & BLOCK_SIZE_MASK, pos.y & BLOCK_SIZE_MASK, pos.z & BLOCK_SIZE_MASK);
	}

	// 将块坐标转换为体素坐标。
	inline Vector3i block_to_voxel(Vector3i bpos) const {
		return bpos * BLOCK_SIZE;
	}

	VoxelDataMap();
	~VoxelDataMap();

	void create(unsigned int lod_index);

	void set_format(const VoxelFormat format);
	inline const VoxelFormat &get_format() const {
		return _format;
	}

	inline unsigned int get_block_size() const {
		return BLOCK_SIZE;
	}
	inline unsigned int get_block_size_pow2() const {
		return BLOCK_SIZE_PO2;
	}
	inline unsigned int get_block_size_mask() const {
		return BLOCK_SIZE_MASK;
	}

	void set_lod_index(int lod_index);
	unsigned int get_lod_index() const;

	int get_voxel(Vector3i pos, unsigned int c = 0) const;
	void set_voxel(int value, Vector3i pos, unsigned int c = 0);

	float get_voxel_f(Vector3i pos, unsigned int c) const;
	void set_voxel_f(real_t value, Vector3i pos, unsigned int c);

	inline void copy(
			const Vector3i min_pos,
			VoxelBuffer &dst_buffer,
			const unsigned int channels_mask,
			const bool with_metadata
	) const {
		copy(min_pos, dst_buffer, channels_mask, nullptr, nullptr, with_metadata);
	}

	// 获取从 min_pos 开始、与 dst_buffer 大小相同的区域内所有体素的副本。
	void copy(
			const Vector3i min_pos,
			VoxelBuffer &dst_buffer,
			const unsigned int channels_mask,
			void *callback_data,
			void (*gen_func)(void *, VoxelBuffer &, Vector3i),
			const bool with_metadata
	) const;

	void paste(
			const Vector3i min_pos,
			const VoxelBuffer &src_buffer,
			const unsigned int channels_mask,
			const bool create_new_blocks,
			const bool with_metadata
	);

	void paste_masked(
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
	);

	// 将给定缓冲区移入地图的一个块中。该缓冲区被引用，不进行复制。
	VoxelDataBlock *set_block_buffer(Vector3i bpos, std::shared_ptr<VoxelBuffer> &buffer, bool overwrite);
	VoxelDataBlock *set_empty_block(Vector3i bpos, bool overwrite);
	void set_block(Vector3i bpos, const VoxelDataBlock &block);

	struct NoAction {
		inline void operator()(VoxelDataBlock &block) {}
	};

	template <typename Action_T>
	void remove_block(Vector3i bpos, Action_T pre_delete) {
		auto it = _blocks_map.find(bpos);
		if (it != _blocks_map.end()) {
			pre_delete(it->second);
			_blocks_map.erase(it);
		}
	}

	VoxelDataBlock *get_block(Vector3i bpos);
	const VoxelDataBlock *get_block(Vector3i bpos) const;

	bool has_block(Vector3i pos) const;
	bool is_block_surrounded(Vector3i pos) const;

	void clear();

	int get_block_count() const;

	// op(Vector3i bpos)
	template <typename Op_T>
	inline void for_each_block_position(Op_T op) const {
		for (auto it = _blocks_map.begin(); it != _blocks_map.end(); ++it) {
			op(it->first);
		}
	}

	// op(Vector3i bpos, VoxelDataBlock &block)
	template <typename Op_T>
	inline void for_each_block(Op_T op) {
		for (auto it = _blocks_map.begin(); it != _blocks_map.end(); ++it) {
			op(it->first, it->second);
		}
	}

	// void op(Vector3i bpos, const VoxelDataBlock &block)
	template <typename Op_T>
	inline void for_each_block(Op_T op) const {
		for (auto it = _blocks_map.begin(); it != _blocks_map.end(); ++it) {
			op(it->first, it->second);
		}
	}

	bool is_area_fully_loaded(const Box3i voxels_box) const;

	template <typename F>
	inline void write_box(const Box3i &voxel_box, unsigned int channel, F action) {
		write_box(voxel_box, channel, action, [](const VoxelBuffer &, const Vector3i &) {});
	}

	// D F(Vector3i pos, D value)
	template <typename F, typename G>
	void write_box(const Box3i &voxel_box, unsigned int channel, F action, G gen_func) {
		const Box3i block_box = voxel_box.downscaled(get_block_size());
		const Vector3i block_size = Vector3iUtil::create(get_block_size());
		block_box.for_each_cell_zxy([this, action, voxel_box, channel, block_size, gen_func](Vector3i block_pos) {
			VoxelDataBlock *block = get_block(block_pos);
			if (block == nullptr) {
				VOXEL_PROFILE_SCOPE_NAMED("Generate");
				block = create_default_block(block_pos);
				gen_func(block->get_voxels(), block_pos << get_block_size_pow2());
			}
			const Vector3i block_origin = block_to_voxel(block_pos);
			Box3i local_box(voxel_box.position - block_origin, voxel_box.size);
			local_box.clip(Box3i(Vector3i(), block_size));
			block->get_voxels().write_box(local_box, channel, action, block_origin);
		});
	}

	template <typename F>
	inline void write_box_2(const Box3i &voxel_box, unsigned int channel0, unsigned int channel1, F action) {
		write_box_2(voxel_box, channel0, channel1, action, [](const VoxelBuffer &, const Vector3i &) {});
	}

	// void F(Vector3i pos, D0 &value, D1 &value)
	template <typename F, typename G>
	void write_box_2(const Box3i &voxel_box, unsigned int channel0, unsigned int channel1, F action, G gen_func) {
		const Box3i block_box = voxel_box.downscaled(get_block_size());
		const Vector3i block_size = Vector3iUtil::create(get_block_size());
		block_box.for_each_cell_zxy(
				[this, action, voxel_box, channel0, channel1, block_size, gen_func](Vector3i block_pos) {
					VoxelDataBlock *block = get_block(block_pos);
					if (block == nullptr) {
						block = create_default_block(block_pos);
						gen_func(block->get_voxels(), block_pos << get_block_size_pow2());
					}
					const Vector3i block_origin = block_to_voxel(block_pos);
					Box3i local_box(voxel_box.position - block_origin, voxel_box.size);
					local_box.clip(Box3i(Vector3i(), block_size));
					block->get_voxels().write_box_2_template<F, uint16_t, uint16_t>(
							local_box, channel0, channel1, action, block_origin
					);
				}
		);
	}

private:
	// void set_block(Vector3i bpos, VoxelDataBlock *block);
	VoxelDataBlock *get_or_create_block_at_voxel_pos(Vector3i pos);
	VoxelDataBlock *create_default_block(Vector3i bpos);

	// void set_block_size_pow2(unsigned int p);

private:
	// 块以三维方向上的空间哈希存储。
	// 以前我使用 Godot 3 的 HashMap（RELATIONSHIP = 2），因为它比默认配置性能更好，
	// 但它在删除时有时会有很长的停顿，而 std::unordered_map 似乎没有
	// （至少没那么严重）。而且整体性能略好。
	// 注意：插入或删除其他元素时，指向元素的指针仍然有效（只有迭代器可能失效）
	StdUnorderedMap<Vector3i, VoxelDataBlock> _blocks_map;

	// 这在单线程场景中可能是可行的优化，但在多线程中不行。
	// 我们希望能够共享读访问，但这是一个可变变量。
	// 如果我们想恢复这个，它可能要以某种方式是线程局部的。
	//
	// 体素访问最常发生在连续区域，因此会访问相同的块。
	// 为了防止过多的哈希计算，会先检查这个引用。
	// mutable VoxelDataBlock *_last_accessed_block = nullptr;

	unsigned int _lod_index = 0;
	VoxelFormat _format;
};

} // namespace voxel

#endif // VOXEL_MAP_H
