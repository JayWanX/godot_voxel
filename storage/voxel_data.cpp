#include "voxel_data.h"
#include "../util/containers/std_vector.h"
#include "../util/dstack.h"
#include "../util/math/conv.h"
#include "../util/string/format.h"
#include "../util/thread/mutex.h"
#include "metadata/voxel_metadata_variant.h"
#include "voxel_buffer_gd.h"
#include "voxel_data_grid.h"

namespace voxel {

namespace {
struct BeforeUnloadSaveAction {
	StdVector<VoxelData::BlockToSave> *to_save;
	Vector3i position;
	unsigned int lod_index;

	inline void operator()(VoxelDataBlock &block) {
		if (block.is_modified()) {
			// 若已修改的数据块没有体素，则相当于从数据流中移除该数据块
			VoxelData::BlockToSave b;
			b.position = position;
			b.lod_index = lod_index;
			if (block.has_voxels()) {
				// 无需复制，因为该数据块反正会被移除
				b.voxels = block.get_voxels_shared();
			}
			to_save->push_back(b);
		}
	}
};

struct ScheduleSaveAction {
	StdVector<VoxelData::BlockToSave> &blocks_to_save;
	uint8_t lod_index;
	bool with_copy;

	void operator()(const Vector3i &bpos, VoxelDataBlock &block) {
		if (block.is_modified()) {
			// print_line(String("Scheduling save for block {0}").format(varray(block->position.to_vec3())));
			VoxelData::BlockToSave b;
			// 若已修改的数据块没有体素，则相当于从数据流中移除该数据块
			if (block.has_voxels()) {
				if (with_copy) {
					b.voxels = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
					block.get_voxels_const().copy_to(*b.voxels, true);
				} else {
					b.voxels = block.get_voxels_shared();
				}
			}
			b.position = bpos;
			b.lod_index = lod_index;
			blocks_to_save.push_back(b);
			block.set_modified(false);
		}
	}
};
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VoxelData::VoxelData() {}
VoxelData::~VoxelData() {}

void VoxelData::set_lod_count(unsigned int p_lod_count) {
	VOXEL_ASSERT(p_lod_count < constants::MAX_LOD);
	VOXEL_ASSERT(p_lod_count >= 1);

	// 由于要重置地图，此锁可能持有更长时间，但这非常罕见。
	// 在游戏中它仅在启动时使用一次。
	// 在编辑器中更频繁些，但依然足够罕见，偶尔卡顿也无妨。
	MutexLock wlock(_settings_mutex);

	if (p_lod_count == _lod_count) {
		return;
	}

	_lod_count = p_lod_count;

	// 并非必需，但运行时更改 LOD 数量很少需要
	reset_maps_no_settings_lock();
}

void VoxelData::reset_maps() {
	MutexLock wlock(_settings_mutex);
	reset_maps_no_settings_lock();
}

void VoxelData::reset_maps_no_settings_lock() {
	for (unsigned int lod_index = 0; lod_index < _lods.size(); ++lod_index) {
		Lod &data_lod = _lods[lod_index];

		// 擦除元素需要对每个数据块具有独占访问权。
		// 即不能有其他线程持有指向地图中数据块的指针。
		SpatialLock3D::Write swlock(data_lod.spatial_lock, BoxBounds3i::from_everywhere());

		RWLockWrite wlock(data_lod.map_lock);

		// 若 LOD 数量更多则新建地图，否则清空它们
		if (lod_index < _lod_count) {
			data_lod.map.create(lod_index);
		} else {
			data_lod.map.clear();
		}
	}
}

void VoxelData::set_bounds(Box3i bounds) {
	MutexLock wlock(_settings_mutex);
	_bounds_in_voxels = bounds;
}

void VoxelData::set_generator(Ref<VoxelGenerator> generator) {
	MutexLock wlock(_settings_mutex);
	_generator = generator;
}

VoxelFormat VoxelData::get_format() const {
	MutexLock rlock(_settings_mutex);
	return _format;
}

void VoxelData::set_format(const VoxelFormat format) {
	MutexLock rlock(_settings_mutex);
	if (format == _format) {
		return;
	}
	// 注意：更改格式通常意味着重新加载全部数据。即使我们锁定了设置，也最好在
	// 没有后台任务运行时进行此更改。
	_format = format;
	for (Lod &lod : _lods) {
		lod.map.set_format(_format);
	}
	reset_maps_no_settings_lock();
}

void VoxelData::set_stream(Ref<VoxelStream> stream) {
	MutexLock wlock(_settings_mutex);
	_stream = stream;
}

void VoxelData::set_streaming_enabled(bool enabled) {
	_streaming_enabled = enabled;
}

void VoxelData::set_full_load_completed(bool complete) {
	// 可由其他线程设置
	_full_load_completed = complete;
}

inline VoxelSingleValue get_voxel_sv(VoxelBuffer &vb, Vector3i pos, unsigned int channel) {
	VoxelSingleValue v;
	if (channel == VoxelBuffer::CHANNEL_SDF) {
		v.f = vb.get_voxel_f(pos.x, pos.y, pos.z, channel);
	} else {
		v.i = vb.get_voxel(pos, channel);
	}
	return v;
}

// TODO 复用 `copy`？其实现相当复杂，并且本就不应作为高效用例
VoxelSingleValue VoxelData::get_voxel(Vector3i pos, unsigned int channel_index, VoxelSingleValue defval) const {
	// VOXEL_PROFILE_SCOPE();

	if (!_bounds_in_voxels.contains(pos)) {
		return defval;
	}

	Vector3i block_pos = pos >> get_block_size_po2();
	bool generate = false;

	if (!_streaming_enabled) {
		if (_full_load_completed == false) {
			return defval;
		}

		const Lod &data_lod0 = _lods[0];

		data_lod0.spatial_lock.lock_read(BoxBounds3i::from_position(block_pos));

		std::shared_ptr<VoxelBuffer> voxels = try_get_voxel_buffer_with_lock(data_lod0, block_pos, generate);

		if (voxels == nullptr) {
			data_lod0.spatial_lock.unlock_read(BoxBounds3i::from_position(block_pos));

			// 没有体素数据。未使用数据流时我们知道一切均已加载，因此尝试直接生成。
			// TODO 若使用了修改器但没有基础生成器，我们也应能获取到值
			Ref<VoxelGenerator> generator = get_generator();
			if (generator.is_valid()) {
				VoxelSingleValue value = generator->generate_single(pos, channel_index);
#ifdef VOXEL_ENABLE_MODIFIERS
				if (channel_index == VoxelBuffer::CHANNEL_SDF) {
					float sdf = value.f;
					_modifiers.apply(sdf, to_vec3f(pos));
					value.f = sdf;
				}
#endif
				return value;
			}
		} else {
			const Vector3i rpos = data_lod0.map.to_local(pos);
			const VoxelSingleValue sv = get_voxel_sv(*voxels, rpos, channel_index);
			data_lod0.spatial_lock.unlock_read(BoxBounds3i::from_position(block_pos));
			return sv;
		}
		return defval;

	} else {
		// 使用数据流时，我们尝试查找体素数据。若找不到且该位置也未加载，我们
		// 只能返回默认值。
		Vector3i voxel_pos = pos;
		Ref<VoxelGenerator> generator = get_generator();
		const unsigned int lod_count = get_lod_count();

		// 遍历所有 LOD，直到找到已加载的位置
		for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
			const Lod &data_lod = _lods[lod_index];

			data_lod.spatial_lock.lock_read(BoxBounds3i::from_position(block_pos));

			std::shared_ptr<VoxelBuffer> voxels = try_get_voxel_buffer_with_lock(data_lod, block_pos, generate);

			if (voxels != nullptr) {
				const VoxelSingleValue sv = get_voxel_sv(*voxels, data_lod.map.to_local(voxel_pos), channel_index);
				data_lod.spatial_lock.unlock_read(BoxBounds3i::from_position(block_pos));
				return sv;

			} else {
				data_lod.spatial_lock.unlock_read(BoxBounds3i::from_position(block_pos));

				if (generate) {
					// TODO 若使用了修改器但没有基础生成器，我们也应能获取到值
					if (generator.is_valid()) {
						VoxelSingleValue value = generator->generate_single(pos, channel_index);
#ifdef VOXEL_ENABLE_MODIFIERS
						if (channel_index == VoxelBuffer::CHANNEL_SDF) {
							float sdf = value.f;
							_modifiers.apply(sdf, to_vec3f(pos));
							value.f = sdf;
						}
#endif
						return value;
					} else {
						return defval;
					}
				}
			}

			// 回退到更低的 LOD
			block_pos = block_pos >> 1;
			voxel_pos = voxel_pos >> 1;
		}
		return defval;
	}
}

std::shared_ptr<VoxelBuffer> VoxelData::try_get_writable_voxel_buffer_assuming_spatial_lock(
		Lod &lod,
		const Vector3i bpos
) {
	bool can_generate = false;
	std::shared_ptr<VoxelBuffer> voxels = try_get_voxel_buffer_with_lock(lod, bpos, can_generate);

	if (voxels == nullptr) {
		// 体素不在内存中有多种原因

		if ((_streaming_enabled && !can_generate) || (!_streaming_enabled && !_full_load_completed)) {
			// 我们不知道数据块里实际是什么，它未加载，无法编辑。
			return voxels;
		}
		// 该数据块要么已加载，要么数据流已关闭（一切均已加载），所以无论如何我们要编辑的数据块
		// 都是已知的

		voxels = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
		voxels->create(Vector3iUtil::create(get_block_size()), &lod.map.get_format());

		Ref<VoxelGenerator> generator = get_generator();
		if (generator.is_valid()) {
			VoxelGenerator::VoxelQueryData q{ *voxels, bpos << get_block_size_po2(), 0 };
			generator->generate_block(q);
#ifdef VOXEL_ENABLE_MODIFIERS
			_modifiers.apply(q.voxel_buffer, AABB(q.origin_in_voxels, q.voxel_buffer.get_size()));
#endif
		}

		RWLockWrite wlock(lod.map_lock);
		// 生成期间没有其他线程可以修改此区域，因为我们持有空间锁。

		lod.map.set_block_buffer(bpos, voxels, true);
	}

	return voxels;
}

// TODO 复用 `paste`？其实现相当复杂，并且本就不应作为高效用例
bool VoxelData::try_set_voxel(uint64_t value, Vector3i pos, unsigned int channel_index) {
	Lod &data_lod0 = _lods[0];
	const Vector3i block_pos_lod0 = data_lod0.map.voxel_to_block(pos);

	SpatialLock3D::Write swlock(data_lod0.spatial_lock, BoxBounds3i::from_position(block_pos_lod0));

	std::shared_ptr<VoxelBuffer> voxels =
			try_get_writable_voxel_buffer_assuming_spatial_lock(data_lod0, block_pos_lod0);
	if (voxels == nullptr) {
		return false;
	}

	voxels->set_voxel(value, data_lod0.map.to_local(pos), channel_index);
	// 我们不更新 mip，这必须由调用方完成
	return true;
}

float VoxelData::get_voxel_f(Vector3i pos, unsigned int channel_index) const {
	VoxelSingleValue defval;
	defval.f = constants::SDF_FAR_OUTSIDE;
	return get_voxel(pos, channel_index, defval).f;
}

bool VoxelData::try_set_voxel_f(const real_t value, const Vector3i pos, const unsigned int channel_index) {
	const uint64_t raw = VoxelBuffer::real_to_raw_voxel(value, _format.depths[channel_index]);
	return try_set_voxel(raw, pos, channel_index);
}

void VoxelData::copy(
		const Vector3i min_pos,
		VoxelBuffer &dst_buffer,
		const unsigned int channels_mask,
		const bool with_metadata
) const {
	VOXEL_PROFILE_SCOPE();

#ifdef DEBUG_ENABLED
	if (channels_mask == 0) {
		VOXEL_PRINT_WARNING("copy was called with empty channel mask, nothing will be copied");
		return;
	}
#endif

	const Lod &data_lod0 = _lods[0];
#ifdef VOXEL_ENABLE_MODIFIERS
	const VoxelModifierStack &modifiers = _modifiers;
#endif

	Ref<VoxelGenerator> generator = get_generator();

	// 我们本可以假定传入的缓冲区已具备正确格式，但那需要改动更多
	// 地方
	get_format().configure_buffer(dst_buffer);

	const Box3i blocks_box = Box3i(min_pos, dst_buffer.get_size()).downscaled(data_lod0.map.get_block_size());
	SpatialLock3D::Read srlock(data_lod0.spatial_lock, BoxBounds3i(blocks_box));

	if (generator.is_null()) {
		RWLockRead rlock(data_lod0.map_lock);
		// 仅获取我们有体素数据的数据块，其余数据块将是空气。
		// TODO 修改器？
		data_lod0.map.copy(min_pos, dst_buffer, channels_mask, with_metadata);

	} else {
		struct GenContext {
			VoxelGenerator &generator;
#ifdef VOXEL_ENABLE_MODIFIERS
			const VoxelModifierStack &modifiers;
#endif
			Box3i voxel_bounds;
		};

		GenContext gctx{ **generator,
#ifdef VOXEL_ENABLE_MODIFIERS
						 modifiers,
#endif
						 _bounds_in_voxels };

		// 注意，启用数据流且与未加载区域相交时，这些区域将回退到生成器。
		// 严格来说这不正确，因为我们并不真正知道这些区域应包含什么，它们可能已被
		// 编辑。调用方最好先检查该区域是否已加载。若所有这些操作能在单个事务中完成会更
		// 好？最终或许需要一个合适的事务 API

		RWLockRead rlock(data_lod0.map_lock);
		data_lod0.map.copy(
				min_pos,
				dst_buffer,
				channels_mask,
				&gctx,
				// 在数据块未被编辑的区域即时生成
				[](void *callback_data, VoxelBuffer &voxels, Vector3i pos) {
					// 以 `2` 作后缀是因为 GCC 警告其遮蔽了先前的局部变量...
					GenContext *gctx2 = reinterpret_cast<GenContext *>(callback_data);
					if (!gctx2->voxel_bounds.contains(pos)) {
						// 越界时，产生空体素？
						// 注意：由于 `copy` 的工作方式，我们预期 `pos` 在特定 chunk 内，且不会
						// 跨多个 chunk 复制，因此我们不必检查每个相交的 chunk
						return;
					}
					VOXEL_PROFILE_SCOPE_NAMED("Generate");
					VoxelGenerator::VoxelQueryData q{ voxels, pos, 0 };
					gctx2->generator.generate_block(q);
#ifdef VOXEL_ENABLE_MODIFIERS
					gctx2->modifiers.apply(voxels, AABB(pos, voxels.get_size()));
#endif
				},
				with_metadata
		);
	}
}

void VoxelData::paste(
		const Vector3i min_pos,
		const VoxelBuffer &src_buffer,
		const unsigned int channels_mask,
		const bool create_new_blocks,
		const bool with_metadata
) {
	VOXEL_PROFILE_SCOPE();

	Lod &data_lod0 = _lods[0];

	const Box3i blocks_box = Box3i(min_pos, src_buffer.get_size()).downscaled(data_lod0.map.get_block_size());
	SpatialLock3D::Write swlock(data_lod0.spatial_lock, BoxBounds3i(blocks_box));

	if (create_new_blocks) {
		// 我们将修改哈希表，因此期间其他线程无法进行查找
		RWLockWrite wlock(data_lod0.map_lock);
		data_lod0.map.paste(min_pos, src_buffer, channels_mask, create_new_blocks, with_metadata);
	} else {
		// 我们不会修改哈希表，因此其他线程仍可在不同区域进行查找
		RWLockRead rlock(data_lod0.map_lock);
		data_lod0.map.paste(min_pos, src_buffer, channels_mask, create_new_blocks, with_metadata);
	}
}

void VoxelData::paste_masked(
		Vector3i min_pos,
		const VoxelBuffer &src_buffer,
		unsigned int channels_mask,
		uint8_t mask_channel,
		uint64_t mask_value,
		bool create_new_blocks
) {
	VOXEL_PROFILE_SCOPE();

	Lod &data_lod0 = _lods[0];

	const Box3i blocks_box = Box3i(min_pos, src_buffer.get_size()).downscaled(data_lod0.map.get_block_size());
	SpatialLock3D::Write swlock(data_lod0.spatial_lock, BoxBounds3i(blocks_box));

	const bool with_metadata = true;

	if (create_new_blocks) {
		// 我们将修改哈希表，因此期间其他线程无法进行查找
		RWLockWrite wlock(data_lod0.map_lock);
		data_lod0.map.paste_masked(
				min_pos,
				src_buffer,
				channels_mask,
				true,
				mask_channel,
				mask_value,
				false, // 未使用的目标掩码
				0,
				Span<const int32_t>(),
				create_new_blocks,
				with_metadata
		);
	} else {
		// 我们不会修改哈希表，因此其他线程仍可在不同区域进行查找
		RWLockRead rlock(data_lod0.map_lock);
		data_lod0.map.paste_masked(
				min_pos,
				src_buffer,
				channels_mask,
				true,
				mask_channel,
				mask_value,
				false, // 未使用的目标掩码
				0,
				Span<const int32_t>(),
				create_new_blocks,
				with_metadata
		);
	}
}

void VoxelData::paste_masked_writable_list(
		Vector3i min_pos,
		const VoxelBuffer &src_buffer,
		unsigned int channels_mask,
		uint8_t src_mask_channel,
		uint64_t src_mask_value,
		uint8_t dst_mask_channel,
		Span<const int32_t> dst_writable_values,
		bool create_new_blocks
) {
	Lod &data_lod0 = _lods[0];

	const Box3i blocks_box = Box3i(min_pos, src_buffer.get_size()).downscaled(data_lod0.map.get_block_size());
	SpatialLock3D::Write swlock(data_lod0.spatial_lock, BoxBounds3i(blocks_box));

	const bool with_metadata = true;

	if (create_new_blocks) {
		// 我们将修改哈希表，因此期间其他线程无法进行查找
		RWLockWrite wlock(data_lod0.map_lock);
		data_lod0.map.paste_masked(
				min_pos,
				src_buffer,
				channels_mask,
				true,
				src_mask_channel,
				src_mask_value,
				true,
				dst_mask_channel,
				dst_writable_values,
				create_new_blocks,
				with_metadata
		);
	} else {
		// 我们不会修改哈希表，因此其他线程仍可在不同区域进行查找
		RWLockRead rlock(data_lod0.map_lock);
		data_lod0.map.paste_masked(
				min_pos,
				src_buffer,
				channels_mask,
				true,
				src_mask_channel,
				src_mask_value,
				true,
				dst_mask_channel,
				dst_writable_values,
				create_new_blocks,
				with_metadata
		);
	}
}

bool VoxelData::is_area_loaded(const Box3i p_voxels_box) const {
	if (is_streaming_enabled() == false) {
		return _full_load_completed;
	}
	const Box3i voxel_box = p_voxels_box.clipped(get_bounds());
	const Box3i block_box = voxel_box.downscaled(get_block_size());
	const Lod &data_lod0 = _lods[0];
	{
		SpatialLock3D::Read srlock(data_lod0.spatial_lock, block_box);

		RWLockRead rlock(data_lod0.map_lock);

		const bool all_blocks_present = block_box.all_cells_match([&data_lod0](Vector3i pos) { //
			return data_lod0.map.has_block(pos);
		});

		// 在多 LOD 环境下，假定父 LOD 遵循覆盖其全部子级的原则。
		// 换言之，假定所有父 LOD 均已加载。
		return all_blocks_present;
	}
}

void VoxelData::pre_generate_box(
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
) {
	// 这主要用于 VoxelLodTerrain，即未编辑的数据块未被缓存的情形。

	VOXEL_PROFILE_SCOPE();
	// ERR_FAIL_COND_MSG(_full_load_mode == false, nullptr, "This function can only be used in full load mode");

	struct Task {
		Vector3i block_pos;
		uint32_t lod_index;
		std::shared_ptr<VoxelBuffer> voxels;
	};

	// TODO 优化：thread_local 池化？
	StdVector<Task> todo;
	// 我们将按 LOD 打包任务，以便减少加锁操作
	FixedArray<unsigned int, constants::MAX_LOD> count_per_lod;
	fill(count_per_lod, 0u);

	// 我们本可以在整个过程中对所有 LOD 加写锁。
	// 但为了减少加锁次数和持锁时间，我们只对它们逐个加读锁
	// 以确定需要生成哪些数据块。然后，我们在不持锁的情况下分别生成体素。
	// 最后，我们再次逐个锁定 LOD 以插入新生成的数据块。
	// 一个缺点是某些数据块的状态可能在此期间发生变化。若发生，我们将跳过插入。

	// 查找空槽位
	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
		const Box3i block_box = voxel_box.downscaled(data_block_size << lod_index);

		// VOXEL_PRINT_VERBOSE(format("Preloading box {} at lod {} synchronously", block_box, lod_index));

		Lod &data_lod = lods[lod_index];
		const unsigned int prev_size = todo.size();

		{
			SpatialLock3D::Read srlock(data_lod.spatial_lock, block_box);

			RWLockRead rlock(data_lod.map_lock);

			block_box.for_each_cell([&data_lod, lod_index, &todo, streaming](Vector3i block_pos) {
				// 我们不检查"加载中的数据块"，因为此函数希望立即完成任务。
				const VoxelDataBlock *block = data_lod.map.get_block(block_pos);
				if (streaming) {
					// 不得触碰未加载的数据块，因为我们不知道其中的内容。
					// 若已加载的数据块没有体素数据，我们可以生成缓存。
					if (block != nullptr && !block->has_voxels()) {
						todo.push_back(Task{ block_pos, lod_index, nullptr });
					}
				} else {
					// 我们可以在体素数据不在内存中的任何位置生成
					if (block == nullptr || !block->has_voxels()) {
						todo.push_back(Task{ block_pos, lod_index, nullptr });
					}
				}
			});
		}

		count_per_lod[lod_index] = todo.size() - prev_size;
	}

