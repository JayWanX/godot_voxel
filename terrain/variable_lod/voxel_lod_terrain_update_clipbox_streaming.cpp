#include "voxel_lod_terrain_update_clipbox_streaming.h"
#include "../../util/containers/std_unordered_set.h"
#include "../../util/math/conv.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"
#include "voxel_lod_terrain_update_task.h"

// #include <fstream>

namespace voxel {

// 注意：
// 这种流式加载方法允许每个 LOD 并行加载，甚至在网格就绪之前。这意味着如果某个数据块
// 被加载并随后被编辑，无法保证其父级 LOD 已加载！虽然可以提高这种可能性，但无法保证。
// 不过，如果 LOD 只是 LOD0 的视觉化表示，这通常不会成为问题。若在 LOD0 上发生编辑
// 且兄弟数据块存在，则可用它们来生成父级 LOD。
// 另一种方案是让 LOD 更新等待。最坏情况下，父级 LOD 将不会更新。

// TODO 八叉树流式加载会持续轮询，但裁剪盒不会。因此若某个任务因距离过远而被丢弃，
// 它可能会造成数据块空洞或 LOD 阻塞，因为不会再次请求它...
// 我们要么处理“已丢弃”的响应并在仍需要时重新触发（就像之前那样），要么为每个加载任务
// 使用一个由任务和请求者共同持有的共享布尔标志，请求者在不再需要时将其置为 false，
// 否则任务不会被取消。

namespace {

bool find_index(Span<const std::pair<ViewerID, VoxelEngine::Viewer>> viewers, ViewerID id, unsigned int &out_index) {
	for (unsigned int i = 0; i < viewers.size(); ++i) {
		if (viewers[i].first == id) {
			out_index = i;
			return true;
		}
	}
	return false;
}

bool contains(Span<const std::pair<ViewerID, VoxelEngine::Viewer>> viewers, ViewerID id) {
	unsigned int _unused;
	return find_index(viewers, id, _unused);
}

bool find_index(Span<const VoxelLodTerrainUpdateData::PairedViewer> viewers, ViewerID id, unsigned int &out_index) {
	for (unsigned int i = 0; i < viewers.size(); ++i) {
		if (viewers[i].id == id) {
			out_index = i;
			return true;
		}
	}
	return false;
}

Box3i get_base_box_in_chunks(
		Vector3i viewer_position_voxels,
		Vector3i distance_voxels,
		int chunk_size,
		bool make_even
) {
	// 获取最小和最大位置
	Vector3i minp = viewer_position_voxels - distance_voxels;
	Vector3i maxp = viewer_position_voxels +
			distance_voxels
			// 当距离是数据块大小的整数倍时，本应能得到一致的盒子大小，
			// 但如果没有这个 +1，仍会有一些特定坐标使盒子因舍入而缩小
			+ Vector3iUtil::create(1);

	// 转换为数据块坐标
	minp = math::floordiv(minp, chunk_size);
	maxp = math::ceildiv(maxp, chunk_size);

	if (make_even) {
		// 向外取整为偶数（细分规则的部分要求）
		// TODO 也许有更巧妙的方法来实现
		minp = math::floordiv(minp, 2) * 2;
		maxp = math::ceildiv(maxp, 2) * 2;
	}

	return Box3i::from_min_max(minp, maxp);
}

// 获取父级 LOD 必须拥有的最小盒子，以便继续遵守相邻规则
Box3i get_minimal_box_for_parent_lod(Box3i child_lod_box, bool make_even) {
	const int min_pad = 1;
	// 注意，细分规则强制子盒子的位置和大小为偶数，因此在转换为父级 LOD 的坐标系时
	// 不会舍入为零。
	Box3i min_box = Box3i(child_lod_box.position >> 1, child_lod_box.size >> 1)
							// 通过向外填充盒子最小量来强制相邻规则，
							// 以便在当前 LOD 中 LOD+1 和 LOD-1 之间至少有 N 个数据块
							.padded(min_pad);

	if (make_even) {
		// 确保保持偶数以遵守细分规则，向外取整
		min_box = min_box.downscaled(2).scaled(2);
	}

	return min_box;
}

Box3i enforce_neighboring_rule(Box3i box, const Box3i &child_lod_box, bool make_even) {
	const Box3i min_box = get_minimal_box_for_parent_lod(child_lod_box, make_even);
	box.merge_with(min_box);
	return box;
}

inline int get_lod_distance_in_mesh_chunks(float lod_distance_in_voxels, int mesh_block_size) {
	return math::max(static_cast<int>(Math::ceil(lod_distance_in_voxels)) / mesh_block_size, 1);
}

// 计算观察者与该 LOD 末端之间、相对于当前 LOD 的以数据块为单位的距离
Vector3i get_relative_lod_distance_in_chunks(
		int lod_index,
		int lod_count,
		int lod0_distance_in_chunks,
		int lodn_distance_in_chunks,
		int lod_chunk_size,
		Vector3i max_view_distance_voxels
) {
	int ld;
	if (lod_index == 0) {
		// 第一个 LOD 使用专用距离
		ld = lod0_distance_in_chunks;
	} else {
		// 后续 LOD 使用另一个距离。
		// 返回的距离相对于当前 LOD 的数据块，因此我们除以 LOD0 距离而不是
		// 乘以 LODN 距离
		ld = (lod0_distance_in_chunks >> lod_index) + lodn_distance_in_chunks;
	}
	Vector3i ld3(ld, ld, ld);
	if (lod_index == lod_count - 1) {
		// 如果可能，最后一个 LOD 可以一直延伸到最大视距
		ld3 = math::max(ld3, math::ceildiv(max_view_distance_voxels, Vector3iUtil::create(lod_chunk_size)));
	}
	return ld3;
}

void process_viewers(
		VoxelLodTerrainUpdateData::ClipboxStreamingState &cs,
		const VoxelLodTerrainUpdateData::Settings &volume_settings,
		unsigned int lod_count,
		Span<const std::pair<ViewerID, VoxelEngine::Viewer>> viewers,
		const Transform3D &volume_transform,
		Box3i volume_bounds_in_voxels,
		int data_block_size_po2,
		bool can_mesh,
		// 按配对观察者列表中的升序索引排列
		StdVector<unsigned int> &unpaired_viewers_to_remove
) {
	VOXEL_PROFILE_SCOPE();

	// 已销毁的观察者
	for (size_t paired_viewer_index = 0; paired_viewer_index < cs.paired_viewers.size(); ++paired_viewer_index) {
		VoxelLodTerrainUpdateData::PairedViewer &pv = cs.paired_viewers[paired_viewer_index];

		if (!contains(viewers, pv.id)) {
			VOXEL_PRINT_VERBOSE(format("Detected destroyed viewer {} in VoxelLodTerrain", pv.id));

			// 将移除解释为视距归零，这样处理数据块加载的同一套代码
			// 也将用于卸载该观察者看到的数据块。
			// 我们实际上会在第二遍中移除未配对的观察者。
			pv.state.view_distance_voxels = VoxelLodTerrainUpdateData::PairedViewer::Distances();

			// 同时更新盒子，因为观察者已被移除，它们不会再被更新。
			// 赋值上一个状态，否则在某些情况下重置盒子会使它们与上一个状态相同，
			// 从而不会触发卸载
			pv.prev_state = pv.state;

			for (unsigned int lod_index = 0; lod_index < pv.state.data_box_per_lod.size(); ++lod_index) {
				pv.state.data_box_per_lod[lod_index] = Box3i();
			}
			for (unsigned int lod_index = 0; lod_index < pv.state.mesh_box_per_lod.size(); ++lod_index) {
				pv.state.mesh_box_per_lod[lod_index] = Box3i();
			}

			unpaired_viewers_to_remove.push_back(paired_viewer_index);
		}
	}

	// TODO 当观察者与体积边界相交时进行配对/取消配对

	const Transform3D world_to_local_transform = volume_transform.affine_inverse();

	// 注意，这不支持非均匀缩放
	// TODO 可能还有更好的方法
	const float view_distance_scale = world_to_local_transform.basis.xform(Vector3(1, 0, 0)).length();

	const int data_block_size = 1 << data_block_size_po2;

	const int mesh_block_size = 1 << volume_settings.mesh_block_size_po2;
	const int mesh_to_data_factor = mesh_block_size / data_block_size;

	const int lod0_distance_in_mesh_chunks =
			get_lod_distance_in_mesh_chunks(volume_settings.lod_distance, mesh_block_size);
	const int lodn_distance_in_mesh_chunks =
			get_lod_distance_in_mesh_chunks(volume_settings.secondary_lod_distance, mesh_block_size);

	// 数据块由网格数据块驱动，因为网格需要数据
	const int lod0_distance_in_data_chunks = lod0_distance_in_mesh_chunks * mesh_to_data_factor;
	const int lodn_distance_in_data_chunks = lodn_distance_in_mesh_chunks * mesh_to_data_factor;

	// const Box3i volume_bounds_in_data_blocks = volume_bounds_in_voxels.downscaled(1 << data_block_size_po2);
	// const Box3i volume_bounds_in_mesh_blocks = volume_bounds_in_voxels.downscaled(1 << mesh_block_size_po2);

	// 新观察者和现有观察者。
	// 已移除的观察者不会被迭代，但仍保持配对，直到稍后处理。
	for (const std::pair<ViewerID, VoxelEngine::Viewer> &viewer_and_id : viewers) {
		const ViewerID viewer_id = viewer_and_id.first;
		const VoxelEngine::Viewer &viewer = viewer_and_id.second;

		unsigned int paired_viewer_index;
		if (!find_index(to_span_const(cs.paired_viewers), viewer_id, paired_viewer_index)) {
			// 新观察者
			VoxelLodTerrainUpdateData::PairedViewer pv;
			pv.id = viewer_id;
			paired_viewer_index = cs.paired_viewers.size();
			cs.paired_viewers.push_back(pv);
			VOXEL_PRINT_VERBOSE(format("Pairing viewer {} to VoxelLodTerrain", viewer_id));
		}

		VoxelLodTerrainUpdateData::PairedViewer &paired_viewer = cs.paired_viewers[paired_viewer_index];

		// 将当前状态保存为上一个状态
		paired_viewer.prev_state = paired_viewer.state;

		{
			const int view_distance_voxels_h =
					static_cast<int>(static_cast<float>(viewer.view_distances.horizontal) * view_distance_scale);
			const int view_distance_voxels_v =
					static_cast<int>(static_cast<float>(viewer.view_distances.vertical) * view_distance_scale);

			paired_viewer.state.view_distance_voxels.horizontal =
					math::min(view_distance_voxels_h, static_cast<int>(volume_settings.view_distance_voxels));
			paired_viewer.state.view_distance_voxels.vertical =
					math::min(view_distance_voxels_v, static_cast<int>(volume_settings.view_distance_voxels));
		}

		// 最后一个 LOD 应至少延伸到视距。它还必须至少是
		// “LOD 距离”指定的距离
		// const int last_lod_mesh_block_size = mesh_block_size << (lod_count - 1);
		// const int last_lod_distance_in_mesh_chunks =
		// 		math::max(math::ceildiv(paired_viewer.state.view_distance_voxels, last_lod_mesh_block_size),
		// 				lod_distance_in_mesh_chunks);
		// const int last_lod_distance_in_data_chunks = last_lod_mesh_block_size * mesh_to_data_factor;

		const Vector3 local_position = world_to_local_transform.xform(viewer.world_position);

		paired_viewer.state.local_position_voxels = math::floor_to_int(local_position);
		paired_viewer.state.requires_collisions = viewer.require_collisions && can_mesh;
		paired_viewer.state.requires_visuals = viewer.require_visuals && can_mesh;

		// 观察者可以请求任意盒子，但必须遵循以下规则：
		// - 父级 LOD 的盒子必须包含子盒子（转换为世界坐标时）
		// - 有父级 LOD 的网格盒子必须具有偶数的大小和位置，以支持细分
		// - 网格盒子必须包含在数据盒子内，以保证网格能访问一致的
		//   体素数据块及其相邻数据块

		// TODO 根 LOD 不应需要偶数大小。
		// 但如果我们这样做，一个边界情况是当编辑器中 LOD 数量发生变化时，可能会引发错误，
		// 因为在处理细分时假定每个 LOD 都具有偶数大小

		// 更新数据和网格盒子
		if (paired_viewer.state.requires_collisions || paired_viewer.state.requires_visuals) {
			// 需要网格

			for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
				const int lod_mesh_block_size_po2 = volume_settings.mesh_block_size_po2 + lod_index;
				const int lod_mesh_block_size = 1 << lod_mesh_block_size_po2;

				const Vector3i ld = get_relative_lod_distance_in_chunks(
						lod_index,
						lod_count,
						lod0_distance_in_mesh_chunks,
						lodn_distance_in_mesh_chunks,
						lod_mesh_block_size,
						Vector3i(
								paired_viewer.state.view_distance_voxels.horizontal,
								paired_viewer.state.view_distance_voxels.vertical,
								paired_viewer.state.view_distance_voxels.horizontal
						)
				);

				// Box3i new_mesh_box = get_lod_box_in_chunks(
				// 		paired_viewer.state.local_position_voxels, ld, volume_settings.mesh_block_size_po2, lod_index);

				// 使子 LOD 中的最小和最大坐标为偶数，以遵守细分规则。
				// 根 LOD 不需要遵守该规则。
				const bool even_coordinates_required = (lod_index != lod_count - 1);

				Box3i new_mesh_box = get_base_box_in_chunks(
						paired_viewer.state.local_position_voxels,
						// 确保距离是数据块大小的整数倍，以获得一致的盒子大小
						ld * lod_mesh_block_size,
						lod_mesh_block_size,
						even_coordinates_required
				);

				if (lod_index > 0) {
					const Box3i &child_box = paired_viewer.state.mesh_box_per_lod[lod_index - 1];
					new_mesh_box = enforce_neighboring_rule(new_mesh_box, child_box, even_coordinates_required);
				}

				paired_viewer.state.mesh_box_per_lod[lod_index] = new_mesh_box;
			}

			// 在第二遍中裁剪所有网格盒子，因为 `enforce_neighboring_rule` 依赖于子 LOD 盒子
			for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
				const int lod_mesh_block_size_po2 = volume_settings.mesh_block_size_po2 + lod_index;
				const int lod_mesh_block_size = 1 << lod_mesh_block_size_po2;
				const Box3i volume_bounds_in_mesh_blocks = volume_bounds_in_voxels.downscaled(lod_mesh_block_size);

				Box3i &box = paired_viewer.state.mesh_box_per_lod[lod_index];
				box.clip(volume_bounds_in_mesh_blocks);
			}

			// TODO 我们应该在服务器端提供一个标志，强制数据盒子基于网格盒子，即使
			// 服务器实际上可能不需要网格。这将帮助服务器向需要数据块用于视觉网格的
			// 客户端提供数据块

			// 数据盒子必须基于网格盒子，以便加载正确的数据块来生成相应的
			// 网格（也包括我们为强制相邻规则而对网格盒子所做的调整）
			for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
				const unsigned int lod_data_block_size_po2 = data_block_size_po2 + lod_index;

				// 只要边界大小是最大 LOD 数据块的整数倍，这应该是正确的
				const Box3i volume_bounds_in_data_blocks =
						Box3i(volume_bounds_in_voxels.position >> lod_data_block_size_po2,
							  volume_bounds_in_voxels.size >> lod_data_block_size_po2);

				// const int ld =
				// 		(lod_index == (lod_count - 1) ? lod_distance_in_data_chunks : last_lod_distance_in_data_chunks);

				// const Box3i new_data_box =
				// 		get_lod_box_in_chunks(paired_viewer.state.local_position_voxels, lod_distance_in_data_chunks,
				// 				data_block_size_po2, lod_index)
				// 				// 考虑到网格需要相邻的（数据）区块。
				// 				// 这从技术上说破坏了细分规则（即每个父区块总是有 8 个子区块），但
				// 				// 应该只会在网格确实需要生成的地方产生影响
				// 				.padded(1)
				// 				.clipped(volume_bounds_in_data_blocks);

				const Box3i &mesh_box = paired_viewer.state.mesh_box_per_lod[lod_index];

				const Box3i data_box =
						Box3i(mesh_box.position * mesh_to_data_factor, mesh_box.size * mesh_to_data_factor)
								// 用于应对网格需要相邻数据块的情况。
								// 这从技术上破坏了细分规则（每个父数据块始终有 8 个子数据块），
								// 但它只在实际需要生成网格的区域才重要
								.padded(1)
								.clipped(volume_bounds_in_data_blocks);

				paired_viewer.state.data_box_per_lod[lod_index] = data_box;
			}

		} else {
			for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
				paired_viewer.state.mesh_box_per_lod[lod_index] = Box3i();
			}

