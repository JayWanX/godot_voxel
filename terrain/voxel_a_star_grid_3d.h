#ifndef VOXEL_A_STAR_GRID_3D_H
#define VOXEL_A_STAR_GRID_3D_H

#include "../storage/voxel_buffer.h"
#include "../storage/voxel_data.h"
#include "../util/a_star_grid_3d.h"
#include "../util/containers/dynamic_bitset.h"
#include "../util/containers/std_vector.h"
#include <atomic>

namespace voxel {

class VoxelTerrain;

class VoxelAStarGrid3DInternal : public AStarGrid3D {
public:
	VoxelAStarGrid3DInternal();

	// 直接引用 VoxelData 而不是使用 VoxelTool，因为这允许在后台线程任务中运行搜索。
	// VoxelTool 目前还不能在线程中使用，因为它持有一个指向地形节点的指针，该节点可能随时被删除。
	std::shared_ptr<VoxelData> data;

	void init_cache();

protected:
	bool is_solid(Vector3i pos) override;

private:
	// 我们为整个可寻路区域存储实心位（solid bits）缓存。
	// 为尽量减少对主体素数据的多线程访问，我们只在需要时加载所需的数据块位。

	struct Chunk {
		static const int SIZE_PO2 = 2;
		static const int SIZE = 1 << SIZE_PO2;
		static const int SIZE_MASK = SIZE - 1;

		// 4x4x4 位，ZXY 顺序
		uint64_t solid_bits = 0;

		inline bool get_solid_bit(Vector3i rel) const {
			const uint64_t i = Vector3iUtil::get_zxy_index(rel, Vector3i(SIZE, SIZE, SIZE));
			// const unsigned int i = rel.y + (rel.x << 2) + (rel.z << 4);
			return ((solid_bits >> i) & uint64_t(1)) != 0;
		}
	};

	// 缓存的三维位图
	StdVector<Chunk> _grid_cache;
	Vector3i _grid_cache_size;

	// 跟踪哪些数据块已加载
	DynamicBitset _grid_chunk_states;

	// 用于从主体素存储读取体素的临时缓冲区
	VoxelBuffer _voxel_buffer;
};

// 面向 Godot 的体素网格 A* 寻路 API。适用于块状地形。
class VoxelAStarGrid3D : public RefCounted {
	GDCLASS(VoxelAStarGrid3D, RefCounted)
public:
	// 目前只是基础实现。未来可能需要更多的配置和自定义。
	// 另外，它不会在查询之间缓存数据。

	// 设置寻路使用的地形
	void set_terrain(VoxelTerrain *node);

	// 寻路区域范围
	void set_region(Box3i region);
	Box3i get_region();

	// 同步寻路，返回路径体素坐标数组
	TypedArray<Vector3i> find_path(Vector3i from_position, Vector3i to_position);

	// 异步寻路及其状态查询
	void find_path_async(Vector3i from_position, Vector3i to_position);
	bool is_running_async() const;

	// 调试：获取寻路过程中访问过的位置
	TypedArray<Vector3i> debug_get_visited_positions() const;

private:
	TypedArray<Vector3i> find_path_internal(Vector3i from_position, Vector3i to_position);
#ifdef DEBUG_ENABLED
	void check_params(Vector3i from_position, Vector3i to_position);
#endif

	void _b_set_region(AABB aabb);
	AABB _b_get_region();
	void _b_on_async_search_completed(TypedArray<Vector3i> path);

	static void _bind_methods();

	VoxelAStarGrid3DInternal _path_finder;
	std::atomic_bool _is_running_async = { false };
};

} // namespace voxel

#endif // VOXEL_A_STAR_GRID_3D_H
