#ifndef VOXEL_GENERATOR_MULTIPASS_CB_STRUCTS_H
#define VOXEL_GENERATOR_MULTIPASS_CB_STRUCTS_H

#include "../../storage/voxel_buffer.h"
#include "../../util/containers/small_vector.h"
#include "../../util/containers/span.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include "../../util/math/vector2i.h"
#include "../../util/math/vector3i.h"
#include "../../util/ref_count.h"
#include "../../util/thread/mutex.h"
#include "../../util/thread/spatial_lock_2d.h"

#include <utility>

namespace voxel {

// 多 pass 生成内部使用的数据结构。

// 不得不把这些结构体移到 `VoxelGeneratorMultipassCB` 外面，因为添加 Godot 虚方法绑定
// 需要 #include `VoxelToolMultipassGenerator`，而它又已经包含前者以访问其嵌套结构体，
// 导致无法编译。因此我把嵌套结构体移到了类外面。


class IThreadedTask;

namespace VoxelGeneratorMultipassCBStructs {

// pass 数量上限很低，因为实际上不需要那么多，而且开销会很快变大
static constexpr int MAX_PASSES = 4;
static constexpr int MAX_PASS_EXTENT = 2;

// pass 在内部被拆分为子 pass：pass 0 有 1 个子 pass，其它 pass 各有 2 个子 pass。每个子 pass
// 都依赖前一个。这是为了覆盖可能修改邻居的 pass 的副作用。
static constexpr int MAX_SUBPASSES = MAX_PASSES * 2 - 1; // get_subpass_count_from_pass_count(MAX_PASSES);
// 我很乐意使用 constexpr 函数，但显然如果它是静态方法，除非把它移到类外面，
// 否则无法工作，而这不太方便

struct Block {
	VoxelBuffer voxels;

	// 若已设置，此任务将在数据块的生成完成时被调度。
	// 只有未调度、未运行的任务才能在这里被引用。
	// 这些任务也绝不能生成子任务。也就是说，它们只在位于此处期间由数据块拥有。
	// 换句话说，我们使用这个指针来防止反复请求列生成，因为一列可能有多个数据块请求。
	IThreadedTask *final_pending_task = nullptr;

	Block() : voxels(VoxelBuffer::ALLOCATOR_POOL) {}

	Block(Block &&other) : Block() {
		voxels = std::move(other.voxels);
		final_pending_task = other.final_pending_task;
	}

	Block &operator=(Block &&other) {
		voxels = std::move(other.voxels);
		final_pending_task = other.final_pending_task;
		return *this;
	}

	~Block() {
		VOXEL_ASSERT_RETURN_MSG(final_pending_task == nullptr, "Unhandled task leaked!");
	}
};

struct Column {
	RefCount viewers;
	// 最后一个直接在此数据块上执行的子 pass 的索引。
	// -1 表示数据块刚刚创建，尚未运行任何子 pass。
	int8_t subpass_index = -1;
	bool saving = false;
	bool loading = false;

	// 每个位为 1 表示在给定子 pass 有一个待处理的任务要处理此数据块。
	uint8_t pending_subpass_tasks_mask = 0;

	// 目前未使用，因为如果数据块被移出缓存或出于任何原因未保存，
	// 它可能会失去同步而我们无从得知。不过它会是一个不错的优化……
	//
	// 缓存了附近有多少数据块在有权访问此数据块（含本身）的情况下被处理。
	// 这是一个优化，可以减少数据块地图查询。之所以是数组，是因为可能
	// 某个子 pass 开始写入一个已完成上一个子 pass 但尚未开始下一个
	// 子 pass 的数据块。
	// FixedArray<uint8_t, MAX_SUBPASSES> subpass_iterations;

	// 垂直堆叠的数据块。除非加载或卸载，否则绝不能调整大小。
	// TODO 或许可以用一个不可调整大小的动态数组替代？
	StdVector<Block> blocks;

	// Column() {
	// 	fill(subpass_iterations, uint8_t(0));
	// }
};

struct Map {
	StdUnorderedMap<Vector2i, Column> columns;
	// 保护哈希表本身
	Mutex mutex;
	// 保护列
	mutable SpatialLock2D spatial_lock;

	~Map() {
		// 如果地图被销毁，我们就知道对它的最后一个引用被移除了，这意味着只有
		// 一个线程可以访问它，所以如果需要清理，我们可以在不锁定任何东西的情况下完成。

		// 最初我以为这里会重新调度仍在数据块中的任何任务指针，但那不可能发生，
		// 因为如果存在引用多 pass 生成器的任务，那么生成器（以及地图）就不会被销毁，
		// 我们也就不会走到这里。
		// 因此这里只有 ~Block 中的一个断言，用于确保这是数据块在
		// 处理这些任务之前从地图移除的唯一其它情况，或者在出现我们未预料的情况时警告我们。
		//
		// 也就是说，这几乎描述了一个循环引用。这个循环通常由 VoxelTerrain 打破，
		// 它控制着流式加载行为。如果地形被销毁，它必须告诉生成器卸载其缓存。
	}
};

struct Pass {
	// 使用上一个 pass 在生成区域周围加载多少数据块，以便在 pass 的效果跨越数据块时
	// 可以访问邻居。
	// 约束：第一个 pass 不能有依赖；其它 pass 必须有依赖。
	int8_t dependency_extents = 0;
};

// 生成器的内部状态。
struct Internal {
	// 仅用于生成目的的地图。它像缓存一样工作，这样我们就不必多次重新计算相同的 pass。
	// 理论上我们可以不缓存或不流式地生成数据块，但那意味着每个数据块请求都必须
	// 反复生成它周围的大量数据（列、邻居、邻居的邻居……），速度会慢得令人望而却步。
	// 作为缓存，它可以随时被删除（当然要小心线程安全）。
	Map map;

	// 参数：它们绝不应该改变。更改它们需要制作一个全新的实例。
	SmallVector<Pass, MAX_PASSES> passes;
	int column_base_y_blocks = -4;
	int column_height_blocks = 8;

	// 若为 `true`，表示生成器的配置已更改。意味着已经制作了一个新的 Internal 实例。
	// 现有任务可能仍使用旧实例完成工作，但结果将被丢弃。如果这些任务检查此布尔值，
	// 它们可以更快结束。
	bool expired = false;

	Internal() {
		// 至少 1 个 pass
		passes.push_back(Pass());
	}

	inline void copy_params(const Internal &other) {
		passes = other.passes;
		column_base_y_blocks = other.column_base_y_blocks;
		column_height_blocks = other.column_height_blocks;
	}
};

struct PassInput {
	// 3D 数据块网格，作为线性序列，ZXY 顺序
	Span<Block *> grid;
	Vector3i grid_size;
	// 网格左下角的位置。
	Vector3i grid_origin;
	// 主数据块或列在世界坐标中的位置。
	Vector3i main_block_position;
	int block_size = 0;
	int pass_index = 0;
};

} // namespace VoxelGeneratorMultipassCBStructs
} // namespace voxel

#endif // VOXEL_GENERATOR_MULTIPASS_CB_STRUCTS_H