			for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
				const int lod_data_block_size_po2 = data_block_size_po2 + lod_index;
				const int lod_data_block_size = 1 << lod_data_block_size_po2;

				// 只要边界大小是最大 LOD 数据块的整数倍，这应该是正确的
				const Box3i volume_bounds_in_data_blocks =
						Box3i(volume_bounds_in_voxels.position >> lod_data_block_size_po2,
							  volume_bounds_in_voxels.size >> lod_data_block_size_po2);

				const Vector3i ld = get_relative_lod_distance_in_chunks(
						lod_index,
						lod_count,
						lod0_distance_in_data_chunks,
						lodn_distance_in_data_chunks,
						lod_data_block_size,
						Vector3i(
								paired_viewer.state.view_distance_voxels.horizontal,
								paired_viewer.state.view_distance_voxels.vertical,
								paired_viewer.state.view_distance_voxels.horizontal
						)
				);

				const Box3i new_data_box =
						get_base_box_in_chunks(
								paired_viewer.state.local_position_voxels,
								// 确保距离是数据块大小的整数倍，以获得一致的盒子大小
								ld * lod_data_block_size,
								lod_data_block_size,
								// 使子 LOD 中的最小和最大坐标为偶数，以遵守细分规则。
								// 根 LOD 不需要遵守该规则，
								lod_index != lod_count - 1
						)
								.clipped(volume_bounds_in_data_blocks);

