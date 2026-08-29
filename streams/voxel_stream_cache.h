#ifndef VOXEL_STREAM_CACHE_H
#define VOXEL_STREAM_CACHE_H

#include "../storage/voxel_buffer.h"
#include "../util/containers/std_unordered_map.h"
#include "../util/memory/memory.h"
#include "../util/thread/rw_lock.h"

#ifdef VOXEL_ENABLE_INSTANCER
#include "instance_data.h"
#endif

namespace voxel {

// 体素数据流的内存数据库。
// 它允许缓存数据块，以便稍后更少频率地保存到文件系统，或快速重新加载最近的数据块。
class VoxelStreamCache {
public:
	struct Block {
		Vector3i position;
		int lod;

		// 缺少体素数据可能意味着两种情况：
		// - 体素数据已被删除（该用例尚未真正实现，但未来可能发生）
		// - 体素数据从未被保存过，因此应保持原样
		bool has_voxels = false;
		bool voxels_deleted = false;

		VoxelBuffer voxels;
#ifdef VOXEL_ENABLE_INSTANCER
		UniquePtr<InstanceBlockData> instances;
#endif

		Block() : voxels(VoxelBuffer::ALLOCATOR_POOL) {}
	};

	// 将缓存的数据块复制到提供的缓冲区中
	bool load_voxel_block(Vector3i position, uint8_t lod_index, VoxelBuffer &out_voxels);

	// 将提供的数据块存入缓存。缓存将拥有所提供数据的所有权。
	void save_voxel_block(Vector3i position, uint8_t lod_index, VoxelBuffer &voxels);

#ifdef VOXEL_ENABLE_INSTANCER
	// 将缓存数据复制到提供的指针中。若找到，将创建新实例。
	bool load_instance_block(Vector3i position, uint8_t lod_index, UniquePtr<InstanceBlockData> &out_instances);

	// 将提供的数据块存入缓存。缓存将拥有所提供数据的所有权。
	void save_instance_block(Vector3i position, uint8_t lod_index, UniquePtr<InstanceBlockData> instances);
#endif

	unsigned int get_indicative_block_count() const;

	template <typename F>
	void flush(F save_func) {
		_count = 0;
		for (unsigned int lod_index = 0; lod_index < _cache.size(); ++lod_index) {
			Lod &lod = _cache[lod_index];
			RWLockWrite wlock(lod.rw_lock);
			for (auto it = lod.blocks.begin(); it != lod.blocks.end(); ++it) {
				Block &block = it->second;
				save_func(block);
			}
			lod.blocks.clear();
		}
	}

private:
	struct Lod {
		// 不使用指向值的指针，因为 unordered_map 不会使指向值的指针失效
		StdUnorderedMap<Vector3i, Block> blocks;
		RWLock rw_lock;
	};

	FixedArray<Lod, constants::MAX_LOD> _cache;
	unsigned int _count = 0;
};

} // namespace voxel

#endif // VOXEL_STREAM_CACHE_H
