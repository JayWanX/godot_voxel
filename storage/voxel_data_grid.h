#ifndef VOXEL_DATA_GRID_H
#define VOXEL_DATA_GRID_H

#include "../storage/voxel_buffer.h"
#include "../util/thread/rw_lock.h"
#include "../util/thread/spatial_lock_3d.h"
#include "voxel_data_map.h"

namespace voxel {

// 在有限网格中存储体素数据块。
// 这被用作某些操作的临时存储，以避免长时间持有地图上的排他锁。
// TODO 提供一个只读版本以强制禁止写入？
class VoxelDataGrid {
public:
	// 重建网格并缓存与指定体素盒相交的块。
	// 警告：给定的盒是以体素为单位、相对于传入地图的。如果该地图不是 LOD0，
	// 若你期望 LOD0 坐标，可能需要缩小盒。
	// inline void reference_area(const VoxelDataMap &map, Box3i voxel_box, SpatialLock3D *sl) {
	// 	const Box3i blocks_box = voxel_box.downscaled(map.get_block_size());
	// 	reference_area_block_coords(map, blocks_box, sl);
	// }

	// TODO 这个 API 有点冒险，也许应该封装到 VoxelData 中
	inline void reference_area_block_coords(
			const VoxelDataMap &map,
			RWLock &map_lock,
			const Box3i blocks_box,
			// 将被操作引用，假设其生命周期等于或长于网格
			SpatialLock3D &spatial_lock
	) {
		VOXEL_PROFILE_SCOPE();
		create(blocks_box.size, map.get_block_size());

		_offset_in_blocks = blocks_box.position;
		_logical_offset_in_blocks = blocks_box.position;

		// 需要加锁，因为我们访问 `has_voxels`
		spatial_lock.lock_read(blocks_box);

		{
			RWLockRead rlock(map_lock);
			blocks_box.for_each_cell_zxy([&map, this](const Vector3i pos) {
				const VoxelDataBlock *block = map.get_block(pos);
				// TODO 可能需要为存在但没有体素的块在某个层级调用生成器，
				// 或确保所有块都包含体素数据
				if (block != nullptr && block->has_voxels()) {
					set_block(pos, block->get_voxels_shared());
				} else {
					set_block(pos, nullptr);
				}
			});
		}

		spatial_lock.unlock_read(blocks_box);

		_spatial_lock = &spatial_lock;
	}

	inline bool has_any_block() const {
		for (unsigned int i = 0; i < _blocks.size(); ++i) {
			if (_blocks[i] != nullptr) {
				return true;
			}
		}
		return false;
	}

	// 在对网格进行操作之前，必须先对其加锁。

	struct LockRead {
		LockRead(const VoxelDataGrid &p_grid) : grid(p_grid) {
			grid.lock_read();
		}
		~LockRead() {
			grid.unlock_read();
		}
		const VoxelDataGrid &grid;
	};

	struct LockWrite {
		LockWrite(VoxelDataGrid &p_grid) : grid(p_grid) {
			grid.lock_write();
		}
		~LockWrite() {
			grid.unlock_write();
		}
		VoxelDataGrid &grid;
	};

	inline void lock_read() const {
		VOXEL_ASSERT(_spatial_lock != nullptr);
		VOXEL_ASSERT(!_locked);
		_spatial_lock->lock_read(BoxBounds3i::from_position_size(_offset_in_blocks, _size_in_blocks));
		_locked = true;
	}

	inline void unlock_read() const {
		VOXEL_ASSERT(_spatial_lock != nullptr);
		VOXEL_ASSERT(_locked);
		_spatial_lock->unlock_read(BoxBounds3i::from_position_size(_offset_in_blocks, _size_in_blocks));
		_locked = false;
	}

	inline void lock_write() {
		VOXEL_ASSERT(_spatial_lock != nullptr);
		VOXEL_ASSERT(!_locked);
		_spatial_lock->lock_write(BoxBounds3i::from_position_size(_offset_in_blocks, _size_in_blocks));
		_locked = true;
	}