	const Vector3i block_size = Vector3iUtil::create(data_block_size);

	// 生成
	for (unsigned int i = 0; i < todo.size(); ++i) {
		Task &task = todo[i];
		task.voxels = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
		task.voxels->create(block_size, &format);
		if (generator.is_valid()) {
			VOXEL_PROFILE_SCOPE_NAMED("Generate");
			VoxelGenerator::VoxelQueryData q{ //
											  *task.voxels,
											  task.block_pos * (data_block_size << task.lod_index),
											  task.lod_index
			};
			generator->generate_block(q);
#ifdef VOXEL_ENABLE_MODIFIERS
			modifiers.apply(q.voxel_buffer, AABB(q.origin_in_voxels, q.voxel_buffer.get_size() << q.lod));
#endif
		}
	}

	// 填充槽位
	unsigned int task_index = 0;
	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
		VOXEL_ASSERT(lod_index < count_per_lod.size());
		const unsigned int count = count_per_lod[lod_index];

		if (count > 0) {
			const unsigned int end_task_index = task_index + count;

			Lod &data_lod = lods[lod_index];

			const Box3i block_box = voxel_box.downscaled(data_block_size << lod_index);
			SpatialLock3D::Write swlock(data_lod.spatial_lock, block_box);

			RWLockWrite wlock(data_lod.map_lock);

			// 任务按 LOD 分组，这样我们可以在连续范围内获取给定 LOD 的全部任务
			for (; task_index < end_task_index; ++task_index) {
				Task &task = todo[task_index];
				VOXEL_ASSERT(task.lod_index == lod_index);
				const VoxelDataBlock *prev_block = data_lod.map.get_block(task.block_pos);
				if (prev_block != nullptr && prev_block->has_voxels()) {
					// 抱歉，该数据块在此期间已被其他线程设置。
					// 我们将假定刚生成的数据块是多余的并将其丢弃。
					continue;
				}
				data_lod.map.set_block_buffer(task.block_pos, task.voxels, true);
			}
		}
	}
}