				// const Box3i new_data_box = get_lod_box_in_chunks(paired_viewer.state.local_position_voxels,
				// 		lod_distance_in_data_chunks, data_block_size_po2, lod_index)
				// 								   .clipped(volume_bounds_in_data_blocks);

				paired_viewer.state.data_box_per_lod[lod_index] = new_data_box;
			}
		}
	}
}

void remove_unpaired_viewers(
		const StdVector<unsigned int> &unpaired_viewers_to_remove,
		StdVector<VoxelLodTerrainUpdateData::PairedViewer> &paired_viewers
) {
	// 向后迭代，以便需要移除的配对观察者的索引不会因移除本身而改变
	for (auto it = unpaired_viewers_to_remove.rbegin(); it != unpaired_viewers_to_remove.rend(); ++it) {
		const unsigned int vi = *it;
		VOXEL_PRINT_VERBOSE(format("Unpairing viewer {} from VoxelLodTerrain", paired_viewers[vi].id));
		paired_viewers[vi] = paired_viewers.back();
		paired_viewers.pop_back();
	}
}

void add_loading_block(
		VoxelLodTerrainUpdateData::Lod &lod,
		Vector3i position,
		uint8_t lod_index,
		StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &blocks_to_load
) {
	auto it = lod.loading_blocks.find(position);

	if (it == lod.loading_blocks.end()) {
		// 第一个请求它的观察者
		VoxelLodTerrainUpdateData::LoadingDataBlock new_loading_block;
		new_loading_block.viewers.add();
		new_loading_block.cancellation_token = TaskCancellationToken::create();

		lod.loading_blocks.insert({ position, new_loading_block });

		blocks_to_load.push_back(
				VoxelLodTerrainUpdateData::BlockToLoad{
						VoxelLodTerrainUpdateData::BlockLocation{ position, lod_index },
						new_loading_block.cancellation_token //
				}
		);

	} else {
		// 已加载
		it->second.viewers.add();
	}
}

void unreference_data_block_from_loading_lists(
		StdUnorderedMap<Vector3i, VoxelLodTerrainUpdateData::LoadingDataBlock> &loading_blocks,
		StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &data_blocks_to_load,
		Vector3i bpos,
		unsigned int lod_index
) {
	auto loading_block_it = loading_blocks.find(bpos);
	if (loading_block_it == loading_blocks.end()) {
		VOXEL_PRINT_VERBOSE("Request to unview a loading block that was never requested");
		// 意外情况，但应该没问题
		return;
	}

	VoxelLodTerrainUpdateData::LoadingDataBlock &loading_block = loading_block_it->second;
	loading_block.viewers.remove();

	if (loading_block.viewers.get() == 0) {
		// 不再想加载它，没有任何数据盒子包含它

		if (loading_block.cancellation_token.is_valid()) {
			// 若任务仍在队列中则取消加载任务
			loading_block.cancellation_token.cancel();
		}

		loading_blocks.erase(loading_block_it);

		// 同时从即将加入加载队列的数据块中移除
		VoxelLodTerrainUpdateData::BlockLocation bloc{ bpos, static_cast<uint8_t>(lod_index) };
		for (size_t i = 0; i < data_blocks_to_load.size(); ++i) {
			if (data_blocks_to_load[i].loc == bloc) {
				data_blocks_to_load[i] = data_blocks_to_load.back();
				data_blocks_to_load.pop_back();
				// 我们不动取消令牌，因为任务尚未为这些数据块生成
				break;
			}
		}
	}
}

void process_data_blocks_sliding_box(
		VoxelLodTerrainUpdateData::State &state,
		VoxelData &data,
		StdVector<VoxelData::BlockToSave> *blocks_to_save,
		// TODO 我们应该能够在“盒子”级别工作以加载数据，这有助于压缩网络消息
		StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &data_blocks_to_load,
		const VoxelLodTerrainUpdateData::Settings &settings,
		int lod_count,
		bool can_load
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN_MSG(data.is_streaming_enabled(), "This function is not meant to run in full load mode");

	const int data_block_size_po2 = data.get_block_size_po2();
	const Box3i bounds_in_voxels = data.get_bounds();

	// const int mesh_to_data_factor = mesh_block_size / data_block_size;

	// const int lod_distance_in_mesh_chunks = get_lod_distance_in_mesh_chunks(settings.lod_distance, mesh_block_size);

	// // 数据块由网格块驱动，因为网格需要数据
	// const int lod_distance_in_data_chunks = lod_distance_in_mesh_chunks * mesh_to_data_factor
	// 		// 考虑到网格需要相邻数据块这一事实
	// 		+ 1;

	static thread_local StdVector<Vector3i> tls_missing_blocks;
	static thread_local StdVector<Vector3i> tls_found_blocks_positions;

#ifdef DEV_ENABLED
	Box3i debug_parent_box;
#endif

	for (const VoxelLodTerrainUpdateData::PairedViewer &paired_viewer : state.clipbox_streaming.paired_viewers) {
		// 从大 LOD 向小 LOD 迭代，以便在边界不相交时提前退出。
		for (int lod_index = lod_count - 1; lod_index >= 0; --lod_index) {
			VOXEL_PROFILE_SCOPE();
			VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

			// 每个 LOD 保存一个已加载数据块的盒子，只有部分数据块会被多边形化。
			// 玩家可以编辑它们，因此更改可以传播到更低的 LOD。

			const unsigned int lod_data_block_size_po2 = data_block_size_po2 + lod_index;

			// 只要边界大小是最大 LOD 数据块的整数倍，这应该是正确的
			const Box3i bounds_in_data_blocks =
					Box3i(bounds_in_voxels.position >> lod_data_block_size_po2,
						  bounds_in_voxels.size >> lod_data_block_size_po2);

			// const Box3i new_data_box = get_lod_box_in_chunks(
			// 		viewer_pos_in_lod0_voxels, lod_distance_in_data_chunks, data_block_size_po2, lod_index)
			// 								   .clipped(bounds_in_data_blocks);

			const Box3i &new_data_box = paired_viewer.state.data_box_per_lod[lod_index];
			const Box3i &prev_data_box = paired_viewer.prev_state.data_box_per_lod[lod_index];

#ifdef DEV_ENABLED
			if (lod_index + 1 != lod_count) {
				const Box3i debug_parent_box_in_current_lod(debug_parent_box.position << 1, debug_parent_box.size << 1);
				VOXEL_ASSERT(debug_parent_box_in_current_lod.contains(new_data_box));
			}
			debug_parent_box = new_data_box;
#endif

			// const Box3i prev_data_box = get_lod_box_in_chunks(
			// 		state.clipbox_streaming.viewer_pos_in_lod0_voxels_previous_update,
			// 		state.clipbox_streaming.lod_distance_in_data_chunks_previous_update, data_block_size_po2, lod_index)
			// 									.clipped(bounds_in_data_blocks);

			if (!new_data_box.intersects(bounds_in_data_blocks) && !prev_data_box.intersects(bounds_in_data_blocks)) {
				// 若此盒子现在或之前都不相交，则更小的盒子也不会有机会相交
				break;
			}

			if (prev_data_box != new_data_box) {
				// 检测需要加载的数据块。
				if (can_load) {
					tls_missing_blocks.clear();

					new_data_box.difference(prev_data_box, [&data, lod_index](Box3i box_to_load) {
						data.view_area(box_to_load, lod_index, &tls_missing_blocks, nullptr, nullptr);
					});

					{
						VOXEL_PROFILE_SCOPE_NAMED("Add loading blocks");
						MutexLock mlock(lod.loading_blocks_mutex);
						for (const Vector3i bpos : tls_missing_blocks) {
							add_loading_block(lod, bpos, lod_index, data_blocks_to_load);
						}
					}
				}

				// 检测需要卸载的数据块
				{
					tls_missing_blocks.clear();
					tls_found_blocks_positions.clear();

					const unsigned int to_save_index0 = blocks_to_save != nullptr ? blocks_to_save->size() : 0;

					prev_data_box.difference(new_data_box, [&data, blocks_to_save, lod_index](Box3i box_to_remove) {
						data.unview_area(
								box_to_remove,
								lod_index,
								&tls_found_blocks_positions,
								&tls_missing_blocks,
								blocks_to_save
						);
					});

					if (blocks_to_save != nullptr && blocks_to_save->size() > to_save_index0) {
						add_unloaded_saving_blocks(lod, to_span(*blocks_to_save).sub(to_save_index0));
					}

					// 无论引用计数如何都移除加载数据块（这些数据块已被加载且其引用计数已归零）
					if (tls_found_blocks_positions.size() > 0) {
						MutexLock mlock(lod.loading_blocks_mutex);
						for (const Vector3i bpos : tls_found_blocks_positions) {
							// emit_data_block_unloaded(bpos);

							// TODO 如果它们已加载，为什么会在加载数据块中？
							// 也许是为了确保无论如何它们都不在这里
							lod.loading_blocks.erase(bpos);
						}
					}

					// 移除加载数据块的引用计数，若计数归零则取消加载
					if (tls_missing_blocks.size() > 0) {
						MutexLock mlock(lod.loading_blocks_mutex);
						for (const Vector3i bpos : tls_missing_blocks) {
							unreference_data_block_from_loading_lists(
									lod.loading_blocks, data_blocks_to_load, bpos, lod_index
							);
						}
					}
				}
			}

			// 我已将其关闭，因为不记得当初为何添加它。将其保留，以防出现能揭示其
			// 存在原因的 bug。
			// 最初在 17c6b1f557c5abc447cb62c200afcff1298fadff 中引入
			// 也许是为了处理在我们到达此处之前列表中已有待处理的更新，因此需要某种
			// 取消它们的方法？但有了裁剪盒逻辑和多个观察者，这不再适用
#if 0
			// TODO 为什么我们要在这里做这个？听起来应该在网格裁剪盒逻辑中完成
			{
				VOXEL_PROFILE_SCOPE_NAMED("Cancel updates");
				// 取消不在填充区域内的网格数据块更新
				// （因为始终需要相邻数据块来重新网格化）

				// TODO 这可能会在地形边界处出错
				const Box3i padded_new_box = new_data_box.padded(-1);
				Box3i mesh_box;
				if (mesh_block_size > data_block_size) {
					const int factor = mesh_block_size / data_block_size;
					mesh_box = padded_new_box.downscaled_inner(factor);
				} else {
					mesh_box = padded_new_box;
				}

				unordered_remove_if(lod.mesh_blocks_pending_update,
						[&lod, mesh_box](const VoxelLodTerrainUpdateData::MeshToUpdate &mtu) {
							if (mesh_box.contains(mtu.position)) {
								return false;
							} else {
								auto mesh_block_it = lod.mesh_map_state.map.find(mtu.position);
								if (mesh_block_it != lod.mesh_map_state.map.end()) {
									mesh_block_it->second.state = VoxelLodTerrainUpdateData::MESH_NEED_UPDATE;
								}
								return true;
							}
						});
			}
#endif

		} // 每个 lod
	} // 每个 viewer

	// state.clipbox_streaming.lod_distance_in_data_chunks_previous_update = lod_distance_in_data_chunks;
}