	inline void unlock_write() {
		VOXEL_ASSERT(_spatial_lock != nullptr);
		VOXEL_ASSERT(_locked);
		_spatial_lock->unlock_write(BoxBounds3i::from_position_size(_offset_in_blocks, _size_in_blocks));
		_locked = false;
	}

	inline bool try_get_voxel_f(Vector3i pos, float &out_value, VoxelBuffer::ChannelId channel) const {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(_locked);
#endif
		const Vector3i bpos = (pos >> _block_size_po2) - _logical_offset_in_blocks;
		if (!is_valid_relative_block_position(bpos)) {
			return false;
		}
		const unsigned int loc = Vector3iUtil::get_zxy_index(bpos, _size_in_blocks);
		const VoxelBuffer *voxels = _blocks[loc].get();
		if (voxels == nullptr) {
			return false;
		}
		const unsigned int mask = (1 << _block_size_po2) - 1;
		const Vector3i rpos = pos & mask;
		out_value = voxels->get_voxel_f(rpos, channel);
		return true;
	}

	// D action(Vector3i pos, D value)
	template <typename F>
	void write_box(Box3i voxel_box, unsigned int channel, F action) {
		if (_spatial_lock != nullptr) {
			lock_write();
		}
		_box_loop(voxel_box, [action, channel](VoxelBuffer &voxels, Box3i local_box, Vector3i voxel_offset) {
			voxels.write_box(local_box, channel, action, voxel_offset);
		});
		if (_spatial_lock != nullptr) {
			unlock_write();
		}
	}

	// D action(Vector3i pos, D value)
	template <typename F>
	void write_box_no_lock(Box3i voxel_box, unsigned int channel, F action) {
		_box_loop(voxel_box, [action, channel](VoxelBuffer &voxels, Box3i local_box, Vector3i voxel_offset) {
			voxels.write_box(local_box, channel, action, voxel_offset);
		});
	}

	// void action(Vector3i pos, D0 &value, D1 &value)
	template <typename F>
	void write_box_2(const Box3i &voxel_box, unsigned int channel0, unsigned int channel1, F action) {
		if (_spatial_lock != nullptr) {
			lock_write();
		}
		_box_loop(voxel_box, [action, channel0, channel1](VoxelBuffer &voxels, Box3i local_box, Vector3i voxel_offset) {
			voxels.write_box_2_template<F, uint16_t, uint16_t>(local_box, channel0, channel1, action, voxel_offset);
		});
		if (_spatial_lock != nullptr) {
			unlock_write();
		}
	}

	// void action(Vector3i pos, D0 &value, D1 &value)
	template <typename F>
	void write_box_2_no_lock(const Box3i &voxel_box, unsigned int channel0, unsigned int channel1, F action) {
		_box_loop(voxel_box, [action, channel0, channel1](VoxelBuffer &voxels, Box3i local_box, Vector3i voxel_offset) {
			voxels.write_box_2_template<F, uint16_t, uint16_t>(local_box, channel0, channel1, action, voxel_offset);
		});
	}

	// inline const VoxelBuffer *get_block(Vector3i position) const {
	// 	ERR_FAIL_COND_V(!is_valid_position(position), nullptr);
	// 	position -= _offset_in_blocks;
	// 	const unsigned int index = Vector3iUtil::get_zxy_index(position, _size_in_blocks);
	// 	CRASH_COND(index >= _blocks.size());
	// 	return _blocks[index].get();
	// }

	inline void clear() {
		VOXEL_ASSERT(!_locked);
		_blocks.clear();
		_size_in_blocks = Vector3i();
		_spatial_lock = nullptr;
	}

	inline VoxelBuffer *get_block_no_lock(Vector3i position) {
		return get_block(position);
	}

	inline unsigned int get_block_size_po2() const {
		return _block_size_po2;
	}

	inline Vector3i get_origin_block_position_in_blocks() const {
		return _offset_in_blocks;
	}

	inline Vector3i get_origin_block_position_in_voxels() const {
		return _offset_in_blocks << _block_size_po2;
	}

	inline void use_relative_coordinates() {
		_logical_offset_in_blocks = Vector3i();
	}

private:
	inline unsigned int get_block_size() const {
		return _block_size;
	}

