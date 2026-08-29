#ifndef VOXEL_DATA_H
#define VOXEL_DATA_H

#include "../generators/voxel_generator.h"
#include "../streams/voxel_stream.h"
#include "../util/thread/mutex.h"
#include "../util/thread/spatial_lock_3d.h"
#include "voxel_data_map.h"
#include "voxel_format.h"

#ifdef VOXEL_ENABLE_MODIFIERS
#include "../modifiers/voxel_modifier_stack.h"
#endif

namespace voxel {

class VoxelDataGrid;

// 包含访问体素数据所需一切内容的通用存储。
// 包含编辑、程序化来源和数据流，因此可以获得未物理存储在内存中的体素。
// 这不包含网格生成或实例化信息，只包含体素。
// 各个调用应当是线程安全的。
class VoxelData {
public:
	VoxelData();
	~VoxelData();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 配置。
	// 在数据已加载时更改这些设置可能代价高昂，或导致数据被重置。
	// 如果线程任务在此期间仍在处理数据，它们应被取消或忽略。

	inline unsigned int get_block_size() const {
		return _lods[0].map.get_block_size();
	}

	inline unsigned int get_block_size_po2() const {
		return _lods[0].map.get_block_size_pow2();
	}

	inline Vector3i voxel_to_block(Vector3i pos) const {
		return _lods[0].map.voxel_to_block(pos);
	}

	inline Vector3i block_to_voxel(Vector3i pos) const {
		return _lods[0].map.block_to_voxel(pos);
	}

	void set_lod_count(unsigned int p_lod_count);

	// 清除体素数据。保留修改器、生成器和设置。
	void reset_maps();

	inline unsigned int get_lod_count() const {
		MutexLock rlock(_settings_mutex);
		return _lod_count;
	}

	void set_bounds(Box3i bounds);

	inline Box3i get_bounds() const {
		MutexLock rlock(_settings_mutex);
		return _bounds_in_voxels;
	}

	VoxelFormat get_format() const;
	void set_format(const VoxelFormat format);

	void set_generator(Ref<VoxelGenerator> generator);

	inline Ref<VoxelGenerator> get_generator() const {
		MutexLock rlock(_settings_mutex);
		return _generator;
	}

	void set_stream(Ref<VoxelStream> stream);

	inline Ref<VoxelStream> get_stream() const {
		MutexLock rlock(_settings_mutex);
		return _stream;
	}

#ifdef VOXEL_ENABLE_MODIFIERS
	inline VoxelModifierStack &get_modifiers() {
		return _modifiers;
	}

	inline const VoxelModifierStack &get_modifiers() const {
		return _modifiers;
	}
#endif

	void set_streaming_enabled(bool enabled);

	inline bool is_streaming_enabled() const {
		return _streaming_enabled;
	}

	void set_full_load_completed(bool complete);