void VoxelData::pre_generate_box(Box3i voxel_box) {
	const unsigned int data_block_size = get_block_size();
	const bool streaming = is_streaming_enabled();
	const unsigned int lod_count = get_lod_count();
	pre_generate_box(
			voxel_box,
			to_span(_lods),
			data_block_size,
			streaming,
			lod_count,
			get_generator(),
#ifdef VOXEL_ENABLE_MODIFIERS
			_modifiers,
#endif
			get_format()
	);
}

void VoxelData::clear_cached_blocks_in_voxel_area(Box3i p_voxel_box) {
	const unsigned int lod_count = get_lod_count();

	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
		Lod &lod = _lods[lod_index];

		// 对区域加写锁，因为技术上我们可能会修改数据块
		const Box3i blocks_box = p_voxel_box.downscaled(lod.map.get_block_size() << lod_index);
		SpatialLock3D::Write swlock(lod.spatial_lock, blocks_box);

		// 对地图加读锁，因为我们不会增删数据块
		RWLockRead rlock(lod.map_lock);

		blocks_box.for_each_cell_zxy([&lod](const Vector3i bpos) {
			VoxelDataBlock *block = lod.map.get_block(bpos);
			if (block == nullptr || block->is_edited() || block->is_modified()) {
				return;
			}
			block->clear_voxels();
		});
	}
}