// TODO 从八叉树流式加载文件中复制的代码
VoxelLodTerrainUpdateData::MeshBlockState &insert_new(
		StdUnorderedMap<Vector3i, VoxelLodTerrainUpdateData::MeshBlockState> &mesh_map,
		Vector3i pos
) {
#ifdef DEBUG_ENABLED
	// 我们之所以到这里，是因为映射中不包含该元素。若它已存在，那将是一个 bug。
	static VoxelLodTerrainUpdateData::MeshBlockState s_default;
	ERR_FAIL_COND_V(mesh_map.find(pos) != mesh_map.end(), s_default);
#endif
	// C++ 标准规定，若元素不存在，则会被默认构造。
	// 因此这里演示了如何将默认的、不可移动的结构体插入到 unordered_map 中。
	// https://stackoverflow.com/questions/22229773/map-unordered-map-with-non-movable-default-constructible-value-type
	VoxelLodTerrainUpdateData::MeshBlockState &block = mesh_map[pos];

	// 这种方法无法编译，不得不改用可写入的 [] 运算符。
	/*
	auto p = lod.mesh_map_state.map.emplace(pos, VoxelLodTerrainUpdateData::MeshBlockState());
	// 我们之所以到这里，是因为映射中不包含该元素。若它已存在，那将是一个 bug。
	CRASH_COND(p.second == false);
	*/

	return block;
}

inline Vector3i get_relative_child_position(unsigned int child_index) {
	return Vector3i( //
			(child_index & 1), //
			((child_index & 2) >> 1), //
			((child_index & 4) >> 2)
	);
}

inline Vector3i get_child_position(Vector3i parent_position, unsigned int child_index) {
	return parent_position * 2 + get_relative_child_position(child_index);
}

// void hide_children_recursive(
// 		VoxelLodTerrainUpdateData::State &state, unsigned int parent_lod_index, Vector3i parent_cpos) {
// 	VOXEL_ASSERT_RETURN(parent_lod_index > 0);
// 	const unsigned int lod_index = parent_lod_index - 1;
// 	VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

// 	for (unsigned int child_index = 0; child_index < 8; ++child_index) {
// 		const Vector3i cpos = get_child_position(parent_cpos, child_index);
// 		auto mesh_it = lod.mesh_map_state.map.find(cpos);

// 		if (mesh_it != lod.mesh_map_state.map.end()) {
// 			VoxelLodTerrainUpdateData::MeshBlockState &mesh_block = mesh_it->second;

// 			if (mesh_block.active) {
// 				mesh_block.active = false;
// 				lod.mesh_blocks_to_deactivate.push_back(cpos);

// 			} else if (lod_index > 0) {
// 				hide_children_recursive(state, lod_index, cpos);
// 			}
// 		}
// 	}
// }

inline void schedule_mesh_load(
		StdVector<VoxelLodTerrainUpdateData::MeshToUpdate> &update_list,
		Vector3i bpos,
		VoxelLodTerrainUpdateData::MeshBlockState &mesh_block,
		bool require_visual
) {
	// VOXEL_PROFILE_SCOPE();

	if (mesh_block.update_list_index != -1) {
		// 在任务被调度之前更新设置
		VoxelLodTerrainUpdateData::MeshToUpdate &u = update_list[mesh_block.update_list_index];
		u.require_visual |= require_visual;
	} else {
		TaskCancellationToken cancellation_token = TaskCancellationToken::create();
		mesh_block.update_list_index = update_list.size();
		update_list.push_back(VoxelLodTerrainUpdateData::MeshToUpdate{ bpos, cancellation_token, require_visual });
		mesh_block.cancellation_token = cancellation_token;
		// TODO `MESH_UPDATE_NOT_SENT` 现在与 `update_list_index` 冗余
		mesh_block.state = VoxelLodTerrainUpdateData::MESH_UPDATE_NOT_SENT;
	}

	// mesh_block.pending_update_has_visuals = require_visual;
}