	inline bool is_full_load_completed() const {
		return _full_load_completed;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Voxel 查询。
	// 未指定时，使用的 LOD 索引为 0。

	VoxelSingleValue get_voxel(Vector3i pos, unsigned int channel_index, VoxelSingleValue defval) const;
	bool try_set_voxel(uint64_t value, Vector3i pos, unsigned int channel_index);

	float get_voxel_f(Vector3i pos, unsigned int channel_index) const;
	bool try_set_voxel_f(const real_t value, const Vector3i pos, const unsigned int channel_index);

	// 从 LOD0 复制一个盒中的体素数据。
	// `channels_mask` 的位指示读取哪个通道。
	void copy(
			const Vector3i min_pos,
			VoxelBuffer &dst_buffer,
			const unsigned int channels_mask,
			const bool with_metadata
	) const;

	// 在 LOD0 粘贴一个盒中的体素数据。
	// `channels_mask` 的位指示粘贴哪个通道。
	// 如果使用 `use_mask`，将只写入源缓冲区中不等于 `mask_value` 的体素。
	// 若 `create_new_blocks` 为 true，区域中找不到的块将被创建。
	void paste(
			const Vector3i min_pos,
			const VoxelBuffer &src_buffer,
			const unsigned int channels_mask,
			const bool create_new_blocks,
			const bool with_metadata
	);

	void paste_masked(
			Vector3i min_pos,
			const VoxelBuffer &src_buffer,
			unsigned int channels_mask,
			uint8_t mask_channel,
			uint64_t mask_value,
			bool create_new_blocks
	);

	void paste_masked_writable_list(
			Vector3i min_pos,
			const VoxelBuffer &src_buffer,
			unsigned int channels_mask,
			uint8_t src_mask_channel,
			uint64_t src_mask_value,
			uint8_t dst_mask_channel,
			Span<const int32_t> dst_writable_values,
			bool create_new_blocks
	);

	// 测试给定区域是否已在 LOD0 加载。
	// 这对于破坏性编辑是必要的。
	bool is_area_loaded(const Box3i p_voxels_box) const;

	// 为编辑准备生成所有不存在的块。
	// 每个 LOD 中与盒相交的每个块都会被检查。
	// 该函数顺序运行，应为线程安全的。如果立即可用需要块，可以使用它。
	// 如果其他线程正在访问相同数据，它会阻塞。
	// 启用数据流时，不会触碰未加载的区域。
	// 警告：这不检查区域是否可编辑。
	void pre_generate_box(Box3i voxel_box);

	// 从纯属生成器和修改器结果的块中清除体素数据。
	// 警告：这不检查区域是否可编辑。
	// TODO 重命名为 `clear_cached_voxel_data_in_area`
	void clear_cached_blocks_in_voxel_area(Box3i p_voxel_box);

	// 将给定区域中的所有块标记为在 LOD0 已修改。
	// 同时将它们标记为需要 LOD 更新（若 LOD 数量为 1，则无效果）。
	// 可选地，返回先前不需要 LOD 更新的受影响块位置列表。
	void mark_area_modified(Box3i p_voxel_box, StdVector<Vector3i> *lod0_new_blocks_to_lod, bool require_lod_updates);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 块感知 API

	// 设置一个块的全部数据。
	// 若块已存在，返回 false。否则返回 true。
	bool try_set_block(Vector3i block_position, const VoxelDataBlock &block);

	// 设置一个块的全部数据。
	// 若块不存在，则添加它并返回 true。
	// 若块已存在，则调用 `action_when_exists` 并返回 false。
	// `void action_when_exists(VoxelDataBlock &existing_block, const VoxelDataBlock &incoming_block)`
	template <typename F>
	bool try_set_block(Vector3i block_position, const VoxelDataBlock &block, F action_when_exists) {
		Lod &lod = _lods[block.get_lod_index()];
#ifdef DEBUG_ENABLED
		if (block.has_voxels()) {
			VOXEL_ASSERT(block.get_voxels_const().get_size() == Vector3iUtil::create(get_block_size()));
		}
#endif
		RWLockWrite wlock(lod.map_lock);
		VoxelDataBlock *existing_block = lod.map.get_block(block_position);
		if (existing_block != nullptr) {
			action_when_exists(*existing_block, block);
			return false;
		} else {
			lod.map.set_block(block_position, block);
			return true;
		}
	}

	template <typename F>
	void for_each_block_position(F op) const {
		const unsigned int lod_count = get_lod_count();
		for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
			const Lod &lod = _lods[lod_index];
			RWLockRead rlock(lod.map_lock);
			lod.map.for_each_block_position(op);
		}
	}

	// void op(Vector3i bpos, const VoxelDataBlock &block)
	// template <typename F>
	// void for_each_block_r(F op) const {
	// 	const unsigned int lod_count = get_lod_count();
	// 	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
	// 		const Lod &lod = _lods[lod_index];
	// 		SpatialLock3D::Read srlock(lod.spatial_lock, BoxBounds3i::from_everywhere());
	// 		RWLockRead rlock(lod.map_lock);
	// 		lod.map.for_each_block(op);
	// 	}
	// }