void VoxelData::mark_area_modified(
		Box3i p_voxel_box,
		StdVector<Vector3i> *lod0_new_blocks_to_lod,
		bool require_lod_updates
) {
	// TODO 我们或许应将此与编辑合并，因为那意味着发生两次独立的加锁。其间存在一段
	// 时间，我们最终得到已修改的体素却尚未标记为已修改。

	const Box3i bbox = p_voxel_box.downscaled(get_block_size());

	Lod &data_lod0 = _lods[0];
	{
		SpatialLock3D::Write swlock(data_lod0.spatial_lock, bbox);

		// 对地图加读锁，因为我们不会增删数据块
		RWLockRead rlock(data_lod0.map_lock);

		bbox.for_each_cell([&data_lod0, lod0_new_blocks_to_lod, require_lod_updates](Vector3i block_pos_lod0) {
			VoxelDataBlock *block = data_lod0.map.get_block(block_pos_lod0);

			// TODO 找不到数据块或已分配体素可能表明其他地方出错，但值得打印吗？
			if (block == nullptr) {
				VOXEL_PRINT_VERBOSE("Modifying area without data blocks?");
				return;
			}
			if (!block->has_voxels()) {
				VOXEL_PRINT_VERBOSE("Modifying area without allocated voxels?");
				return;
			}

			// RWLockWrite wlock(block->get_voxels_shared()->get_lock());
			block->set_modified(true);
			block->set_edited(true);

			// TODO 该布尔值也会被线程化更新任务修改（总是设为 false）
			if (!block->get_needs_lodding() && require_lod_updates) {
				block->set_needs_lodding(true);

				// 这间接导致网格重新生成
				if (lod0_new_blocks_to_lod != nullptr) {
					lod0_new_blocks_to_lod->push_back(block_pos_lod0);
				}
			}
		});
	}
}