	template <typename Block_F>
	inline void _box_loop(const Box3i voxel_box, Block_F block_action) {
		_box_loop_with_offset(voxel_box, _logical_offset_in_blocks, block_action);
	}

	template <typename Block_F>
	inline void _box_loop_with_offset(const Box3i voxel_box, const Vector3i offset_in_blocks, Block_F block_action) {
		Vector3i block_rpos;
		const Vector3i area_origin_in_voxels = offset_in_blocks * _block_size;
		unsigned int index = 0;
		for (block_rpos.z = 0; block_rpos.z < _size_in_blocks.z; ++block_rpos.z) {
			for (block_rpos.x = 0; block_rpos.x < _size_in_blocks.x; ++block_rpos.x) {
				for (block_rpos.y = 0; block_rpos.y < _size_in_blocks.y; ++block_rpos.y) {
					VoxelBuffer *block = _blocks[index].get();
					// 扁平网格和迭代顺序允许我们直接递增索引，因为我们遍历了所有元素
					++index;
					if (block == nullptr) {
						continue;
					}
					const Vector3i block_origin = block_rpos * _block_size + area_origin_in_voxels;
					Box3i local_box(voxel_box.position - block_origin, voxel_box.size);
					local_box.clip(Box3i(Vector3i(), Vector3iUtil::create(_block_size)));
					block_action(*block, local_box, block_origin);
				}
			}
		}
	}

	inline void create(Vector3i size, unsigned int block_size) {
		VOXEL_PROFILE_SCOPE();
		_blocks.clear();
		_blocks.resize(Vector3iUtil::get_volume_u64(size));
		_size_in_blocks = size;
		_block_size = block_size;
	}

	inline bool is_valid_relative_block_position(Vector3i pos) const {
		return pos.x >= 0 && //
				pos.y >= 0 && //
				pos.z >= 0 && //
				pos.x < _size_in_blocks.x && //
				pos.y < _size_in_blocks.y && //
				pos.z < _size_in_blocks.z;
	}

	inline bool is_valid_block_position(Vector3i pos) const {
		return is_valid_relative_block_position(pos - _offset_in_blocks);
	}

	inline void set_block(Vector3i position, std::shared_ptr<VoxelBuffer> block) {
		VOXEL_ASSERT_RETURN(is_valid_block_position(position));
		position -= _offset_in_blocks;
		const unsigned int index = Vector3iUtil::get_zxy_index(position, _size_in_blocks);
		VOXEL_ASSERT(index < _blocks.size());
		_blocks[index] = block;
	}

	inline VoxelBuffer *get_block(Vector3i position) {
		VOXEL_ASSERT_RETURN_V(is_valid_block_position(position), nullptr);
		position -= _offset_in_blocks;
		const unsigned int index = Vector3iUtil::get_zxy_index(position, _size_in_blocks);
		VOXEL_ASSERT(index < _blocks.size());
		return _blocks[index].get();
	}

	// 按 ZXY 顺序索引的扁平网格
	// TODO 能使用线程局部/栈池分配器吗？这类网格往往是临时的
	StdVector<std::shared_ptr<VoxelBuffer>> _blocks;
	// 网格大小（以块为单位）
	Vector3i _size_in_blocks;
	// 块坐标偏移。当我们缓存地图的一个子区域时，需要在内存中保留区域的
	// 原点，以便继续使用相同的坐标空间
	Vector3i _offset_in_blocks;
	Vector3i _logical_offset_in_blocks;
	// 块大小（以体素为单位）
	unsigned int _block_size_po2 = constants::DEFAULT_BLOCK_SIZE_PO2;
	unsigned int _block_size = 1 << constants::DEFAULT_BLOCK_SIZE_PO2;
	// 用于防止体素数据被多线程并发访问。不拥有此锁。其生命周期必须由用户保证，
	// 例如通过持有空间锁的 std::shared_ptr<VoxelData>。
	SpatialLock3D *_spatial_lock = nullptr;
	mutable bool _locked = false;
};

} // namespace voxel

#endif // VOXEL_DATA_GRID_H