	// void op(Vector3i bpos, const VoxelDataBlock &block)
	template <typename F>
	void for_each_block_at_lod_r(F op, unsigned int lod_index) const {
		const Lod &lod = _lods[lod_index];
		SpatialLock3D::Read srlock(lod.spatial_lock, BoxBounds3i::from_everywhere());
		RWLockRead rlock(lod.map_lock);
		lod.map.for_each_block(op);
	}

	// 测试指定块位置和 LOD 索引处是否存在块。
	// 这主要用于调试，因此不是最优的；若你要查询许多块，请不要使用它。
	bool has_block(Vector3i bpos, unsigned int lod_index) const;

	// 测试一个区域中的所有块是否都已加载。若任一未加载，返回 false。否则返回 true。
	// 会考虑数据边界，但因此较慢。
	bool has_all_blocks_in_area(Box3i data_blocks_box, unsigned int lod_index) const;

	// 测试一个区域中的所有块是否都已加载。若任一未加载，返回 false。否则返回 true。
	// 不考虑数据边界，因此若给定盒与外部重叠，将返回 false。
	bool has_all_blocks_in_area_unbound(Box3i data_blocks_box, unsigned int lod_index) const;

	// 获取已分配块的总数。包括没有体素数据的块。
	unsigned int get_block_count() const;

	struct BlockLocation {
		Vector3i position;
		uint32_t lod_index;
	};

	// 更新给定位置所有块的 LOD，并重置它们"需要 LOD 更新"的标志。
	// 可选地，返回受影响的块位置列表。
	void update_lods(Span<const Vector3i> modified_lod0_blocks, StdVector<BlockLocation> *out_updated_blocks);

	struct BlockToSave {
		std::shared_ptr<VoxelBuffer> voxels;
		Vector3i position;
		uint32_t lod_index;
	};

	// 卸载指定区域中的数据块。若其中一些已修改且 `to_save` 不为 null，它们的
	// 数据将被返回供调用方保存。
	void unload_blocks(Box3i bbox, unsigned int lod_index, StdVector<BlockToSave> *to_save);

	// 卸载 LOD0 指定位置的数据块。若其中一些已修改且 `to_save` 不为 null，
	// 它们的数据将被返回供调用方保存。
	// void unload_blocks(Span<const Vector3i> positions, StdVector<BlockToSave> *to_save);

	// 若指定 LOD0 位置处的块存在且已修改，则将其标记为未修改并返回其数据的
	// 副本以供保存。若有内容需要保存，返回 true。
	bool consume_block_modifications(Vector3i bpos, BlockToSave &out_to_save);

	// 将所有已修改块标记为未修改并返回其数据以供保存。若 `with_copy` 为 true，
	// 返回的数据将是副本，否则将引用体素数据。例如在即将退出时，优先使用引用。
	void consume_all_modifications(StdVector<BlockToSave> &to_save, bool with_copy);

	// 从给定块位置中获取缺失的块。
	// 警告：边界外的位置也会被视为缺失。
	// TODO 不要把边界外的位置视为缺失？这只是迁移旧代码的副产品。
	// 它不检查这一点，因为使用该函数的代码已经做了检查（虽然效率
	// 稍高一些，但仍然）。
	void get_missing_blocks(
			Span<const Vector3i> block_positions,
			unsigned int lod_index,
			StdVector<Vector3i> &out_missing
	) const;

	// 从以块坐标表示的给定区域中获取缺失的块。
	// 若区域与边界外部相交，将被裁剪。
	void get_missing_blocks(Box3i p_blocks_box, unsigned int lod_index, StdVector<Vector3i> &out_missing) const;

	// 获取以块坐标表示的给定区域中带有体素数据的块。
	// 体素数据引用被返回在一个足够大的数组中，以容纳该区域大小的网格。
	// 找到的块将被放置在一个按扁平网格（ZXY）计算的索引处。
	// 没有体素数据的条目将保持为 null。
	void get_blocks_with_voxel_data(
			Box3i p_blocks_box,
			unsigned int lod_index,
			Span<std::shared_ptr<VoxelBuffer>> out_blocks
	) const;