bool VoxelData::try_set_block(Vector3i block_position, const VoxelDataBlock &block) {
	bool inserted = true;
	try_set_block(block_position, block, [&inserted](VoxelDataBlock &existing, const VoxelDataBlock &incoming) {
		inserted = false;
	});
	return inserted;
}

bool VoxelData::has_block(Vector3i bpos, unsigned int lod_index) const {
	const Lod &data_lod = _lods[lod_index];
	RWLockRead rlock(data_lod.map_lock);
	return data_lod.map.has_block(bpos);
}

bool VoxelData::has_all_blocks_in_area(Box3i data_blocks_box, unsigned int lod_index) const {
	VOXEL_PROFILE_SCOPE();
	// TODO get_bounds 会锁定互斥量，所有调用方或许最好使用无边界版本并自行裁剪
	// 尤其是需要多次执行此操作时
	const Box3i bounds_in_blocks = get_bounds().downscaled(get_block_size() << lod_index);
	data_blocks_box = data_blocks_box.clipped(bounds_in_blocks);

	return has_all_blocks_in_area_unbound(data_blocks_box, lod_index);
}

bool VoxelData::has_all_blocks_in_area_unbound(Box3i data_blocks_box, unsigned int lod_index) const {
	// VOXEL_PROFILE_SCOPE();
	const Lod &data_lod = _lods[lod_index];
	RWLockRead rlock(data_lod.map_lock);

	return data_blocks_box.all_cells_match([&data_lod](Vector3i bpos) { //
		return data_lod.map.has_block(bpos);
	});
}

unsigned int VoxelData::get_block_count() const {
	unsigned int sum = 0;
	const unsigned int lod_count = get_lod_count();
	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
		const Lod &lod = _lods[lod_index];
		RWLockRead rlock(lod.map_lock);
		sum += lod.map.get_block_count();
	}
	return sum;
}