void view_mesh_box(
		const Box3i box_to_add,
		VoxelLodTerrainUpdateData::Lod &lod,
		unsigned int lod_index,
		bool is_full_load_mode,
		int mesh_to_data_factor,
		const VoxelData &voxel_data,
		bool require_visuals,
		bool require_collisions
) {
	VOXEL_PROFILE_SCOPE();

	const Box3i bounds_in_data_blocks = voxel_data.get_bounds().downscaled(voxel_data.get_block_size() << lod_index);

	box_to_add.for_each_cell([&lod, //
							  is_full_load_mode, //
							  mesh_to_data_factor, //
							  &voxel_data,
							  lod_index, //
							  require_visuals, //
							  require_collisions, //
							  bounds_in_data_blocks](Vector3i bpos) {
		VoxelLodTerrainUpdateData::MeshBlockState *mesh_block;
		auto mesh_block_it = lod.mesh_map_state.map.find(bpos);

		if (mesh_block_it == lod.mesh_map_state.map.end()) {
			// RWLockWrite wlock(lod.mesh_map_state.map_lock);
			mesh_block = &insert_new(lod.mesh_map_state.map, bpos);

			// if (is_full_load_mode) {
			// 	// 由于所有内容在加载时一次性载入，因此我们直接触发网格构建，而无需
			// 	// 等待数据块加载完成后再做出反应
			// 	schedule_mesh_load(lod.mesh_blocks_pending_update, bpos, *mesh_block, require_visuals);
			// }

		} else {
			mesh_block = &mesh_block_it->second;
		}

		bool first_visuals = false;
		if (require_visuals) {
			first_visuals = mesh_block->mesh_viewers.get() == 0;
			mesh_block->mesh_viewers.add();
		}

		bool first_collision = false;
		if (require_collisions) {
			first_collision = mesh_block->collision_viewers.get() == 0;
			mesh_block->collision_viewers.add();
		}

		if (first_visuals || first_collision) {
			// TODO 优化：若已向任务系统发送了具有相同选项的更新，则不要再次调度。
			// 目前我们只对列表中尚未发送到任务系统的请求这样做。
			// 若在连续帧中于同一区域生成大量选项各不相同的观察者，这可能会成为问题。

			// TODO 优化：不要触发我们已为其调度过计算的任务。
			// 例如，若更新列表中没有任务，但最后调度的任务计算了碰撞而未计算视觉，
			// 当一个观察者请求视觉时，调度的任务只需计算视觉。重新计算碰撞没有必要，
			// 因为在此场景下网格不会改变。
			// （体素更改会触发每个引用计数选项的更新，并使用不同的代码路径）。
			// 但我们仍然会重新网格化（在某些情况下还会生成未经编辑的体素数据），
			// 为了避免这种情况，我们需要自己缓存网格数据。问题是 Godot 也在 ArrayMesh
			// 中缓存网格数据（但原因不同，因此不可靠），所以这会带来可观的额外内存开销。

			if (is_full_load_mode) {
				// 所有内容都是预先加载的，因此我们必须直接触发网格化，而不是
				// 响应已加载的数据块
				schedule_mesh_load(lod.mesh_blocks_pending_update, bpos, *mesh_block, require_visuals);

			} else {
				// 若数据已可用，（重新）触发网格化。
				// 这在流式加载模式下尤其需要，因为若数据已在那里，就不会有“数据已加载”事件可供
				// 响应。在此之前，网格只会在数据块被加载或修改时更新，
				// 因此更改数据块大小或观察者标志不会使网格出现。两个观察者
				// 区域相接也会引发问题。

				const Box3i data_box = Box3i(bpos * mesh_to_data_factor, Vector3iUtil::create(mesh_to_data_factor))
											   .padded(1)
											   .clipped(bounds_in_data_blocks);

				// 若此时得到空盒子，说明调用方有问题
				VOXEL_ASSERT_RETURN(!data_box.is_empty());

				const bool data_available = voxel_data.has_all_blocks_in_area_unbound(data_box, lod_index);

				if (data_available) {
					schedule_mesh_load(lod.mesh_blocks_pending_update, bpos, *mesh_block, require_visuals);
				}
				// 否则，我们将在数据加载时作出响应
			}
		}

#if 0
		// TODO 若这是第一个拥有视觉的观察者，则触发带视觉的网格更新。
		// 当发生这种情况时，忽略已有待处理网格更新的事实，除非它是以
		// 相同标志触发的。

		// 若数据已可用则触发网格化。
		// 这是必要的，因为否则就不会有“数据已加载”事件可供响应。
		// 在此之前，网格只会在数据块被加载或修改时更新，
		// 因此更改数据块大小或观察者标志不会使网格出现。两个
		// 观察者区域相接也会引发问题。
		//
		// TODO 这似乎表明数据块应该从这里分配
		// 而不是其他地方，但这会将网格加载与数据加载耦合，迫使在不需要网格的
		// 观察者的情况下复制数据代码路径。
		// try_schedule_mesh_update(*block);
		// 另一种方案：在数据差异中，将每个找到的数据块放入一个列表，我们也会在
		// `process_loaded_data_blocks_trigger_meshing` 中遍历该列表？
		//
		if (!is_full_load_mode && (!mesh_block->loaded || first_visuals) &&
				// 是否已有待处理的更新？
				mesh_block->state != VoxelLodTerrainUpdateData::MESH_UPDATE_NOT_SENT &&
				mesh_block->state != VoxelLodTerrainUpdateData::MESH_UPDATE_SENT) {
			//
			const Box3i data_box =
					Box3i(bpos * mesh_to_data_factor, Vector3iUtil::create(mesh_to_data_factor)).padded(1);

			// 若此时得到空盒子，说明调用方有问题
			VOXEL_ASSERT_RETURN(!data_box.is_empty());

			const bool data_available = voxel_data.has_all_blocks_in_area(data_box, lod_index);

			if (data_available) {
				schedule_mesh_load(lod.mesh_blocks_pending_update, bpos, *mesh_block, first_visuals);
			}
		}
#endif
	});
}

void unview_mesh_box(
		const Box3i out_of_range_box,
		VoxelLodTerrainUpdateData::Lod &lod,
		unsigned int lod_index,
		unsigned int lod_count,
		VoxelLodTerrainUpdateData::State &state,
		bool visual_flag,
		bool collision_flag
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(collision_flag || visual_flag);

	out_of_range_box.for_each_cell([&lod, visual_flag, collision_flag](Vector3i bpos) {
		auto mesh_block_it = lod.mesh_map_state.map.find(bpos);

		if (mesh_block_it != lod.mesh_map_state.map.end()) {
			VoxelLodTerrainUpdateData::MeshBlockState &mesh_block = mesh_block_it->second;

			bool visual_needed;
			if (visual_flag) {
				visual_needed = (mesh_block.mesh_viewers.remove() > 1); // 注意，remove() 返回先前的计数
			} else {
				visual_needed = mesh_block.mesh_viewers.get() > 0;
			}

			bool collision_needed;
			if (collision_flag) {
				collision_needed = (mesh_block.collision_viewers.remove() > 1);
			} else {
				collision_needed = mesh_block.collision_viewers.get() > 0;
			}

			if (collision_needed == false && visual_needed == false) {
				// 不再有观察者需要此网格

				if (mesh_block.cancellation_token.is_valid()) {
					mesh_block.cancellation_token.cancel();
				}

				if (mesh_block.update_list_index != -1) {
					unordered_remove(lod.mesh_blocks_pending_update, mesh_block.update_list_index);
					mesh_block.update_list_index = -1;
				}

				// TODO 如果一个观察者导致卸载，随后另一个观察者又更新并本会保持网格加载，
				// 会怎样？那将触发重新加载，但如果网格加载很快，主线程是否会因为先前的
				// 短暂卸载而卸载新网格？非常边缘的情况，但留个备忘，以防实践中出现异常。
				lod.mesh_map_state.map.erase(mesh_block_it);
				lod.mesh_blocks_to_unload.push_back(bpos);

			} else {
				// 数据块保留，但我们可以卸载其某个资源

				if (visual_flag && !visual_needed) {
					// 卸载图形以节省内存
					lod.mesh_blocks_to_drop_visual.push_back(bpos);
					// 注意，`visuals_loaded` 在它们真正被卸载之前将保持为 true。
				}

				if (collision_flag && !collision_needed) {
					// 卸载碰撞体以节省内存
					lod.mesh_blocks_to_drop_collision.push_back(bpos);
				}
			}
		}
	});

	// 当子节点被移除时立即显示父节点。
	// 这是一种廉价的方法，因为父网格大多数时候都可用。
	// 然而，在高速移动时，如果加载跟不上，就会在
	// 移动的反方向开始出现空洞和重叠。
	const unsigned int parent_lod_index = lod_index + 1;
	if (parent_lod_index < lod_count) {
		// 应该始终有效且不会归零，因为非最大 LOD 由于细分规则
		// 始终是 2 的倍数
		const Box3i parent_box = Box3i(out_of_range_box.position >> 1, out_of_range_box.size >> 1);

		VoxelLodTerrainUpdateData::Lod &parent_lod = state.lods[parent_lod_index];

		// 当子节点被移除时显示父节点
		parent_box.for_each_cell([&parent_lod, //
								  &lod, //
								  visual_flag, //
								  collision_flag //
		](Vector3i bpos) {
			auto mesh_it = parent_lod.mesh_map_state.map.find(bpos);

			if (mesh_it == parent_lod.mesh_map_state.map.end()) {
				return;
			}

			VoxelLodTerrainUpdateData::MeshBlockState &mesh_block = mesh_it->second;

			bool activated = false;

			if (visual_flag) {
				if (!mesh_block.visual_active) {
					// 仅在子数据块确实被移除时才执行合并逻辑。
					// 在多观察者场景中，裁剪盒可能已远离子 LOD 的数据块，
					// 但另一个观察者仍可能引用它们，因此我们不应
					// 立即合并它们。
					// 此检查假定子节点要么始终有 8 个，要么没有
					const Vector3i child_bpos0 = bpos << 1;
					auto child_mesh0_it = lod.mesh_map_state.map.find(child_bpos0);

					if (child_mesh0_it == lod.mesh_map_state.map.end() ||
						child_mesh0_it->second.mesh_viewers.get() == 0) {
						mesh_block.visual_active = true;
						parent_lod.mesh_blocks_to_activate_visuals.push_back(bpos);
						activated = true;
					}

					// 我们知道 parent_lod_index 必须 > 0
					// if (parent_lod_index > 0) {
					// 这实际上不会做任何事情，因为子节点已被移除
					// hide_children_recursive(state, parent_lod_index, bpos);
					// }
				}
			}
			if (collision_flag) {
				if (!mesh_block.collision_active) {
					const Vector3i child_bpos0 = bpos << 1;
					auto child_mesh0_it = lod.mesh_map_state.map.find(child_bpos0);

					if (child_mesh0_it == lod.mesh_map_state.map.end() ||
						child_mesh0_it->second.collision_viewers.get() == 0) {
						mesh_block.collision_active = true;
						parent_lod.mesh_blocks_to_activate_collision.push_back(bpos);
						activated = true;
					}
				}
			}

			if (activated) {
				// 网格的体素可能在其处于非活动状态时被修改过（尤其是 LOD），因此
				// 触发一次更新。
				//
				// TODO 这种方法会引起轻微闪烁。希望能找到避免的方法。
				// 在编辑后离开编辑区域，未及时更新的 LOD1 网格会立即出现，
				// 然后它们再更新，从而引起闪烁。
				// 最简单的方案是在父级 LOD 网格处于非活动状态时更新它们，但那样会
				// 使编辑变得不必要地昂贵（从技术上也会触发物理更新）。也许可以给这些
				// 更新非常低的任务优先级，但这仍然是并非立即可需要的工作。
				// 我曾尝试在裁剪盒移开时“延迟”数据块切换 LOD，但由于状态需要随时间
				// 持续，最终导致大量麻烦和边缘情况。
				// 这个问题在八叉树中不会出现，因为它逐个等待父网格就绪，
				// 完全不会利用已加载区域的形状（这在一定程度上也是它更慢的原因）
				if (mesh_block.state == VoxelLodTerrainUpdateData::MESH_NEED_UPDATE) {
					mesh_block.state = VoxelLodTerrainUpdateData::MESH_UPDATE_NOT_SENT;
					mesh_block.update_list_index = parent_lod.mesh_blocks_pending_update.size();
					parent_lod.mesh_blocks_pending_update.push_back(
							VoxelLodTerrainUpdateData::MeshToUpdate{
									bpos, TaskCancellationToken(), mesh_block.mesh_viewers.get() > 0 }
					);
				}
			}
		});
	}
}

