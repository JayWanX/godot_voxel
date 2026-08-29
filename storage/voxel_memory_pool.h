#ifndef VOXEL_MEMORY_POOL_H
#define VOXEL_MEMORY_POOL_H

#include "../util/containers/fixed_array.h"
#ifdef DEBUG_ENABLED
#include "../util/containers/std_unordered_map.h"
#endif
#include "../util/containers/std_vector.h"
#include "../util/dstack.h"
#include "../util/math/funcs.h"
#include "../util/thread/mutex.h"

#include <atomic>
#include <limits>

namespace voxel {

// 基于"分配的块通常大小相同"这一场景的池。
// 为每个 2 的幂分配一个块池。
// 大多数 VoxelBuffer 使用 2 的幂，因此大多数时候
// 我们不会浪费内存。有时会创建非 2 的幂的缓冲区，
// 但它们往往是临时且数量较少的。
class VoxelMemoryPool {
private:
#ifdef DEBUG_ENABLED
	struct DebugUsedBlocks {
		Mutex mutex;
		StdUnorderedMap<void *, dstack::Info> blocks;

		void add(void *mem) {
			MutexLock lock(mutex);
			auto it = blocks.find(mem);
			// 不能重复添加
			VOXEL_ASSERT(it == blocks.end());
			blocks.insert({ mem, dstack::Info() });
		}

		void remove(void *block) {
			MutexLock lock(mutex);
			auto it = blocks.find(block);
			// 必须存在
			VOXEL_ASSERT(it != blocks.end());
			blocks.erase(it);
		}
	};
#endif

	struct Pool {
		Mutex mutex;
		// 链表会不会更好？
		StdVector<uint8_t *> blocks;
#ifdef DEBUG_ENABLED
		DebugUsedBlocks debug_used_blocks;
#endif
	};

public:
	static void create_singleton();
	static void destroy_singleton();
	static VoxelMemoryPool &get_singleton();

	VoxelMemoryPool();
	~VoxelMemoryPool();

	uint8_t *allocate(size_t size);
	void recycle(uint8_t *block, size_t size);

	void clear_unused_blocks();

	void debug_print();
	unsigned int debug_get_used_blocks() const;
	size_t debug_get_used_memory() const;
	size_t debug_get_total_memory() const;

private:
	void clear();

	inline size_t get_highest_supported_size() const {
		return size_t(1) << (_pot_pools.size() - 1);
	}

	inline unsigned int get_pool_index_from_size(size_t size) const {
#ifdef DEBUG_ENABLED
		// `get_next_power_of_two_32` 接受 unsigned int
		VOXEL_ASSERT(size <= std::numeric_limits<unsigned int>::max());
#endif
		return math::get_shift_from_power_of_two_32(math::get_next_power_of_two_32(size));
	}

	static inline size_t get_size_from_pool_index(unsigned int i) {
		return size_t(1) << i;
	}

#ifdef DEBUG_ENABLED
	void debug_print_used_blocks(unsigned int max_amount);
#endif

	// 我们处理的分配上限为 2^20 = 1,048,576 字节。
	// 这是根据实际需求选择的。
	// 该数组中的每个槽位对应包含 2^index 字节的分配。
	FixedArray<Pool, 21> _pot_pools;
#ifdef DEBUG_ENABLED
	DebugUsedBlocks _debug_nonpooled_used_blocks;
#endif

	std::atomic_uint32_t _used_blocks = { 0 };
	std::atomic_uint64_t _used_memory = { 0 };
	std::atomic_uint64_t _total_memory = { 0 };
};

} // namespace voxel

#endif // VOXEL_MEMORY_POOL_H
