#ifndef VOXEL_MESH_MAP_H
#define VOXEL_MESH_MAP_H

#include "../engine/voxel_engine.h"
#include "../util/containers/std_unordered_map.h"
#include "../util/containers/std_vector.h"
#include "../util/macros.h"

namespace voxel {

// 在无限稀疏的网格（即数据块，blocks）中存储网格和碰撞体。
template <typename MeshBlock_T>
class VoxelMeshMap {
public:
	VoxelMeshMap() : _last_accessed_block(nullptr) {}

	~VoxelMeshMap() {
		clear();
	}

	struct NoAction {
		inline void operator()(MeshBlock_T &block) {}
	};

	template <typename Action_T>
	void remove_block(Vector3i bpos, Action_T pre_delete) {
		if (_last_accessed_block && _last_accessed_block->position == bpos) {
			_last_accessed_block = nullptr;
		}
		auto it = _blocks_map.find(bpos);
		if (it != _blocks_map.end()) {
			const unsigned int i = it->second.index;
#ifdef DEBUG_ENABLED
			CRASH_COND(i >= _blocks.size());
#endif
			MeshBlock_T *block = _blocks[i];
			ERR_FAIL_COND(block == nullptr);
			pre_delete(*block);
			queue_free_mesh_block(block);
			remove_block_internal(it, i);
		}
	}

	MeshBlock_T *get_block(Vector3i bpos) {
		if (_last_accessed_block && _last_accessed_block->position == bpos) {
			return _last_accessed_block;
		}
		auto it = _blocks_map.find(bpos);
		if (it != _blocks_map.end()) {
#ifdef DEBUG_ENABLED
			const unsigned int i = it->second.index;
			CRASH_COND(i >= _blocks.size());
			MeshBlock_T *block = _blocks[i];
			CRASH_COND(block == nullptr); // 该映射不应包含空数据块
			CRASH_COND(it->second.block == nullptr);
#endif
			_last_accessed_block = it->second.block;
			return _last_accessed_block;
		}
		return nullptr;
	}

	const MeshBlock_T *get_block(Vector3i bpos) const {
		if (_last_accessed_block != nullptr && _last_accessed_block->position == bpos) {
			return _last_accessed_block;
		}
		auto it = _blocks_map.find(bpos);
		if (it != _blocks_map.end()) {
#ifdef DEBUG_ENABLED
			const unsigned int i = it->second.index;
			CRASH_COND(i >= _blocks.size());
			MeshBlock_T *block = _blocks[i];
			CRASH_COND(block == nullptr); // 该映射不应包含空数据块
			CRASH_COND(it->second.block == nullptr);
#endif
			// 此函数无法缓存 _last_accessed_block，因为它是 const 的，所以重复访问仍需再次哈希……
			return it->second.block;
		}
		return nullptr;
	}

	void set_block(Vector3i bpos, MeshBlock_T *block) {
		ERR_FAIL_COND(block == nullptr);
		CRASH_COND(bpos != block->position);
		if (_last_accessed_block == nullptr || _last_accessed_block->position == bpos) {
			_last_accessed_block = block;
		}
#ifdef DEBUG_ENABLED
		CRASH_COND(has_block(bpos));
#endif
		unsigned int i = _blocks.size();
		_blocks.push_back(block);
		_blocks_map.insert({ bpos, { block, i } });
	}

	bool has_block(Vector3i pos) const {
		//(_last_accessed_block != nullptr && _last_accessed_block->pos == pos) ||
		return _blocks_map.find(pos) != _blocks_map.end();
	}

	void clear() {
		for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
			MeshBlock_T *block = *it;
			if (block == nullptr) {
				ERR_PRINT("Unexpected nullptr in VoxelMap::clear()");
			} else {
				VOXEL_DELETE(block);
			}
		}
		_blocks.clear();
		_blocks_map.clear();
		_last_accessed_block = nullptr;
	}

	unsigned int get_block_count() const {
#ifdef DEBUG_ENABLED
		const unsigned int blocks_map_size = _blocks_map.size();
		CRASH_COND(_blocks.size() != blocks_map_size);
#endif
		return _blocks.size();
	}

	template <typename Op_T>
	inline void for_each_block(Op_T op) {
		for (MeshBlock_T *block : _blocks) {
#ifdef DEV_ENABLED
			CRASH_COND(block == nullptr);
#endif
			op(*block);
		}
	}

	template <typename Op_T>
	inline void for_each_block(Op_T op) const {
		for (const MeshBlock_T *block : _blocks) {
#ifdef DEV_ENABLED
			CRASH_COND(block == nullptr);
#endif
			op(*block);
		}
	}

private:
	struct MapItem {
		MeshBlock_T *block;
		// 数据块在向量存储中的索引
		unsigned int index;
	};

	void remove_block_internal(typename StdUnorderedMap<Vector3i, MapItem>::iterator rm_it, unsigned int index) {
		// TODO 如果映射中包含大量条目，`erase` 偶尔会非常慢（毫秒级）。
		// 这可能是由内部的重新哈希/扩容引起的。
		// 我们应该寻找更快的容器，或者减少条目的数量。

		// 此函数假定数据块已经被释放
		_blocks_map.erase(rm_it);

		MeshBlock_T *moved_block = _blocks.back();
#ifdef DEBUG_ENABLED
		CRASH_COND(index >= _blocks.size());
#endif
		_blocks[index] = moved_block;
		_blocks.pop_back();

		if (index < _blocks.size()) {
			auto moved_block_index_it = _blocks_map.find(moved_block->position);
			CRASH_COND(moved_block_index_it == _blocks_map.end());
			moved_block_index_it->second.index = index;
		}
	}

	static void queue_free_mesh_block(MeshBlock_T *block) {
		// 我们将其分散开是因为物理原因
		// TODO 在 ~MeshBlock_T() 中通过任务同时进行渲染和物理释放是否就足够了？
		struct FreeMeshBlockTask : public voxel::ITimeSpreadTask {
			void run(TimeSpreadTaskContext &ctx) override {
				VOXEL_DELETE(block);
			}
			MeshBlock_T *block = nullptr;
		};
		ERR_FAIL_COND(block == nullptr);
		FreeMeshBlockTask *task = VOXEL_NEW(FreeMeshBlockTask);
		task->block = block;
		VoxelEngine::get_singleton().push_main_thread_time_spread_task(task);
	}

private:
	// 数据块通过三维空间哈希存储。
	StdUnorderedMap<Vector3i, MapItem> _blocks_map;
	// 数据块存储在向量中，以便更快地遍历所有数据块。
	// 用例包括更新网格的变换。
	StdVector<MeshBlock_T *> _blocks;

	// 体素访问最常发生在连续区域，因此会访问相同的数据块。
	// 为避免过多的哈希操作，在哈希之前先检查该引用。
	mutable MeshBlock_T *_last_accessed_block;
};

} // namespace voxel

#endif // VOXEL_MESH_BLOCK_MAP_H