inline bool requires_meshes(const VoxelLodTerrainUpdateData::PairedViewer::State &viewer_state) {
	return viewer_state.requires_collisions || viewer_state.requires_visuals;
}

void process_viewer_mesh_blocks_sliding_box(
		VoxelLodTerrainUpdateData::State &state,
		int mesh_block_size_po2,
		int lod_count,
		const Box3i &volume_bounds_in_voxels,
		const VoxelLodTerrainUpdateData::PairedViewer &paired_viewer,
		bool can_load,
		bool is_full_load_mode,
		int mesh_to_data_factor,
		const VoxelData &data
) {
	VOXEL_PROFILE_SCOPE();

#ifdef DEV_ENABLED
	Box3i debug_parent_box;
#endif

	// TODO 优化：当观察者不需要视觉时，我们只需为碰撞构建网格直到某个
	// LOD（碰撞最大 LOD 属性）。这将是针对服务器、NPC 和玩家主机的优化

	// 从大 LOD 向小 LOD 迭代，以便在边界不相交时提前退出。
	for (int lod_index = lod_count - 1; lod_index >= 0; --lod_index) {
		VOXEL_PROFILE_SCOPE();
		VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

		const int lod_mesh_block_size_po2 = mesh_block_size_po2 + lod_index;
		const int lod_mesh_block_size = 1 << lod_mesh_block_size_po2;
		// const Vector3i viewer_block_pos_within_lod = math::floor_to_int(p_viewer_pos) >> block_size_po2;

		const Box3i bounds_in_mesh_blocks = volume_bounds_in_voxels.downscaled(lod_mesh_block_size);

		// const Box3i new_mesh_box = get_lod_box_in_chunks(
		// 		viewer_pos_in_lod0_voxels, lod_distance_in_mesh_chunks, mesh_block_size_po2, lod_index)
		// 								   .clipped(bounds_in_mesh_blocks);

		const Box3i &new_mesh_box = paired_viewer.state.mesh_box_per_lod[lod_index];
		const Box3i &prev_mesh_box = paired_viewer.prev_state.mesh_box_per_lod[lod_index];

#ifdef DEV_ENABLED
		if (lod_index + 1 != lod_count) {
			const Box3i debug_parent_box_in_current_lod(debug_parent_box.position << 1, debug_parent_box.size << 1);
			VOXEL_ASSERT(debug_parent_box_in_current_lod.contains(new_mesh_box));
		}
		debug_parent_box = new_mesh_box;
#endif

		// const Box3i prev_mesh_box = get_lod_box_in_chunks(
		// 		state.clipbox_streaming.viewer_pos_in_lod0_voxels_previous_update,
		// 		state.clipbox_streaming.lod_distance_in_mesh_chunks_previous_update, mesh_block_size_po2, lod_index)
		// 									.clipped(bounds_in_mesh_blocks);

		if (!new_mesh_box.intersects(bounds_in_mesh_blocks) && !prev_mesh_box.intersects(bounds_in_mesh_blocks)) {
			// 若此盒子现在或之前都不相交，则更小的盒子也不会有机会相交
			break;
		}

		if (prev_mesh_box != new_mesh_box) {
			RWLockWrite wlock(lod.mesh_map_state.map_lock);

			// 添加进入范围的网格
			if (requires_meshes(paired_viewer.state) && can_load) {
				SmallVector<Box3i, 6> new_mesh_boxes;
				new_mesh_box.difference_to_vec(prev_mesh_box, new_mesh_boxes);

				for (const Box3i &box_to_add : new_mesh_boxes) {
					view_mesh_box(
							box_to_add,
							lod,
							lod_index,
							is_full_load_mode,
							mesh_to_data_factor,
							data,
							paired_viewer.state.requires_visuals,
							paired_viewer.state.requires_collisions
					);
				}
			}

			// 移除超出范围的网格
			if (requires_meshes(paired_viewer.prev_state)) {
				SmallVector<Box3i, 6> old_mesh_boxes;
				prev_mesh_box.difference_to_vec(new_mesh_box, old_mesh_boxes);

				for (const Box3i &out_of_range_box : old_mesh_boxes) {
					unview_mesh_box(
							out_of_range_box,
							lod,
							lod_index,
							lod_count,
							state,
							// 使用上一个状态，因为旧盒子是由它们加载的
							paired_viewer.prev_state.requires_visuals,
							paired_viewer.prev_state.requires_collisions
					);
				}
			}
		}

		// 处理观察者标志在运行时的变化。不过，除了临时的编辑器操作之外，我暂时想不到
		// 什么使用场景。这种情况应该很少见，或者根本不会发生。
		// 这操作的是与上述不同的数据块集合。
		// 另外，对于没有先前状态的新观察者，这不会做任何事情，因为上一个盒子将是
		// 空的。
		if (!Vector3iUtil::is_empty_size(prev_mesh_box.size)) {
			if (paired_viewer.state.requires_collisions != paired_viewer.prev_state.requires_collisions) {
				const Box3i box = new_mesh_box.clipped(prev_mesh_box);
				if (paired_viewer.state.requires_collisions) {
					// 仅增加碰撞的引用计数
					view_mesh_box(box, lod, lod_index, is_full_load_mode, mesh_to_data_factor, data, false, true);
				} else {
					// 仅移除碰撞的引用计数
					unview_mesh_box(box, lod, lod_index, lod_count, state, false, true);
				}
			}

			if (paired_viewer.state.requires_visuals != paired_viewer.prev_state.requires_visuals) {
				const Box3i box = new_mesh_box.clipped(prev_mesh_box);
				if (paired_viewer.state.requires_visuals) {
					view_mesh_box(box, lod, lod_index, is_full_load_mode, mesh_to_data_factor, data, true, false);
				} else {
					unview_mesh_box(box, lod, lod_index, lod_count, state, true, false);
				}
			}
		}

		// {
		// 	VOXEL_PROFILE_SCOPE_NAMED("Cancel updates");
		// 	// 取消不在新区域内的区块更新
		// 	unordered_remove_if(lod.mesh_blocks_pending_update,
		// 			[new_mesh_box](const VoxelLodTerrainUpdateData::MeshToUpdate &mtu) { //
		// 				return !new_mesh_box.contains(mtu.position);
		// 			});
		// }
	}
}

void process_mesh_blocks_sliding_box(
		VoxelLodTerrainUpdateData::State &state,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const Box3i bounds_in_voxels,
		int lod_count,
		bool is_full_load_mode,
		bool can_load,
		const VoxelData &data,
		int data_block_size
) {
	VOXEL_PROFILE_SCOPE();

	const int mesh_block_size_po2 = settings.mesh_block_size_po2;

	// const int lod_distance_in_mesh_chunks = get_lod_distance_in_mesh_chunks(settings.lod_distance, mesh_block_size);

	const int mesh_block_size = 1 << mesh_block_size_po2;
	const int mesh_to_data_factor = mesh_block_size / data_block_size;

	for (const VoxelLodTerrainUpdateData::PairedViewer &paired_viewer : state.clipbox_streaming.paired_viewers) {
		// 只围绕需要网格的观察者进行更新。
		// 也要检查上一个状态，以防需要处理它们的变化
		if (requires_meshes(paired_viewer.state) || requires_meshes(paired_viewer.prev_state)) {
			process_viewer_mesh_blocks_sliding_box(
					state,
					mesh_block_size_po2,
					lod_count,
					bounds_in_voxels,
					paired_viewer,
					can_load,
					is_full_load_mode,
					mesh_to_data_factor,
					data
			);
		}
	}

	// VoxelLodTerrainUpdateData::ClipboxStreamingState &clipbox_streaming = state.clipbox_streaming;
	// clipbox_streaming.lod_distance_in_mesh_chunks_previous_update = lod_distance_in_mesh_chunks;
}