void VoxelData::update_lods(Span<const Vector3i> modified_lod0_blocks, StdVector<BlockLocation> *out_updated_blocks) {
	VOXEL_DSTACK();
	VOXEL_PROFILE_SCOPE();
	// 将迄今执行的编辑传播到其他 LOD。
	// 这些 LOD 当前必须在内存中，否则地形数据将错过这些编辑。
	// 目前这通过我们以"金字塔"方式加载数据块来保证，
	// 即若父 LOD 尚未加载，则数据块无法加载。
	// 未来我们可能实现存储编辑，以便在找不到数据块时稍后应用。

	const unsigned int data_block_size = get_block_size();
	const int data_block_size_po2 = get_block_size_po2();
	const unsigned int lod_count = get_lod_count();
	const bool streaming_enabled = is_streaming_enabled();
	Ref<VoxelGenerator> generator = get_generator();

	static thread_local FixedArray<StdVector<Vector3i>, constants::MAX_LOD> tls_blocks_to_process_per_lod;

	// 确保即使 _lod_count 为 1，LOD0 也能获得更新
	{
		StdVector<Vector3i> &dst_lod0 = tls_blocks_to_process_per_lod[0];
		dst_lod0.resize(modified_lod0_blocks.size());
		// TODO 可以使用 std::copy，但我不确定 Vector3i 是否会被认为"平凡"到足以让复制
		// 被优化为 memcpy/memmove。需要验证，如果可能应为其编写测试。
		memcpy(dst_lod0.data(), modified_lod0_blocks.data(), dst_lod0.size() * sizeof(Vector3i));
	}
	{
		Lod &data_lod0 = _lods[0];
		RWLockRead rlock(data_lod0.map_lock);

		StdVector<Vector3i> &blocks_pending_lodding_lod0 = tls_blocks_to_process_per_lod[0];

		for (const Vector3i data_block_pos : blocks_pending_lodding_lod0) {
			VoxelDataBlock *data_block = data_lod0.map.get_block(data_block_pos);
			ERR_CONTINUE(data_block == nullptr);
			// TODO 线程：这是在未加空间锁的情况下设置的，因此理论上其他线程也可能更改它！
			data_block->set_needs_lodding(false);

			if (out_updated_blocks != nullptr) {
				out_updated_blocks->push_back(BlockLocation{ data_block_pos, 0 });
			}
		}
	}

	const int half_bs = data_block_size >> 1;

	// 按连续 LOD 对向上处理降采样。
	// 这确保我们不会多次处理相同的数据块。
	// 目前只有 LOD0 可编辑，因此我们将从那里开始降采样
	for (uint8_t dst_lod_index = 1; dst_lod_index < lod_count; ++dst_lod_index) {
		const uint8_t src_lod_index = dst_lod_index - 1;
		StdVector<Vector3i> &src_lod_blocks_to_process = tls_blocks_to_process_per_lod[src_lod_index];
		StdVector<Vector3i> &dst_lod_blocks_to_process = tls_blocks_to_process_per_lod[dst_lod_index];

		// VoxelLodTerrainUpdateData::Lod &dst_lod = state.lods[dst_lod_index];

		for (unsigned int i = 0; i < src_lod_blocks_to_process.size(); ++i) {
			Lod &src_data_lod = _lods[src_lod_index];
			Lod &dst_data_lod = _lods[dst_lod_index];

			const Vector3i src_bpos = src_lod_blocks_to_process[i];
			const Vector3i dst_bpos = src_bpos >> 1;

			// TODO 研究更好的锁定策略。
			// 地图必须在空间锁之后锁定以防止死锁。它们必须保持锁定，因为
			// 数据块不是共享指针。若能在可能的生成之后再获取空间锁会更好
			// ...或许数据块需要被共享，而不是体素缓冲区
			SpatialLock3D::Read srlock(src_data_lod.spatial_lock, BoxBounds3i::from_position(src_bpos));

			// TODO 锁定此处可能耗时较长，我们可以先生成内容，最后再赋值给地图。
			// 此外，在按数据块流式加载模式下，这没有必要，因为数据块应当已存在
			SpatialLock3D::Write swlock(dst_data_lod.spatial_lock, BoxBounds3i::from_position(dst_bpos));

			VoxelDataBlock *src_block;
			VoxelDataBlock *dst_block;
			{
				RWLockRead rlock(src_data_lod.map_lock);
				src_block = src_data_lod.map.get_block(src_bpos);
			}
			{
				RWLockRead rlock(dst_data_lod.map_lock);
				dst_block = dst_data_lod.map.get_block(dst_bpos);
			}

			VOXEL_ASSERT(src_block != nullptr);
			src_block->set_needs_lodding(false);

			struct L {
				static std::shared_ptr<VoxelBuffer> generate_voxels(
						Vector3i dst_bpos,
						uint8_t dst_lod_index,
						int data_block_size,
						int data_block_size_po2,
						Ref<VoxelGenerator> generator,
#ifdef VOXEL_ENABLE_MODIFIERS
						const VoxelModifierStack &modifiers,
#endif
						const VoxelFormat &format
				) {
					//
					std::shared_ptr<VoxelBuffer> voxels =
							make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
					voxels->create(Vector3iUtil::create(data_block_size), &format);
					VoxelGenerator::VoxelQueryData q{ //
													  *voxels, //
													  dst_bpos << (dst_lod_index + data_block_size_po2), //
													  dst_lod_index
					};
					if (generator.is_valid()) {
						VOXEL_PROFILE_SCOPE_NAMED("Generate");
						generator->generate_block(q);
					}
#ifdef VOXEL_ENABLE_MODIFIERS
					modifiers.apply(
							q.voxel_buffer, AABB(q.origin_in_voxels, q.voxel_buffer.get_size() << dst_lod_index)
					);
#endif

					return voxels;
				}
			};

			if (dst_block == nullptr) {
				if (!streaming_enabled) {
					// TODO 在主线程上执行此操作可能开销很大并导致停顿。
					// 我们应该想办法使其异步、不需要 mip，或不编辑查看器区域之外。
					std::shared_ptr<VoxelBuffer> voxels = L::generate_voxels(
							dst_bpos,
							dst_lod_index,
							data_block_size,
							data_block_size_po2,
							generator,
#ifdef VOXEL_ENABLE_MODIFIERS
							_modifiers,
#endif
							_format
					);

					{
						RWLockWrite wlock(dst_data_lod.map_lock);
						dst_block = dst_data_lod.map.set_block_buffer(dst_bpos, voxels, true);
					}

				} else {
					VOXEL_PRINT_ERROR(
							format("Destination block {} not found when cascading edits on LOD {}",
								   dst_bpos,
								   static_cast<int>(dst_lod_index))
					);
					continue;
				}
			}

			// 该数据块及其更低 LOD 索引预期是可用的。
			// 否则意味着该函数被调用得太晚了？
			VOXEL_ASSERT(dst_block != nullptr);
			// VOXEL_ASSERT(dst_block != nullptr);
			// 若该数据块已被编辑或生成过 mip，则应当拥有体素。
			VOXEL_ASSERT(src_block->has_voxels());

			if (out_updated_blocks != nullptr) {
				out_updated_blocks->push_back(BlockLocation{ dst_bpos, dst_lod_index });
			}

			if (!dst_block->has_voxels()) {
				// 目标数据块已加载但未缓存体素。我们需要生成体素以
				// 更新它。
				std::shared_ptr<VoxelBuffer> voxels = L::generate_voxels(
						dst_bpos,
						dst_lod_index,
						data_block_size,
						data_block_size_po2,
						generator,
#ifdef VOXEL_ENABLE_MODIFIERS
						_modifiers,
#endif
						_format
				);
				dst_block->set_voxels(voxels);
			}

			dst_block->set_modified(true);

			if (dst_lod_index != lod_count - 1 && !dst_block->get_needs_lodding()) {
				dst_block->set_needs_lodding(true);
				dst_lod_blocks_to_process.push_back(dst_bpos);
			}

			const Vector3i rel = src_bpos - (dst_bpos << 1);

			// 更新更低 LOD
			// 这必须在编辑后、保存前始终执行，否则 LOD 将不匹配，看起来会
			// 很糟糕。
			// TODO 优化：尝试缩小到已编辑区域，而不是取整个数据块
			{
				VOXEL_PROFILE_SCOPE_NAMED("Downscale");
				// TODO 目标数据块应当被锁定！
				// 或许至今未做是因为还没有其他内容访问更高 LOD 索引，或是因为我们
				// 正持有包含它的地图上的锁
				src_block->get_voxels().downscale_to(
						dst_block->get_voxels(), Vector3i(), src_block->get_voxels_const().get_size(), rel * half_bs
				);
			}
		}

		src_lod_blocks_to_process.clear();
		// 无需清空最后一个列表，因为我们从不向其中添加数据块
	}

	//	uint64_t time_spent = profiling_clock.restart();
	//	if (time_spent > 10) {
	//		print_line(String("Took {0} us to update lods").format(varray(time_spent)));
	//	}
}

void VoxelData::unload_blocks(Box3i bbox, unsigned int lod_index, StdVector<BlockToSave> *to_save) {
	Lod &lod = _lods[lod_index];
	SpatialLock3D::Write swlock(lod.spatial_lock, bbox);
	RWLockWrite wlock(lod.map_lock);
	if (to_save == nullptr) {
		bbox.for_each_cell_zxy([&lod](Vector3i bpos) { //
			lod.map.remove_block(bpos, VoxelDataMap::NoAction());
		});
	} else {
		bbox.for_each_cell_zxy([&lod, lod_index, to_save](Vector3i bpos) {
			lod.map.remove_block(bpos, BeforeUnloadSaveAction{ to_save, bpos, lod_index });
		});
	}
}

