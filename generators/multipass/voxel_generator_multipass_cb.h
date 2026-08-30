#ifndef VOXEL_GENERATOR_MULTIPASS_CB_H
#define VOXEL_GENERATOR_MULTIPASS_CB_H

#include "../../engine/ids.h"
#include "../../storage/voxel_buffer_gd.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/core/gdvirtual.h"
#include "../../util/math/box3i.h"
#include "../../util/math/vector2i.h"
#include "../../util/memory/memory.h"
#include "../../util/ref_count.h"
#include "../../util/thread/mutex.h"
#include "../voxel_generator.h"
#include "voxel_generator_multipass_cb_structs.h"
#include "voxel_tool_multipass_generator.h" // 必须包含，以便我们能够定义 GDVIRTUAL 方法

namespace voxel {

// TODO 防止在多块地形上共享使用，或想办法支持它
// TODO 防止在 VoxelLodTerrain 上使用，或让其返回空数据块

// 运行多个相互依赖的 pass 的生成器。
// 它适用于在地形上生成树木等小型和中等规模的建筑结构。
// 与大多数其它生成器不同，它持有需要感知地形观察者的状态（缓存）。
// 它内部也是基于列（CB）的，因此可以处理垂直堆叠的数据块，而不是一次处理单个数据块。
class VoxelGeneratorMultipassCB : public VoxelGenerator {
	GDCLASS(VoxelGeneratorMultipassCB, VoxelGenerator)
public:
	static constexpr int MAX_PASSES = VoxelGeneratorMultipassCBStructs::MAX_PASSES;
	static constexpr int MAX_PASS_EXTENT = VoxelGeneratorMultipassCBStructs::MAX_PASS_EXTENT;

	static constexpr int MAX_SUBPASSES = VoxelGeneratorMultipassCBStructs::MAX_SUBPASSES;
	// 我很乐意使用 constexpr 函数，但显然如果它是静态方法，除非把它移到类外面，
	// 否则无法工作，而这不太方便

	// 合理的最大安全值。
	// 供参考，Minecraft 为 24 个数据块高（384 个体素）
	static constexpr int MAX_COLUMN_HEIGHT_BLOCKS = 32;

	// 由 pass 数量计算子 pass 数量
	static inline int get_subpass_count_from_pass_count(int pass_count) {
		return pass_count * 2 - 1;
	}

	// 由子 pass 索引计算所在 pass 索引
	static inline int get_pass_index_from_subpass(int subpass_index) {
		return (subpass_index + 1) / 2;
	}

	VoxelGeneratorMultipassCB();
	~VoxelGeneratorMultipassCB();

	// 是否支持 LOD
	bool supports_lod() const override {
		return false;
	}

	// 生成单个数据块的体素数据
	Result generate_block(VoxelQueryData input) override;
	// 获取生成器使用的通道掩码
	int get_used_channels_mask() const override;

	// 创建用于异步生成数据块的任务
	IThreadedTask *create_block_task(const VoxelGenerator::BlockTaskParams &params) const override;

	// 在数据块网格上执行一次 pass。
	// 网格包含一列中心数据块，主要处理必须在这里发生。
	// 它被邻居包围，如果主要处理部分影响到它们（结构相交、光照扩散……），则可以访问这些邻居。
	// "主要处理"每列每个 pass 只调用一次。
	// 网格中的每个数据块都保证至少被前面的 pass 处理过。
	// 然而，邻居数据块可能已经被当前 pass 处理过（无论是否"主要"），这是能够修改邻居的副作用。
	// 这就是处理种子生成时需要小心的地方：如果两个相邻数据块都生成相互重叠的内容，
	// 其中一个必然在另一个之前生成。
	// 由于多线程和玩家移动，该顺序是不可预测的。因此由你负责确保
	// 每种顺序都能产生相同的结果。如果不这样做，可能不是大问题，但同一世界生成两次
	// 将因此产生一些差异。
	virtual void generate_pass(VoxelGeneratorMultipassCBStructs::PassInput input);

	// pass 数量
	int get_pass_count() const;
	void set_pass_count(int pass_count);

	// 列的基础 Y 坐标（数据块为单位）
	int get_column_base_y_blocks() const;
	void set_column_base_y_blocks(int new_y);

	// 列的高度（数据块为单位）
	int get_column_height_blocks() const;
	void set_column_height_blocks(int new_height);

	// 指定 pass 的扩展范围（数据块为单位）
	int get_pass_extent_blocks(int pass_index) const;
	void set_pass_extent_blocks(int pass_index, int new_extent);