	// 获取指定 LOD 处带有体素的块，并将它们索引到网格中。这会查询与盒在
	// 指定 LOD 相交的每个位置，因此如果区域很大，你可能想先做一次粗略检查。
	// 警告：数据未加锁，你必须持有 VoxelData 的共享引用才能使用 SpatialLock3D。
	void get_blocks_grid(VoxelDataGrid &grid, Box3i box_in_voxels, unsigned int lod_index) const;

	// TODO 使用此访问器的区域不妨把它们的逻辑移到这个类中
	SpatialLock3D &get_spatial_lock(unsigned int lod_index) const;

	// 通过查找 LOD mip 来测试给定区域中是否存在已编辑的块。由于检查的粗略性质，
	// 它可能报告误报，但运行速度比完整测试快得多。这仅适用于使用
	// LOD mip 的体积（已编辑的块在直到最大 LOD 的每一级都有半分辨率的对应块）。

	bool has_blocks_with_voxels_in_area_broad_mip_test(Box3i box_in_voxels) const;

	// 访问特定块的体素。
	// 警告：调用前必须持有空间锁，直到你完成对这些块的处理为止。
	// 可能返回 null。
	std::shared_ptr<VoxelBuffer> try_get_block_voxels(Vector3i bpos);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reference-counted API（仅 LOD0）
	// 数据块有一个可选的引用计数。

	// 增加区域内已加载块的引用计数。
	// 返回块已加载的位置，以及缺失的位置。
	// 返回找到的块的浅拷贝（体素数据被引用）。
	// 仅在启用引用计数时使用，否则可能失败。
	void view_area(
			Box3i blocks_box,
			unsigned int lod_index,
			StdVector<Vector3i> *missing_blocks,
			StdVector<Vector3i> *found_blocks_positions,
			StdVector<VoxelDataBlock> *found_blocks
	);

	// 减少区域内已加载块的引用计数。计数降到零的块将被卸载。
	// 返回块被卸载的位置，以及缺失的位置。
	// 如果 `to_save` 不为 null 且某些被卸载的块包含修改，它们的数据也会被返回。
	// 仅在启用引用计数时使用，否则可能失败。
	void unview_area(
			Box3i blocks_box,
			unsigned int lod_index,
			// 实际被移除的块（某些区域可能没有块）
			StdVector<Vector3i> *removed_blocks,
			// 当调用方持有正在加载的块集合时使用缺失的块，以便取消它们
			StdVector<Vector3i> *missing_blocks,
			// 需要保存的块是那些含有未保存修改的块
			StdVector<BlockToSave> *to_save
	);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Metadata 查询。
	// 仅在 LOD0。

	void set_voxel_metadata(const Vector3i pos, const Variant &meta);
	Variant get_voxel_metadata(const Vector3i pos);

private:
	void reset_maps_no_settings_lock();

	struct Lod {
		// 已编辑和已缓存体素的存储。
		VoxelDataMap map;

		// 多线程访问策略：
		// - 先获取空间锁
		// - 在持有空间锁的同时获取地图锁，仅用于查询地图
		// 假设当其它元素被插入或移除时，哈希表值的地址保持稳定，那么这是安全的。
		// 如果确实还需要同时锁定两个 LOD，先锁定索引较低的那个，再锁定索引较高的。

		// 保护地图本身的锁，因为它使用哈希表。
		// 只有当地图被修改（添加或移除块）时，才应以写模式锁定该锁。
		// 否则可以以读模式锁定。
		// 在我们完成地图查询后即可解锁。
		mutable RWLock map_lock;
		// 在块中读写体素/元数据时应使用此锁。它以块坐标为空间单位。
		mutable SpatialLock3D spatial_lock;
	};

	static void pre_generate_box(
			Box3i voxel_box,
			Span<Lod> lods,
			unsigned int data_block_size,
			bool streaming,
			unsigned int lod_count,
			Ref<VoxelGenerator> generator,
#ifdef VOXEL_ENABLE_MODIFIERS
			VoxelModifierStack &modifiers,
#endif
			const VoxelFormat format
	);