// void VoxelData::unload_blocks(Span<const Vector3i> positions, StdVector<BlockToSave> *to_save) {
// 	// 效率不高！我们需要在每个位置也加空间锁才能卸载...
// 	Lod &lod = _lods[0];
// 	RWLockWrite wlock(lod.map_lock);
// 	if (to_save == nullptr) {
// 		for (Vector3i bpos : positions) {
// 			lod.map.remove_block(bpos, VoxelDataMap::NoAction());
// 		}
// 	} else {
// 		for (Vector3i bpos : positions) {
// 			lod.map.remove_block(bpos, BeforeUnloadSaveAction{ to_save, bpos, 0 });
// 		}
// 	}
// }

bool VoxelData::consume_block_modifications(Vector3i bpos, VoxelData::BlockToSave &out_to_save) {
	Lod &lod = _lods[0];

	// 对数据块加写锁，因为我们将更改其状态。
	// TODO 若争用过大，这种情况或许可以使用原子操作？
	SpatialLock3D::Write swlock(lod.spatial_lock, BoxBounds3i::from_position(bpos));

	// 对地图加读锁，因为我们不会增删数据块
	RWLockRead rlock(lod.map_lock);

	VoxelDataBlock *block = lod.map.get_block(bpos);
	if (block == nullptr) {
		return false;
	}
	if (block->is_modified()) {
		if (block->has_voxels()) {
			out_to_save.voxels = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
			block->get_voxels_const().copy_to(*out_to_save.voxels, true);
		}
		out_to_save.position = bpos;
		out_to_save.lod_index = 0;
		block->set_modified(false);
		return true;
	}
	return false;
}

void VoxelData::consume_all_modifications(StdVector<BlockToSave> &to_save, bool with_copy) {
	const unsigned int lod_count = get_lod_count();
	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
		Lod &lod = _lods[lod_index];

		// 对数据块加写锁，因为我们将更改其状态。
		// TODO 若争用过大，这种情况或许可以使用原子操作？
		SpatialLock3D::Write srlock(lod.spatial_lock, BoxBounds3i::from_everywhere());

		// 对地图加读锁，因为我们不会增删数据块
		RWLockRead rlock(lod.map_lock);

		lod.map.for_each_block(ScheduleSaveAction{ to_save, uint8_t(lod_index), with_copy });
	}
}

void VoxelData::get_missing_blocks(
		Span<const Vector3i> block_positions,
		unsigned int lod_index,
		StdVector<Vector3i> &out_missing
) const {
	const Lod &lod = _lods[lod_index];
	RWLockRead rlock(lod.map_lock);
	for (const Vector3i &pos : block_positions) {
		if (!lod.map.has_block(pos)) {
			out_missing.push_back(pos);
		}
	}
}

void VoxelData::get_missing_blocks(Box3i p_blocks_box, unsigned int lod_index, StdVector<Vector3i> &out_missing) const {
	const Lod &data_lod = _lods[lod_index];

	const Box3i bounds_in_blocks = get_bounds().downscaled(get_block_size());
	const Box3i blocks_box = p_blocks_box.clipped(bounds_in_blocks);

	RWLockRead rlock(data_lod.map_lock);

	blocks_box.for_each_cell_zxy([&data_lod, &out_missing](Vector3i bpos) {
		if (!data_lod.map.has_block(bpos)) {
			out_missing.push_back(bpos);
		}
	});
}

void VoxelData::get_blocks_with_voxel_data(
		Box3i p_blocks_box,
		unsigned int lod_index,
		Span<std::shared_ptr<VoxelBuffer>> out_blocks
) const {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(out_blocks.size() >= Vector3iUtil::get_volume_u64(p_blocks_box.size));

	const Lod &data_lod = _lods[lod_index];

	// 同时使用空间锁，因为我们需要检查数据块是否有体素，这是一种可能被其他线程
	// （理论上）更改的状态
	SpatialLock3D::Read srlock(data_lod.spatial_lock, p_blocks_box);

	RWLockRead rlock(data_lod.map_lock);

	unsigned int index = 0;

	p_blocks_box.for_each_cell_zxy([&index, &data_lod, &out_blocks](Vector3i data_block_pos) {
		const VoxelDataBlock *nblock = data_lod.map.get_block(data_block_pos);
		// 该数据块在某些情况下实际上可能为 null。尚不确定这是否很糟
		// CRASH_COND(nblock == nullptr);
		if (nblock != nullptr && nblock->has_voxels()) {
			out_blocks[index] = nblock->get_voxels_shared();
		}
		++index;
	});
}

void VoxelData::get_blocks_grid(VoxelDataGrid &grid, Box3i box_in_voxels, unsigned int lod_index) const {
	VOXEL_PROFILE_SCOPE();
	const Lod &data_lod = _lods[lod_index];
	const int bs = data_lod.map.get_block_size() << lod_index;
	const Box3i box_in_blocks = box_in_voxels.downscaled(bs);
	grid.reference_area_block_coords(data_lod.map, data_lod.map_lock, box_in_blocks, data_lod.spatial_lock);
}

SpatialLock3D &VoxelData::get_spatial_lock(unsigned int lod_index) const {
	const Lod &data_lod = _lods[lod_index];
	return data_lod.spatial_lock;
}

bool VoxelData::has_blocks_with_voxels_in_area_broad_mip_test(Box3i box_in_voxels) const {
	VOXEL_PROFILE_SCOPE();

	// 先找到要查询的最高 LOD 层级
	const Vector3i box_size_in_blocks = box_in_voxels.size >> get_block_size_po2();
	const int box_size_in_blocks_longest_axis =
			math::max(box_size_in_blocks.x, math::max(box_size_in_blocks.y, box_size_in_blocks.z));
	const int top_lod_index =
			math::min(math::get_next_power_of_two_32_shift(box_size_in_blocks_longest_axis), get_lod_count());

	// 检查是否存在已编辑的 mip
	const Lod &mip_data_lod = _lods[top_lod_index];
	{
		// 若该 box 是立方体，理想情况下它不应与超过 8 个数据块相交。
		const Box3i mip_blocks_box = box_in_voxels.downscaled(mip_data_lod.map.get_block_size() << top_lod_index);

		SpatialLock3D::Read srlock(mip_data_lod.spatial_lock, mip_blocks_box);

		RWLockRead rlock(mip_data_lod.map_lock);

		const VoxelDataMap &map = mip_data_lod.map;
		const bool no_blocks_found = mip_blocks_box.all_cells_match([&map](const Vector3i pos) {
			const VoxelDataBlock *block = map.get_block(pos);
			return block == nullptr || block->has_voxels() == false;
		});

		if (no_blocks_found) {
			// 在此 mip 未找到编辑，我们可以假定更低 LOD 中也没有编辑。
			return false;
		}
	}

	// 假定可能存在编辑
	return true;
}