	// 运行生成器从零获取特定的一列，使用单线程以便更好地调试脚本
	// （因为编写本文时，Godot 4 仍不支持在不同线程中调试脚本）。这不使用内部缓存，可能极慢。
	TypedArray<godot::VoxelBuffer> debug_generate_test_column(Vector2i column_position_blocks);

	// 内部

	// 获取内部状态
	std::shared_ptr<VoxelGeneratorMultipassCBStructs::Internal> get_internal() const;

	// 当观察者与地形配对、移动或解除配对时必须调用。
	// 配对时应发送一个空的先前盒子。
	// 移动时应发送先前盒子和新盒子。
	// 解除配对时应发送一个空盒子作为当前盒子。
	void process_viewer_diff(ViewerID viewer_id, Box3i p_requested_box, Box3i p_prev_requested_box) override;

	// 清空内部缓存
	void clear_cache() override;

	// 当前配置是否可运行
	bool is_runnable() const override;

	// 编辑器

#ifdef TOOLS_ENABLED
	// 获取编辑器中显示的操作警告
	void get_configuration_warnings(PackedStringArray &out_warnings) const override;
#endif

	struct DebugColumnState {
		Vector2i position;
		int8_t subpass_index;
		uint8_t viewer_count;
	};

	// 尝试获取各列当前的子 pass 与观察者状态（调试用）
	bool debug_try_get_column_states(StdVector<DebugColumnState> &out_states);

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	GDVIRTUAL2(_generate_pass, Ref<VoxelToolMultipassGenerator>, int)

	// 在列区域之外调用，因此可以定义什么会在顶部和底部之外生成
	GDVIRTUAL2(_generate_block_fallback, Ref<godot::VoxelBuffer>, Vector3i)

	GDVIRTUAL0RC(int, _get_used_channels_mask)

private:
	void process_viewer_diff_internal(Box3i p_requested_box, Box3i p_prev_requested_box);
	void re_initialize_column_refcounts();
	void generate_block_fallback_script(VoxelQueryData &input);

	// 每次 pass 的结构发生变化（pass 数量、范围）时都必须调用
	template <typename F>
	void reset_internal(F f) {
		std::shared_ptr<VoxelGeneratorMultipassCBStructs::Internal> old_internal = get_internal();

		std::shared_ptr<VoxelGeneratorMultipassCBStructs::Internal> new_internal =
				make_shared_instance<VoxelGeneratorMultipassCBStructs::Internal>();

		new_internal->copy_params(*old_internal);

		f(*new_internal);

		{
			MutexLock mlock(_internal_mutex);
			_internal = new_internal;
			old_internal->expired = true;
		}

		// 注意，重置缓存也意味着我们丢失了列中所有的观察者引用计数，现在可能不同了。
		// 例如，如果 pass 数量或范围发生了变化，观察者将需要引用更大的区域。
		// 根据上下文，我们可能会调用 `re_initialize_column_refcounts()`。

		// 替代方案：
		// - 票据计数器。锁定整个地图（或某个任务运行时也锁定的共享互斥锁）并递增，
		// 然后如果票据不匹配，任何进行中的任务都被禁止修改地图中的任何内容。

		// 问题：
		// 这种生成器很可能与脚本一起使用。但和经典的 VoxelGeneratorScript 一样，
		// 在编辑器中当脚本在线程中执行时修改它是完全不安全的……
	}

	static void _bind_methods();

	struct PairedViewer {
		ViewerID id;
		Box3i request_box;
	};

	// 跟踪观察者以解决一些边界情况。
	// 这很令人沮丧，因为它与 VoxelTerrain 绑定，并且涉及我们通常
	// 在游戏过程中永远不会遇到的使用场景。例如，如果用户在地形加载时在编辑器中更改 pass 数量，
	// 缓存中观察者需要加载的引用计数区域必须变大，而我们无法在不访问已配对观察者列表的情况下做到这一点……
	// 如果不这样做，并且用户不重新生成地形，那么四处移动将开始导致一连串失败的生成请求，
	// 因为部分生成的列的缓存将不会处于正确的状态……
	StdVector<PairedViewer> _paired_viewers;

	// 线程可能非常繁忙地处理这个数据结构。然而在编辑器中，用户可以随时修改其参数，
	// 这可能会破坏一切。因此，当任何参数发生变化时，会制作此结构的副本，
	// 旧的副本从此处解除引用（并在最后一个线程处理完后被丢弃）。这会在设置属性时
	// 产生相当多的开销，但这应该主要发生在编辑器和资源加载中，
	// 绝不会发生在游戏进行中。
	std::shared_ptr<VoxelGeneratorMultipassCBStructs::Internal> _internal;
	Mutex _internal_mutex;
};

} // namespace voxel

#endif // VOXEL_GENERATOR_MULTIPASS_CB_H