	static inline std::shared_ptr<VoxelBuffer> try_get_voxel_buffer_with_lock(
			const Lod &data_lod,
			Vector3i block_pos,
			bool &out_generate
	) {
		RWLockRead rlock(data_lod.map_lock);
		const VoxelDataBlock *block = data_lod.map.get_block(block_pos);
		if (block == nullptr) {
			// 块不存在，所以除非未启用数据流，否则我们不知道它是否有编辑。
			return nullptr;
		}

		// 块存在，因此我们知道它是否有编辑。

		// TODO 线程安全：这种检查体素存在性的方式并不安全。
		// 如果修改器在同一区域内移动，在网格生成期间它可能会发生变化，
		// 因为它会使缓存数据失效（这不需要锁定地图，也不需要锁定 VoxelBuffer，
		// 因此没有同步在进行）。一种修复方法是实现空间锁。
		if (!block->has_voxels()) {
			out_generate = true;
			return nullptr;
		}
		return block->get_voxels_shared();
	}

	std::shared_ptr<VoxelBuffer> try_get_writable_voxel_buffer_assuming_spatial_lock(Lod &lod, const Vector3i bpos);

	// 每个 LOD 在坐标集中工作，索引越高，覆盖的体素范围越大（每次 2 倍）。
	// LOD 0 是已编辑数据的主要存储。更高的索引是"mip-map"。
	// 使用固定数组是因为最大 LOD 数量很小，并且不需要线程锁定。
	// 注意这些 LOD 不会自动更新，由类的使用者负责触发更新。
	//
	// TODO 优化：（低优先级）即使不使用 LOD，这也占用对象中超过 5Kb 的空间。
	// 每个 LOD 包含一个 RWLock（242 字节），*24 很快就累积起来了。
	// 一种解决方案是在构造函数中动态分配 LOD（LOD 的存在与否在构造后
	// 不需要改变，到目前为止也没有这种用例）。
	FixedArray<Lod, constants::MAX_LOD> _lods;

	// 体素可以存在的区域。
	// 注意，这些边界可能无法精确表示。体积基于 chunk，因此结果可能
	// 近似到最近的 chunk。
	Box3i _bounds_in_voxels;

	uint8_t _lod_count = 1;

	// 如果启用，某些数据块可以具有"未加载"和"已加载"状态。这意味着在从
	// stream 加载它们之前，我们无法假设它们的内容。如果禁用，所有编辑都
	// 加载在内存中，我们知道如果一个块没有被存储，可以使用生成器和修改器
	// 获取其数据。这主要改变这个类的使用方式，streaming 本身并不直接在此类中实现。
	bool _streaming_enabled = true;

	// 当数据流被禁用时，这里会说明所有数据是否已完成加载。
	// 这是因为*所有内容*都会被加载，我们无法通过查看单个块来预先判断
	// 哪些已加载、哪些未加载。
	bool _full_load_completed = false;

	// 程序化生成栈
#ifdef VOXEL_ENABLE_MODIFIERS
	VoxelModifierStack _modifiers;
#endif
	Ref<VoxelGenerator> _generator;

	// 持久化存储（文件）。
	Ref<VoxelStream> _stream;

	VoxelFormat _format;

	// 访问 settings 成员时应锁定此锁。
	// 如果同时需要其它锁（如体素地图锁），应始终在其之后锁定，以防止
	// 死锁。
	//
	// 它不是 RWLock，因为它可能被锁定非常短的时间（只读取小值）。
	// 相比之下，RWLock 在底层使用 `shared_timed_mutex`，无论怎样，
	// 以读模式锁定它内部都会锁定一个互斥锁。
	// 有时锁定可能需要更长时间，但这种情况很少发生，例如在更改 LOD 数量时。
	Mutex _settings_mutex;
};

} // namespace voxel

#endif // VOXEL_DATA_H