void VoxelData::view_area(
		Box3i blocks_box,
		unsigned int lod_index,
		StdVector<Vector3i> *missing_blocks,
		StdVector<Vector3i> *found_blocks_positions,
		StdVector<VoxelDataBlock> *found_blocks
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(lod_index < _lods.size());

	const Box3i bounds_in_blocks = get_bounds().downscaled(get_block_size());
	blocks_box = blocks_box.clipped(bounds_in_blocks);

	Lod &lod = _lods[lod_index];

	// 对数据块加写锁，因为我们将修改其状态。
	// TODO 若争用过大，是否可以使用原子操作？
	SpatialLock3D::Write swlock(lod.spatial_lock, blocks_box);

	// 对地图加读锁，因为我们不会增删数据块。
	RWLockRead rlock(lod.map_lock);

	blocks_box.for_each_cell_zxy([&lod, found_blocks_positions, found_blocks, &missing_blocks](Vector3i bpos) {
		VoxelDataBlock *block = lod.map.get_block(bpos);
		if (block != nullptr) {
			block->viewers.add();
			if (found_blocks != nullptr) {
				found_blocks->push_back(*block);
			}
			if (found_blocks_positions != nullptr) {
				found_blocks_positions->push_back(bpos);
			}
		} else if (missing_blocks != nullptr) {
			missing_blocks->push_back(bpos);
		}
	});
}

void VoxelData::unview_area(
		Box3i blocks_box,
		unsigned int lod_index,
		StdVector<Vector3i> *removed_blocks,
		StdVector<Vector3i> *missing_blocks,
		StdVector<BlockToSave> *to_save
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(lod_index < _lods.size());

	const Box3i bounds_in_blocks = get_bounds().downscaled(get_block_size());
	blocks_box = blocks_box.clipped(bounds_in_blocks);

	Lod &lod = _lods[lod_index];

	// 对数据块加写锁，因为我们将修改其状态。
	// TODO 若争用过大，是否可以使用原子操作？不过若使用，我们需要确保没有其他线程持有
	// 指向我们可能移除的任一数据块的指针。
	SpatialLock3D::Write swlock(lod.spatial_lock, blocks_box);

	// 对地图加写锁，因为我们可能会从地图中移除数据块。
	RWLockWrite wlock(lod.map_lock);

	blocks_box.for_each_cell_zxy([&lod, missing_blocks, removed_blocks, to_save, lod_index](Vector3i bpos) {
		VoxelDataBlock *block = lod.map.get_block(bpos);
		if (block != nullptr) {
			block->viewers.remove();
			if (block->viewers.get() == 0) {
				if (to_save == nullptr) {
					lod.map.remove_block(bpos, VoxelDataMap::NoAction());
				} else {
					lod.map.remove_block(bpos, BeforeUnloadSaveAction{ to_save, bpos, lod_index });
				}
				if (removed_blocks != nullptr) {
					removed_blocks->push_back(bpos);
				}
			}
		} else if (missing_blocks != nullptr) {
			missing_blocks->push_back(bpos);
		}
	});
}

std::shared_ptr<VoxelBuffer> VoxelData::try_get_block_voxels(Vector3i bpos) {
	Lod &lod = _lods[0];

	// 调用方必须锁定空间锁并保持锁定，直到完成对数据块的访问
	// SpatialLock3D::Read srlock(lod.spatial_lock, BoxBounds3i::from_position(bpos));

	RWLockRead rlock(lod.map_lock);

	VoxelDataBlock *block = lod.map.get_block(bpos);
	if (block == nullptr) {
		return nullptr;
	}
	if (block->has_voxels()) {
		return block->get_voxels_shared();
	}
	return nullptr;
}

void VoxelData::set_voxel_metadata(const Vector3i pos, const Variant &meta) {
	Lod &lod = _lods[0];

	const Vector3i bpos = lod.map.voxel_to_block(pos);

	SpatialLock3D::Write swlock(lod.spatial_lock, BoxBounds3i::from_position(bpos));
	std::shared_ptr<VoxelBuffer> vb = try_get_writable_voxel_buffer_assuming_spatial_lock(lod, bpos);
	VOXEL_ASSERT_RETURN_MSG(vb != nullptr, "Area not editable");

	const Vector3i rpos = lod.map.to_local(pos);
	voxel::godot::set_voxel_metadata(*vb, rpos, meta);
}

Variant VoxelData::get_voxel_metadata(const Vector3i pos) {
	if (!_bounds_in_voxels.contains(pos)) {
		return Variant();
	}

	const unsigned int lod_index = 0;
	Lod &lod = _lods[lod_index];
	const Vector3i bpos = lod.map.voxel_to_block(pos);
	const Vector3i rpos = lod.map.to_local(pos);

	bool generate = false;
	{
		SpatialLock3D::Read srlock(lod.spatial_lock, BoxBounds3i::from_position(bpos));
		std::shared_ptr<VoxelBuffer> voxels = try_get_voxel_buffer_with_lock(lod, bpos, generate);

		if (voxels != nullptr) {
			return voxel::godot::get_voxel_metadata(*voxels, rpos);
		}
	}
	if (generate || (_streaming_enabled == false && _full_load_completed)) {
		Ref<VoxelGenerator> generator = get_generator();
		if (generator.is_valid()) {
			// TODO 这感觉不太好。导致走到这里的设置组合本不该出现。
			VoxelBuffer temp(VoxelBuffer::ALLOCATOR_POOL);
			temp.create(Vector3i(1, 1, 1));
			VoxelGenerator::VoxelQueryData q{ temp, pos, lod_index };
			generator->generate_block(q);
			return voxel::godot::get_voxel_metadata(temp, Vector3i(0, 0, 0));
		}
		return Variant();
	}
	return Variant();
}

} // namespace voxel
