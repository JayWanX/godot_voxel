#include "voxel_stream_cache.h"

namespace voxel {

bool VoxelStreamCache::load_voxel_block(Vector3i position, uint8_t lod_index, VoxelBuffer &out_voxels) {
	const Lod &lod = _cache[lod_index];

	RWLockRead rlock(lod.rw_lock);

	auto it = lod.blocks.find(position);

	if (it == lod.blocks.end()) {
		// 不在缓存中，需要查询
		return false;

	} else {
		const Block &block = it->second;
		if (!block.has_voxels) {
			// 缓存中有数据块但没有体素数据
			return false;
		}
		// 已在缓存中，直接提供

		// 必须进行拷贝，因为缓存拥有其数据的所有权，
		// 而请求方需要我们去填充它提供的缓冲区
		block.voxels.copy_to(out_voxels, true);

		return true;
	}
}

void VoxelStreamCache::save_voxel_block(Vector3i position, uint8_t lod_index, VoxelBuffer &voxels) {
	Lod &lod = _cache[lod_index];
	RWLockWrite wlock(lod.rw_lock);
	auto it = lod.blocks.find(position);

	VOXEL_ASSERT_RETURN_MSG(
			!Vector3iUtil::is_empty_size(voxels.get_size()), "Saving voxel buffer with empty size is not expected. Bug?"
	);

	if (it == lod.blocks.end()) {
		// 尚未缓存，创建条目
		Block b;
		b.position = position;
		b.lod = lod_index;
		// TODO 优化：如果我们知道缓冲区未共享，就可以改用 move 操作
		voxels.copy_to(b.voxels, true);
		b.has_voxels = true;
		lod.blocks.insert(std::make_pair(position, std::move(b)));
		++_count;

	} else {
		// 已缓存，覆盖
		voxels.move_to(it->second.voxels);
		it->second.has_voxels = true;
	}
}

#ifdef VOXEL_ENABLE_INSTANCER

bool VoxelStreamCache::load_instance_block(
		Vector3i position,
		uint8_t lod_index,
		UniquePtr<InstanceBlockData> &out_instances
) {
	const Lod &lod = _cache[lod_index];
	lod.rw_lock.read_lock();
	auto it = lod.blocks.find(position);

	if (it == lod.blocks.end()) {
		// 不在缓存中，需要查询
		lod.rw_lock.read_unlock();
		return false;

	} else {
		// 已在缓存中，直接提供

		if (it->second.instances == nullptr) {
			out_instances = nullptr;

		} else {
			// 必须进行拷贝，因为缓存拥有其数据的所有权
			out_instances = make_unique_instance<InstanceBlockData>();
			it->second.instances->copy_to(*out_instances);
		}

		lod.rw_lock.read_unlock();
		return true;
	}
}

void VoxelStreamCache::save_instance_block(
		Vector3i position,
		uint8_t lod_index,
		UniquePtr<InstanceBlockData> instances
) {
	Lod &lod = _cache[lod_index];
	RWLockWrite wlock(lod.rw_lock);
	auto it = lod.blocks.find(position);

	if (it == lod.blocks.end()) {
		// 尚未缓存，创建条目
		Block b;
		b.position = position;
		b.lod = lod_index;
		b.instances = std::move(instances);
		lod.blocks.insert(std::make_pair(position, std::move(b)));
		++_count;

	} else {
		// 已缓存，覆盖
		it->second.instances = std::move(instances);
	}
}

#endif

unsigned int VoxelStreamCache::get_indicative_block_count() const {
	return _count;
}

} // namespace voxel