void process_loaded_data_blocks_trigger_meshing(
		const VoxelData &data,
		VoxelLodTerrainUpdateData::State &state,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const Box3i bounds_in_voxels
) {
	VOXEL_PROFILE_SCOPE();
	// 此函数只应在数据流式加载开启时使用。
	// 当所有内容都已加载时，还假定数据块可以即时生成，因此加载事件只会
	// 稀疏地出现在已编辑区域。所以在响应数据加载时触发网格化没有多大意义。
	VOXEL_ASSERT_RETURN(data.is_streaming_enabled());

	const int mesh_block_size_po2 = settings.mesh_block_size_po2;

	VoxelLodTerrainUpdateData::ClipboxStreamingState &clipbox_streaming = state.clipbox_streaming;

	// 获取自上次更新以来加载的数据块列表
	static thread_local StdVector<VoxelLodTerrainUpdateData::BlockLocation> tls_loaded_blocks;
	tls_loaded_blocks.clear();
	{
		MutexLock mlock(clipbox_streaming.loaded_data_blocks_mutex);
		append_array(tls_loaded_blocks, clipbox_streaming.loaded_data_blocks);
		clipbox_streaming.loaded_data_blocks.clear();
	}

	// TODO 内存池化
	FixedArray<StdUnorderedSet<Vector3i>, constants::MAX_LOD> checked_mesh_blocks_per_lod;

	const int data_to_mesh_shift = mesh_block_size_po2 - data.get_block_size_po2();

	for (VoxelLodTerrainUpdateData::BlockLocation bloc : tls_loaded_blocks) {
		// VOXEL_PROFILE_SCOPE_NAMED("Block");
		// 由于相邻依赖，可能有多个网格数据块感兴趣。

		// 我们可以按 LOD 对已加载的数据块进行分组，从而减少一些计算次数？
		const int lod_data_block_size_po2 = data.get_block_size_po2() + bloc.lod;
		const Box3i bounds_in_data_blocks =
				Box3i(bounds_in_voxels.position >> lod_data_block_size_po2,
					  bounds_in_voxels.size >> lod_data_block_size_po2);

		const Box3i data_neighboring =
				Box3i(bloc.position - Vector3i(1, 1, 1), Vector3i(3, 3, 3)).clipped(bounds_in_data_blocks);

		StdUnorderedSet<Vector3i> &checked_mesh_blocks = checked_mesh_blocks_per_lod[bloc.lod];
		VoxelLodTerrainUpdateData::Lod &lod = state.lods[bloc.lod];

		const unsigned int lod_index = bloc.lod;

		data_neighboring.for_each_cell([data_to_mesh_shift,
										&checked_mesh_blocks,
										&lod,
										&data,
										lod_index,
										&bounds_in_data_blocks](Vector3i data_bpos) {
			// VOXEL_PROFILE_SCOPE_NAMED("Cell");

			const Vector3i mesh_block_pos = data_bpos >> data_to_mesh_shift;
			if (!checked_mesh_blocks.insert(mesh_block_pos).second) {
				// 已检查过
				return;
			}

			// 我们不会在这里向映射添加/移除条目，只有更新任务可以这样做，因此无需
			// 加锁
			// RWLockRead rlock(lod.mesh_map_state.map_lock);
			auto mesh_it = lod.mesh_map_state.map.find(mesh_block_pos);
			if (mesh_it == lod.mesh_map_state.map.end()) {
				// 未被请求
				return;
			}
			VoxelLodTerrainUpdateData::MeshBlockState &mesh_block = mesh_it->second;
			const VoxelLodTerrainUpdateData::MeshState mesh_state = mesh_block.state;

			// TODO 检查网格是否还有更多需要计算的标志（碰撞体？渲染？）
			if (mesh_state != VoxelLodTerrainUpdateData::MESH_NEED_UPDATE &&
				mesh_state != VoxelLodTerrainUpdateData::MESH_NEVER_UPDATED) {
				// 已更新或正在更新
				return;
			}

			bool data_available = true;
			// if (data.is_streaming_enabled()) {
			const Box3i data_box = Box3i((mesh_block_pos << data_to_mesh_shift) - Vector3i(1, 1, 1),
										 Vector3iUtil::create((1 << data_to_mesh_shift) + 2))
										   .clipped(bounds_in_data_blocks);
			// TODO 预先执行一次网格查询，它们会重叠，因此我们在做冗余查找！
			data_available = data.has_all_blocks_in_area_unbound(data_box, lod_index);
			// } else {
			// 	if (!data.is_full_load_completed()) {
			// 		VOXEL_PRINT_ERROR("This function should not run until full load has completed");
			// 	}
			// }

			if (data_available) {
				schedule_mesh_load(
						lod.mesh_blocks_pending_update, mesh_block_pos, mesh_block, mesh_block.mesh_viewers.get() > 0
				);
				// 我们假定此后数据块不会卸载，直到数据被收集，因为卸载
				// 在此逻辑之前运行。
			}
		});
	}
}

// void debug_dump_mesh_maps(const VoxelLodTerrainUpdateData::State &state, unsigned int lod_count) {
// 	std::ofstream ofs("ddd_meshmaps.json", std::ios::binary | std::ios::trunc);
// 	ofs << "[";
// 	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
// 		const VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
// 		if (lod_index > 0) {
// 			ofs << ",";
// 		}
// 		ofs << "[";
// 		for (auto it = lod.mesh_map_state.map.begin(); it != lod.mesh_map_state.map.end(); ++it) {
// 			const Vector3i pos = it->first;
// 			const VoxelLodTerrainUpdateData::MeshBlockState &ms = it->second;
// 			if (it != lod.mesh_map_state.map.begin()) {
// 				ofs << ",";
// 			}
// 			ofs << "[";
// 			ofs << "[";
// 			ofs << pos.x;
// 			ofs << ",";
// 			ofs << pos.y;
// 			ofs << ",";
// 			ofs << pos.z;
// 			ofs << "],";
// 			ofs << static_cast<int>(ms.state);
// 			ofs << ",";
// 			ofs << ms.active;
// 			ofs << ",";
// 			ofs << ms.loaded;
// 			ofs << ",";
// 			ofs << ms.mesh_viewers.get();
// 			ofs << "]";
// 		}
// 		ofs << "]";
// 	}
// 	ofs << "]";
// 	ofs.close();
// }

enum MeshBlockFeatureIndex { //
	MESH_VISUAL = 0,
	MESH_COLLIDER = 1
};

bool is_loaded(const VoxelLodTerrainUpdateData::MeshBlockState &ms, MeshBlockFeatureIndex i) {
	switch (i) {
		case MESH_VISUAL:
			return ms.visual_loaded;
		case MESH_COLLIDER:
			return ms.collision_loaded;
		default:
			VOXEL_CRASH();
			return false;
	}
}

bool is_active(const VoxelLodTerrainUpdateData::MeshBlockState &ms, MeshBlockFeatureIndex i) {
	switch (i) {
		case MESH_VISUAL:
			return ms.visual_active;
		case MESH_COLLIDER:
			return ms.collision_active;
		default:
			VOXEL_CRASH();
			return false;
	}
}

void set_active(
		VoxelLodTerrainUpdateData::MeshBlockState &mesh_block,
		MeshBlockFeatureIndex feature_index,
		VoxelLodTerrainUpdateData::Lod &lod,
		const Vector3i bpos
) {
	switch (feature_index) {
		case MESH_VISUAL:
			if (!mesh_block.visual_active) {
				mesh_block.visual_active = true;
				lod.mesh_blocks_to_activate_visuals.push_back(bpos);
			}
			break;
		case MESH_COLLIDER:
			if (!mesh_block.collision_active) {
				mesh_block.collision_active = true;
				lod.mesh_blocks_to_activate_collision.push_back(bpos);
			}
			break;
		default:
			VOXEL_CRASH();
			break;
	}
}

void set_inactive(
		VoxelLodTerrainUpdateData::MeshBlockState &mesh_block,
		MeshBlockFeatureIndex feature_index,
		VoxelLodTerrainUpdateData::Lod &lod,
		const Vector3i bpos
) {
	switch (feature_index) {
		case MESH_VISUAL:
			if (mesh_block.visual_active) {
				mesh_block.visual_active = false;
				lod.mesh_blocks_to_deactivate_visuals.push_back(bpos);
			}
			break;
		case MESH_COLLIDER:
			if (mesh_block.collision_active) {
				mesh_block.collision_active = false;
				lod.mesh_blocks_to_deactivate_collision.push_back(bpos);
			}
			break;
		default:
			VOXEL_CRASH();
			break;
	}
}

// 当网格数据块加载完成时激活它们。在可能的情况下激活更高 LOD 并隐藏更低 LOD。
// 这实质上运行八叉树细分逻辑，但仅从特定节点及其后代开始。
void update_mesh_block_load(
		VoxelLodTerrainUpdateData::State &state,
		Vector3i bpos,
		unsigned int lod_index,
		unsigned int lod_count,
		MeshBlockFeatureIndex feature_index
) {
	VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
	auto mesh_it = lod.mesh_map_state.map.find(bpos);

	if (mesh_it == lod.mesh_map_state.map.end()) {
		return;
	}
	VoxelLodTerrainUpdateData::MeshBlockState &mesh_block = mesh_it->second;

	if (!is_loaded(mesh_block, feature_index)) {
		return;
	}

	// 网格已按指定标志加载

	const unsigned int parent_lod_index = lod_index + 1;
	if (parent_lod_index == lod_count) {
		// 根节点
		// 这里无需担心细分规则（无需检查兄弟节点），因为没有父节点

		// TODO 若数据块已被细分，则不要激活它
		set_active(mesh_block, feature_index, lod, bpos);

		if (lod_index > 0) {
			const unsigned int child_lod_index = lod_index - 1;
			for (unsigned int child_index = 0; child_index < 8; ++child_index) {
				const Vector3i child_bpos = get_child_position(bpos, child_index);
				update_mesh_block_load(state, child_bpos, child_lod_index, lod_count, feature_index);
			}
		}

	} else {
		// 非根节点
		// 我们必须考虑兄弟节点，因为不能一次只激活一个，必须是全有或全无

		const Vector3i parent_bpos = bpos >> 1;
		VoxelLodTerrainUpdateData::Lod &parent_lod = state.lods[parent_lod_index];

		auto parent_mesh_it = parent_lod.mesh_map_state.map.find(parent_bpos);
		// if (parent_mesh_it == parent_lod.mesh_map_state.map.end()) {
		// 	debug_dump_mesh_maps(state, lod_count);
		// }
		// 父节点必须存在，因为滑动盒子相互包含。也许将来并不总是如此，
		// 如果观察者有特殊行为的话？
		VOXEL_ASSERT_RETURN_MSG(
				parent_mesh_it != parent_lod.mesh_map_state.map.end(), "Expected parent due to subdivision rules, bug?"
		);

		VoxelLodTerrainUpdateData::MeshBlockState &parent_mesh_block = parent_mesh_it->second;

		if (is_active(parent_mesh_block, feature_index)) {
			bool all_siblings_loaded = true;

			// 测试所有兄弟节点是否都已加载
			// TODO 这需要优化。在父节点中存储缓存？
			for (unsigned int sibling_index = 0; sibling_index < 8; ++sibling_index) {
				const Vector3i sibling_bpos = get_child_position(parent_bpos, sibling_index);
				auto sibling_it = lod.mesh_map_state.map.find(sibling_bpos);
				if (sibling_it == lod.mesh_map_state.map.end()) {
					// 由于细分规则，在网格映射中找到这个会很奇怪。我们不期望兄弟节点
					// 缺失，因为每个网格数据块始终有 8 个子节点。
					VOXEL_PRINT_ERROR("Didn't expect missing sibling");
					all_siblings_loaded = false;
					break;
				}
				const VoxelLodTerrainUpdateData::MeshBlockState &sibling = sibling_it->second;
				if (!is_loaded(sibling, feature_index)) {
					all_siblings_loaded = false;
					break;
				}
			}

			if (all_siblings_loaded) {
				// 隐藏父节点
				set_inactive(parent_mesh_block, feature_index, parent_lod, parent_bpos);

				// 显示兄弟节点
				for (unsigned int sibling_index = 0; sibling_index < 8; ++sibling_index) {
					const Vector3i sibling_bpos = get_child_position(parent_bpos, sibling_index);
					auto sibling_it = lod.mesh_map_state.map.find(sibling_bpos);
					VoxelLodTerrainUpdateData::MeshBlockState &sibling = sibling_it->second;
					// TODO 优化：若该兄弟节点自身会被细分，则无需将其设为可见。
					// 也许可以让 `update_mesh_block_load` 返回该信息，从而避免调度激活？
					set_active(sibling, feature_index, lod, sibling_bpos);

					if (lod_index > 0) {
						// 检查子节点是否也已加载
						const unsigned int child_lod_index = lod_index - 1;
						for (unsigned int child_index = 0; child_index < 8; ++child_index) {
							const Vector3i child_bpos = get_child_position(sibling_bpos, child_index);
							update_mesh_block_load(state, child_bpos, child_lod_index, lod_count, feature_index);
						}
					}
				}
			}
		}
	}
}

void process_loaded_mesh_blocks_trigger_visibility_changes(
		VoxelLodTerrainUpdateData::State &state,
		unsigned int lod_count
) {
	VOXEL_PROFILE_SCOPE();

	VoxelLodTerrainUpdateData::ClipboxStreamingState &clipbox_streaming = state.clipbox_streaming;

	// 获取自上次更新以来加载的网格数据块列表
	// TODO 可考虑用于 TempAllocator
	static thread_local StdVector<VoxelLodTerrainUpdateData::LoadedMeshBlockEvent> tls_loaded_blocks;
	tls_loaded_blocks.clear();
	{
		// 如果这里有争用，我们可以尝试加锁并在失败时跳过
		MutexLock mlock(clipbox_streaming.loaded_mesh_blocks_mutex);
		append_array(tls_loaded_blocks, clipbox_streaming.loaded_mesh_blocks);
		clipbox_streaming.loaded_mesh_blocks.clear();
	}

	for (const VoxelLodTerrainUpdateData::LoadedMeshBlockEvent event : tls_loaded_blocks) {
		// TODO 这并非最优。若同时需要视觉和碰撞，执行此操作的成本会翻倍。
		if (event.visual) {
			update_mesh_block_load(state, event.position, event.lod_index, lod_count, MESH_VISUAL);
		}
		// TODO 我们不应在没有碰撞的 LOD 上运行此操作
		if (event.collision) {
			update_mesh_block_load(state, event.position, event.lod_index, lod_count, MESH_COLLIDER);
		}
	}

	{
		uint32_t lods_to_update_transitions = 0;
		for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
			VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
			// 仅在视觉变化时更新过渡掩码，这是一个渲染特性
			if (lod.mesh_blocks_to_activate_visuals.size() > 0 || lod.mesh_blocks_to_deactivate_visuals.size() > 0) {
				lods_to_update_transitions |= (0b111 << lod_index);
			}
		}
		// TODO 这相当慢（参见实现）。
		// 也许可以用裁剪盒逻辑来优化（更新可以按新/旧盒子分组，
		// 但现状不可行，因为网格更新需要时间才能真正可见。也许
		// 也可以以某种方式增量更新掩码？）。最初添加此流式加载系统是为了帮助
		// 提升服务器端性能。此功能仅限客户端，所以暂时不需要过度优化。
		update_transition_masks(state, lods_to_update_transitions, lod_count, true);
	}
}

} // namespace

void process_clipbox_streaming(
		VoxelLodTerrainUpdateData::State &state,
		VoxelData &data,
		Span<const std::pair<ViewerID, VoxelEngine::Viewer>> viewers,
		const Transform3D &volume_transform,
		StdVector<VoxelData::BlockToSave> *data_blocks_to_save,
		StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &data_blocks_to_load,
		const VoxelLodTerrainUpdateData::Settings &settings,
		bool can_load,
		bool can_mesh
) {
	VOXEL_PROFILE_SCOPE();

	const unsigned int lod_count = data.get_lod_count();
	const Box3i bounds_in_voxels = data.get_bounds();
	const unsigned int data_block_size_po2 = data.get_block_size_po2();
	const bool streaming_enabled = data.is_streaming_enabled();
	const bool full_load_completed = data.is_full_load_completed();

	StdVector<unsigned int> unpaired_viewers_to_remove;

	process_viewers(
			state.clipbox_streaming,
			settings,
			lod_count,
			viewers,
			volume_transform,
			bounds_in_voxels,
			data_block_size_po2,
			can_mesh,
			unpaired_viewers_to_remove
	);

	if (streaming_enabled) {
		process_data_blocks_sliding_box(
				state, data, data_blocks_to_save, data_blocks_to_load, settings, lod_count, can_load
		);
	} else {
		if (full_load_completed == false) {
			// 在加载完成之前不做任何事情，因为当网格数据块被创建时我们会直接触发网格化。
			// 如果在此之前运行，网格数据块会被创建，但我们将无法判断何时
			// 对每个数据块触发网格化。不过，如果将来需要这样做，我们可以在
			// “完全加载”状态变为 true 时对其进行差异对比并迭代所有网格数据块？
			return;
		}
	}

	process_mesh_blocks_sliding_box(
			state, settings, bounds_in_voxels, lod_count, !streaming_enabled, can_load, data, 1 << data_block_size_po2
	);

	// 在盒子差异之后移除配对观察者，因为我们将观察者移除解释为盒子大小归零，因此
	// 在真正移除它们之前，需要一步处理来应对这一点
	remove_unpaired_viewers(unpaired_viewers_to_remove, state.clipbox_streaming.paired_viewers);

	if (streaming_enabled) {
		// TODO 提供完全关闭网格化的选项（如果游戏不使用网格碰撞体，对服务器可能有用）
		process_loaded_data_blocks_trigger_meshing(data, state, settings, bounds_in_voxels);
	}

	process_loaded_mesh_blocks_trigger_visibility_changes(state, lod_count);

	// state.clipbox_streaming.viewer_pos_in_lod0_voxels_previous_update = viewer_pos_in_lod0_voxels;
}

} // namespace voxel
