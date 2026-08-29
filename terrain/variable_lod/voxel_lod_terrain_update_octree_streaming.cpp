#include "../../storage/voxel_data.h"
#include "../../util/math/conv.h"
#include "voxel_lod_terrain_update_data.h"
#include "voxel_lod_terrain_update_task.h"

namespace voxel {

namespace {

void process_unload_data_blocks_sliding_box(
		VoxelLodTerrainUpdateData::State &state,
		VoxelData &data,
		Vector3 p_viewer_pos,
		StdVector<VoxelData::BlockToSave> *blocks_to_save,
		const VoxelLodTerrainUpdateData::Settings &settings
) {
	VOXEL_PROFILE_SCOPE_NAMED("Sliding box data unload");
	// TODO 对所有数据块进行滚动更新是否真的就足够了？

	VOXEL_ASSERT_RETURN_MSG(data.is_streaming_enabled(), "This function is not meant to run in full load mode");

	// 相对于每个 LOD，这应该是相同的距离
	const int data_block_size = data.get_block_size();
	const int data_block_size_po2 = data.get_block_size_po2();
	const int data_block_region_extent =
			VoxelEngine::get_octree_lod_block_region_extent(settings.lod_distance, data_block_size);
	const Box3i bounds_in_voxels = data.get_bounds();

	const int mesh_block_size = 1 << settings.mesh_block_size_po2;

	const int lod_count = data.get_lod_count();

	// 忽略最大的 LOD，因为它可能因视距设置而略微超出范围。
	// 这些数据块改由八叉树林管理来卸载。
	// TODO 在哪里？
	//
	// 从大 LOD 向小 LOD 迭代，以便在边界不相交时提前退出。
	for (int lod_index = lod_count - 2; lod_index >= 0; --lod_index) {
		VOXEL_PROFILE_SCOPE();
		VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

		// 每个 LOD 保存一个已加载数据块的盒子，只有部分数据块会被多边形化。
		// 玩家可以编辑它们，因此更改可以传播到更低的 LOD。

		const unsigned int block_size_po2 = data_block_size_po2 + lod_index;
		const Vector3i viewer_block_pos_within_lod =
				VoxelDataMap::voxel_to_block_b(math::floor_to_int(p_viewer_pos), block_size_po2);

		// 只要边界大小是最大 LOD 数据块的整数倍，这应该是正确的
		const Box3i bounds_in_blocks =
				Box3i(bounds_in_voxels.position >> block_size_po2, //
					  bounds_in_voxels.size >> block_size_po2);

		const Box3i new_box =
				Box3i::from_center_extents(viewer_block_pos_within_lod, Vector3iUtil::create(data_block_region_extent));
		const Box3i prev_box = Box3i::from_center_extents(
				lod.last_viewer_data_block_pos, Vector3iUtil::create(lod.last_view_distance_data_blocks)
		);

		if (!new_box.intersects(bounds_in_blocks) && !prev_box.intersects(bounds_in_blocks)) {
			// 若此盒子现在或之前都不相交，则更小的盒子也不会有机会相交
			break;
		}

		if (prev_box != new_box) {
			// 清除不再需要的挂起数据块
			VOXEL_PROFILE_SCOPE_NAMED("Unload data");

			// VoxelDataLodMap::Lod &data_lod = data.lods[lod_index];
			// RWLockWrite wlock(data_lod.map_lock);

			const unsigned int to_save_index0 = blocks_to_save != nullptr ? blocks_to_save->size() : 0;

			prev_box.difference(new_box, [&data, &blocks_to_save, lod_index](Box3i box_to_remove) {
				data.unload_blocks(box_to_remove, lod_index, blocks_to_save);
			});

			if (blocks_to_save != nullptr && blocks_to_save->size() > to_save_index0) {
				add_unloaded_saving_blocks(lod, to_span(*blocks_to_save).sub(to_save_index0));
			}
		}

		{
			VOXEL_PROFILE_SCOPE_NAMED("Cancel updates");
			// 取消不在填充区域内的数据块更新
			// （因为始终需要相邻数据块来重新网格化）

			const Box3i padded_new_box = new_box.padded(-1);
			Box3i mesh_box;
			if (mesh_block_size > data_block_size) {
				const int factor = mesh_block_size / data_block_size;
				mesh_box = padded_new_box.downscaled_inner(factor);
			} else {
				mesh_box = padded_new_box;
			}

			unordered_remove_if(
					lod.mesh_blocks_pending_update,
					[&lod, mesh_box](const VoxelLodTerrainUpdateData::MeshToUpdate &mtl) {
						if (mesh_box.contains(mtl.position)) {
							return false;
						} else {
							auto mesh_block_it = lod.mesh_map_state.map.find(mtl.position);
							if (mesh_block_it != lod.mesh_map_state.map.end()) {
								mesh_block_it->second.state = VoxelLodTerrainUpdateData::MESH_NEED_UPDATE;
							}
							return true;
						}
					}
			);
		}

		lod.last_viewer_data_block_pos = viewer_block_pos_within_lod;
		lod.last_view_distance_data_blocks = data_block_region_extent;
	}
}

void process_unload_mesh_blocks_sliding_box(
		VoxelLodTerrainUpdateData::State &state,
		Vector3 p_viewer_pos,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const VoxelData &data
) {
	VOXEL_PROFILE_SCOPE_NAMED("Sliding box mesh unload");
	// TODO 对所有数据块进行滚动更新是否真的就足够了？

	// 相对于每个 LOD，这应该是相同的距离
	const int mesh_block_size_po2 = settings.mesh_block_size_po2;
	const int mesh_block_size = 1 << mesh_block_size_po2;
	const int mesh_block_region_extent =
			VoxelEngine::get_octree_lod_block_region_extent(settings.lod_distance, mesh_block_size);
	const int lod_count = data.get_lod_count();
	const Box3i bounds_in_voxels = data.get_bounds();

	// 忽略最大的 LOD，因为它可能因视距设置而略微超出范围。
	// 这些数据块改由八叉树林管理来卸载。
	// 从大 LOD 向小 LOD 迭代，以便在边界不相交时提前退出。
	for (int lod_index = lod_count - 2; lod_index >= 0; --lod_index) {
		VOXEL_PROFILE_SCOPE();
		VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

		unsigned int block_size_po2 = mesh_block_size_po2 + lod_index;
		const Vector3i viewer_block_pos_within_lod = math::floor_to_int(p_viewer_pos) >> block_size_po2;

		const Box3i bounds_in_blocks =
				Box3i(bounds_in_voxels.position >> block_size_po2, //
					  bounds_in_voxels.size >> block_size_po2);

		const Box3i new_box =
				Box3i::from_center_extents(viewer_block_pos_within_lod, Vector3iUtil::create(mesh_block_region_extent));
		const Box3i prev_box = Box3i::from_center_extents(
				lod.last_viewer_mesh_block_pos, Vector3iUtil::create(lod.last_view_distance_mesh_blocks)
		);

		if (!new_box.intersects(bounds_in_blocks) && !prev_box.intersects(bounds_in_blocks)) {
			// 若此盒子现在或之前都不相交，则更小的盒子也不会有机会相交
			break;
		}

		// 清除不再需要的挂起数据块

		if (prev_box != new_box) {
			VOXEL_PROFILE_SCOPE_NAMED("Unload meshes");
			RWLockWrite wlock(lod.mesh_map_state.map_lock);
			prev_box.difference(new_box, [&lod](Box3i out_of_range_box) {
				out_of_range_box.for_each_cell([&lod](Vector3i pos) {
					// print_line(String("Immerge {0}").format(varray(pos.to_vec3())));
					// unload_mesh_block(pos, lod_index);
					lod.mesh_map_state.map.erase(pos);
					lod.mesh_blocks_to_unload.push_back(pos);
				});
			});
		}

		{
			VOXEL_PROFILE_SCOPE_NAMED("Cancel updates");
			// 取消不在新区域内的数据块更新
			unordered_remove_if(
					lod.mesh_blocks_pending_update,
					[new_box](const VoxelLodTerrainUpdateData::MeshToUpdate &mtu) { //
						return !new_box.contains(mtu.position);
					}
			);
		}

		lod.last_viewer_mesh_block_pos = viewer_block_pos_within_lod;
		lod.last_view_distance_mesh_blocks = mesh_block_region_extent;
	}
}

void process_octrees_sliding_box(
		VoxelLodTerrainUpdateData::State &state,
		Vector3 p_viewer_pos,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const VoxelData &data
) {
	VOXEL_PROFILE_SCOPE_NAMED("Sliding box octrees");
	// TODO 调查多八叉树是否会在地形中产生裂缝（到目前为止我还没有注意到）

	const unsigned int lod_count = data.get_lod_count();
	const unsigned int mesh_block_size_po2 = settings.mesh_block_size_po2;
	const unsigned int octree_size_po2 = LodOctree::get_octree_size_po2(mesh_block_size_po2, lod_count);
	const unsigned int octree_size = 1 << octree_size_po2;
	const unsigned int octree_region_extent = 1 + settings.view_distance_voxels / (1 << octree_size_po2);

	const Vector3i viewer_octree_pos =
			(math::floor_to_int(p_viewer_pos) + Vector3iUtil::create(octree_size / 2)) >> octree_size_po2;

	const Box3i bounds_in_octrees = data.get_bounds().downscaled(octree_size);

	const Box3i new_box = Box3i::from_center_extents(viewer_octree_pos, Vector3iUtil::create(octree_region_extent))
								  .clipped(bounds_in_octrees);
	const Box3i prev_box = state.octree_streaming.last_octree_region_box;

	if (new_box != prev_box) {
		struct CleanOctreeAction {
			VoxelLodTerrainUpdateData::State &state;
			Vector3i block_offset_lod0;

			void operator()(Vector3i node_pos, unsigned int lod_index) {
				VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

				Vector3i bpos = node_pos + (block_offset_lod0 >> lod_index);

				auto block_it = lod.mesh_map_state.map.find(bpos);
				if (block_it != lod.mesh_map_state.map.end()) {
					lod.mesh_blocks_to_deactivate_visuals.push_back(bpos);
					lod.mesh_blocks_to_deactivate_collision.push_back(bpos);
					block_it->second.visual_active = false;
					block_it->second.collision_active = false;
				}
			}
		};

		struct ExitAction {
			VoxelLodTerrainUpdateData::State &state;
			unsigned int lod_count;

			void operator()(const Vector3i &pos) {
				StdMap<Vector3i, VoxelLodTerrainUpdateData::OctreeItem>::iterator it =
						state.octree_streaming.lod_octrees.find(pos);
				if (it == state.octree_streaming.lod_octrees.end()) {
					return;
				}

				VoxelLodTerrainUpdateData::OctreeItem &item = it->second;
				const Vector3i block_pos_maxlod = it->first;

				const unsigned int last_lod_index = lod_count - 1;

				// 我们直接丢弃八叉树，并隐藏它之前视为可见的数据块。
				// 通常这种八叉树不会太深，因为它们很可能位于已加载区域
				// 的边缘，除非玩家传送了很远的距离。
				CleanOctreeAction a{ state, block_pos_maxlod << last_lod_index };
				item.octree.clear(a);

				state.octree_streaming.lod_octrees.erase(it);

				// 从这里卸载最后一个 LOD，因为它可能比其它 LOD 延伸得更远一些。
				// 其它 LOD 通过滑动区域更早卸载。
				VoxelLodTerrainUpdateData::Lod &last_lod = state.lods[last_lod_index];
				last_lod.mesh_map_state.map.erase(pos);
				last_lod.mesh_blocks_to_unload.push_back(pos);
			}
		};

		struct EnterAction {
			VoxelLodTerrainUpdateData::State &state;
			unsigned int lod_count;

			void operator()(const Vector3i &pos) {
				// 这是我们正要进入的新单元，那里不应有任何东西
				CRASH_COND(state.octree_streaming.lod_octrees.find(pos) != state.octree_streaming.lod_octrees.end());

				// 创建新的八叉树
				// TODO 使用 ObjectPool 存储它们，删除成本不低
				std::pair<StdMap<Vector3i, VoxelLodTerrainUpdateData::OctreeItem>::iterator, bool> p =
						state.octree_streaming.lod_octrees.insert({ pos, VoxelLodTerrainUpdateData::OctreeItem() });
				CRASH_COND(p.second == false);
				VoxelLodTerrainUpdateData::OctreeItem &item = p.first->second;
				LodOctree::NoDestroyAction nda;
				item.octree.create(lod_count, nda);
			}
		};

		ExitAction exit_action{ state, lod_count };
		EnterAction enter_action{ state, lod_count };
		{
			VOXEL_PROFILE_SCOPE_NAMED("Unload octrees");

			const unsigned int last_lod_index = lod_count - 1;
			VoxelLodTerrainUpdateData::Lod &last_lod = state.lods[last_lod_index];
			RWLockWrite wlock(last_lod.mesh_map_state.map_lock);

			prev_box.difference(new_box, [exit_action](Box3i out_of_range_box) { //
				out_of_range_box.for_each_cell(exit_action);
			});
		}
		{
			VOXEL_PROFILE_SCOPE_NAMED("Load octrees");
			new_box.difference(prev_box, [enter_action](Box3i box_to_load) { //
				box_to_load.for_each_cell(enter_action);
			});
		}

		state.octree_streaming.force_update_octrees_next_update = true;
	}

	state.octree_streaming.last_octree_region_box = new_box;
}

#ifdef DEBUG_ENABLED
inline bool check_block_sizes(int data_block_size, int mesh_block_size) {
	return (data_block_size == 16 || data_block_size == 32) && (mesh_block_size == 16 || mesh_block_size == 32) &&
			mesh_block_size >= data_block_size;
}
#endif

bool add_loading_block(VoxelLodTerrainUpdateData::Lod &lod, Vector3i position) {
	auto it = lod.loading_blocks.find(position);

	if (it == lod.loading_blocks.end()) {
		// 第一个请求它的观察者
		VoxelLodTerrainUpdateData::LoadingDataBlock new_loading_block;
		new_loading_block.viewers.add();

		lod.loading_blocks.insert({ position, new_loading_block });

		return true;
	}
	// TODO 当前的八叉树逻辑无法可靠地只增加一次引用计数
	// 	it->second.viewers.add();
	return false;
}

bool check_block_mesh_updated(
		VoxelLodTerrainUpdateData::State &state,
		const VoxelData &data,
		VoxelLodTerrainUpdateData::MeshBlockState &mesh_block,
		Vector3i mesh_block_pos,
		uint8_t lod_index,
		StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &blocks_to_load,
		const VoxelLodTerrainUpdateData::Settings &settings
) {
	// VOXEL_PROFILE_SCOPE();

	VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

	const VoxelLodTerrainUpdateData::MeshState mesh_state = mesh_block.state;

	switch (mesh_state) {
		case VoxelLodTerrainUpdateData::MESH_NEVER_UPDATED:
		case VoxelLodTerrainUpdateData::MESH_NEED_UPDATE: {
			bool surrounded = true;
			if (data.is_streaming_enabled()) {
				const int mesh_block_size = 1 << settings.mesh_block_size_po2;
				const int data_block_size = data.get_block_size();
#ifdef DEBUG_ENABLED
				ERR_FAIL_COND_V(!check_block_sizes(data_block_size, mesh_block_size), false);
#endif
				// TODO 为什么我们只检查相邻数据块？
				// 从 `check_block_loaded_and_meshed` 调用时这也显得冗余

				// 查找数据块相邻位置
				const int factor = mesh_block_size / data_block_size;
				const Vector3i data_block_pos0 = factor * mesh_block_pos;
				const Box3i data_box(
						data_block_pos0 - Vector3i(1, 1, 1), Vector3iUtil::create(factor) + Vector3i(2, 2, 2)
				);
				const Box3i bounds = data.get_bounds().downscaled(data_block_size);
				// 56 是当网格数据块大小为 32 时，用这种方式能收集到的最大位置数量。
				FixedArray<Vector3i, 56> neighbor_positions;
				unsigned int neighbor_positions_count = 0;
				data_box.for_inner_outline([bounds, &neighbor_positions, &neighbor_positions_count](Vector3i pos) {
					if (bounds.contains(pos)) {
						neighbor_positions[neighbor_positions_count] = pos;
						++neighbor_positions_count;
					}
				});

				static thread_local StdVector<Vector3i> tls_missing;
				tls_missing.clear();

				// 检查相邻数据块是否已加载
				data.get_missing_blocks(to_span(neighbor_positions, neighbor_positions_count), lod_index, tls_missing);

				surrounded = tls_missing.size() == 0;

				// 安排加载缺失的相邻数据块
				MutexLock lock(lod.loading_blocks_mutex);
				for (const Vector3i &missing_pos : tls_missing) {
					if (add_loading_block(lod, missing_pos)) {
						blocks_to_load.push_back(
								VoxelLodTerrainUpdateData::BlockToLoad{
										VoxelLodTerrainUpdateData::BlockLocation{ missing_pos, lod_index },
										TaskCancellationToken() }
						);
					}
				}
			}

			if (surrounded) {
				lod.mesh_blocks_pending_update.push_back(
						VoxelLodTerrainUpdateData::MeshToUpdate{ mesh_block_pos, TaskCancellationToken(), true }
				);
				mesh_block.state = VoxelLodTerrainUpdateData::MESH_UPDATE_NOT_SENT;
			}

			return false;
		}

		case VoxelLodTerrainUpdateData::MESH_UPDATE_NOT_SENT:
		case VoxelLodTerrainUpdateData::MESH_UPDATE_SENT:
			return false;

		case VoxelLodTerrainUpdateData::MESH_UP_TO_DATE:
			return true;

		default:
			CRASH_NOW();
			break;
	}

	return true;
}

VoxelLodTerrainUpdateData::MeshBlockState &insert_new(
		StdUnorderedMap<Vector3i, VoxelLodTerrainUpdateData::MeshBlockState> &mesh_map,
		Vector3i pos
) {
#ifdef DEBUG_ENABLED
	// 我们能到这里是因为映射不包含该元素。如果已经包含，那是一个 bug。
	static VoxelLodTerrainUpdateData::MeshBlockState s_default;
	ERR_FAIL_COND_V(mesh_map.find(pos) != mesh_map.end(), s_default);
#endif
	// C++ 标准规定，若元素不存在，它将被默认构造。
	// 因此这里是向 unordered_map 插入默认、不可移动结构体的方法。
	// https://stackoverflow.com/questions/22229773/map-unordered-map-with-non-movable-default-constructible-value-type
	VoxelLodTerrainUpdateData::MeshBlockState &block = mesh_map[pos];

	// 这种方法无法编译，不得不改用写操作的 [] 运算符绕过。
	/*
	auto p = lod.mesh_map_state.map.emplace(pos, VoxelLodTerrainUpdateData::MeshBlockState());
	// 我们到这里是因为映射中不包含该元素。如果已经包含，那就是一个 bug。
	CRASH_COND(p.second == false);
	*/

	return block;
}

bool check_block_loaded_and_meshed(
		VoxelLodTerrainUpdateData::State &state,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const VoxelData &data,
		const Vector3i &p_mesh_block_pos,
		uint8_t lod_index,
		StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &blocks_to_load
) {
	//

	if (data.is_streaming_enabled()) {
		const int mesh_block_size = 1 << settings.mesh_block_size_po2;
		const int data_block_size = data.get_block_size();

#ifdef DEBUG_ENABLED
		ERR_FAIL_COND_V(!check_block_sizes(data_block_size, mesh_block_size), false);
#endif
		// 我们想知道与此网格数据块相交的数据的一切信息。
		// 流式加载时无法预先得知，必须请求。
		// 不流式加载时，`block == null` 等同于 `!block->has_voxels()`，因此我们不需要进入这里。

		static thread_local StdVector<Vector3i> tls_missing;
		tls_missing.clear();

		const int factor = mesh_block_size / data_block_size;
		const Box3i data_blocks_box = Box3i(p_mesh_block_pos * factor, Vector3iUtil::create(factor)).padded(1);

		data.get_missing_blocks(data_blocks_box, lod_index, tls_missing);

		// TODO 八叉树逻辑：为已加载的数据块增加引用计数？
		// 当数据块已加载时，我们如何知道这真的是第一次检查该区域？
		// 我们可能会多次进入这里，一些数据块会逐渐加载直到全部加载完成。
		// 但这意味着如果我们使用 `view_area`，会不必要地增加额外引用计数。
		// 也许可以给八叉树节点添加状态，以检查是否是第一次请求子节点？
		// 另一个选择是用滑动盒子逻辑来完成，它更容易理解。

		if (tls_missing.size() > 0) {
			VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
			MutexLock mlock(lod.loading_blocks_mutex);
			for (const Vector3i &missing_bpos : tls_missing) {
				if (add_loading_block(lod, missing_bpos)) {
					blocks_to_load.push_back(
							VoxelLodTerrainUpdateData::BlockToLoad{
									VoxelLodTerrainUpdateData::BlockLocation{ missing_bpos, lod_index },
									TaskCancellationToken() }
					);
				}
			}
			return false;
		}
	}

	VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

	VoxelLodTerrainUpdateData::MeshBlockState *mesh_block = nullptr;
	auto mesh_block_it = lod.mesh_map_state.map.find(p_mesh_block_pos);
	if (mesh_block_it == lod.mesh_map_state.map.end()) {
		// 若这成为与主线程 `apply_mesh_update` 的竞争来源，
		// 我们可以将添加操作推迟到八叉树适配结束时。
		RWLockWrite wlock(lod.mesh_map_state.map_lock);
		mesh_block = &insert_new(lod.mesh_map_state.map, p_mesh_block_pos);
		mesh_block->mesh_viewers.add();
		mesh_block->collision_viewers.add();
	} else {
		mesh_block = &mesh_block_it->second;
	}

	return check_block_mesh_updated(state, data, *mesh_block, p_mesh_block_pos, lod_index, blocks_to_load, settings);
}

void process_octrees_fitting(
		VoxelLodTerrainUpdateData::State &state,
		const VoxelLodTerrainUpdateData::Settings &settings,
		VoxelData &data,
		Vector3 p_viewer_pos,
		StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &data_blocks_to_load
) {
	//
	VOXEL_PROFILE_SCOPE();

	const int mesh_block_size = 1 << settings.mesh_block_size_po2;
	const int octree_leaf_node_size = mesh_block_size;
	const unsigned int lod_count = data.get_lod_count();

	const bool force_update_octrees = state.octree_streaming.force_update_octrees_next_update;
	state.octree_streaming.force_update_octrees_next_update = false;

	// 在某些条件下，八叉树可能不需要每帧更新
	if (!state.octree_streaming.had_blocked_octree_nodes_previous_update && !force_update_octrees &&
		p_viewer_pos.distance_squared_to(Vector3(state.octree_streaming.local_viewer_pos_previous_octree_update)) <
				math::squared(octree_leaf_node_size / 2)) {
		return;
	}

	state.octree_streaming.local_viewer_pos_previous_octree_update = p_viewer_pos;

	const float lod_distance_octree_space = settings.lod_distance / octree_leaf_node_size;

	unsigned int blocked_octree_nodes = 0;

	// 偏移一位：第二位是 LOD0，第一位未使用
	uint32_t lods_to_update_transitions = 0;

	// TODO 优化：维护一个向量以加快迭代？
	for (auto octree_it = state.octree_streaming.lod_octrees.begin();
		 octree_it != state.octree_streaming.lod_octrees.end();
		 ++octree_it) {
		VOXEL_PROFILE_SCOPE();

		struct OctreeActions {
			VoxelLodTerrainUpdateData::State &state;
			const VoxelLodTerrainUpdateData::Settings &settings;
			VoxelData &data;
			StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &data_blocks_to_load;
			Vector3i block_offset_lod0;
			unsigned int blocked_count = 0;
			float lod_distance_octree_space;
			Vector3 viewer_pos_octree_space;
			uint32_t &lods_to_update_transitions;

			void create_child(Vector3i node_pos, int lod_index, LodOctree::NodeData &node_data) {
				VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
				const Vector3i bpos = node_pos + (block_offset_lod0 >> lod_index);
				auto mesh_block_it = lod.mesh_map_state.map.find(bpos);

				// 绝不显示尚未网格化的子节点，若我们到达这里，那将是一个 bug
				CRASH_COND(mesh_block_it == lod.mesh_map_state.map.end());
				CRASH_COND(mesh_block_it->second.state != VoxelLodTerrainUpdateData::MESH_UP_TO_DATE);

				// self->set_mesh_block_active(*block, true);
				lod.mesh_blocks_to_activate_visuals.push_back(bpos);
				lod.mesh_blocks_to_activate_collision.push_back(bpos);
				mesh_block_it->second.visual_active = true;
				mesh_block_it->second.collision_active = true;
				lods_to_update_transitions |= (0b111 << lod_index);
			}

			void destroy_child(Vector3i node_pos, int lod_index) {
				VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
				const Vector3i bpos = node_pos + (block_offset_lod0 >> lod_index);
				auto mesh_block_it = lod.mesh_map_state.map.find(bpos);

				if (mesh_block_it != lod.mesh_map_state.map.end()) {
					// self->set_mesh_block_active(*block, false);
					mesh_block_it->second.visual_active = false;
					mesh_block_it->second.collision_active = false;
					lod.mesh_blocks_to_deactivate_visuals.push_back(bpos);
					lod.mesh_blocks_to_deactivate_collision.push_back(bpos);
					lods_to_update_transitions |= (0b111 << lod_index);
				}
			}

			void show_parent(Vector3i node_pos, int lod_index) {
				VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
				Vector3i bpos = node_pos + (block_offset_lod0 >> lod_index);
				auto mesh_block_it = lod.mesh_map_state.map.find(bpos);

				// 若我们传送到远处，我们之前所在的区域将合并，
				// 数据块可能已被完全卸载。
				// 因此在这种情况下找不到任何数据块是正常的。
				// 否则，最终必须始终有一个可见的父节点，除非八叉树消失了。
				if (mesh_block_it != lod.mesh_map_state.map.end() &&
					mesh_block_it->second.state == VoxelLodTerrainUpdateData::MESH_UP_TO_DATE) {
					// self->set_mesh_block_active(*block, true);
					mesh_block_it->second.visual_active = true;
					mesh_block_it->second.collision_active = true;
					lod.mesh_blocks_to_activate_visuals.push_back(bpos);
					lod.mesh_blocks_to_activate_collision.push_back(bpos);
					lods_to_update_transitions |= (0b111 << lod_index);
				}
			}

			void hide_parent(Vector3i node_pos, int lod_index) {
				destroy_child(node_pos, lod_index); // 相同
			}

			bool can_create_root(int lod_index) {
				const Vector3i offset = block_offset_lod0 >> lod_index;
				const bool can =
						check_block_loaded_and_meshed(state, settings, data, offset, lod_index, data_blocks_to_load);
				if (!can) {
					++blocked_count;
				}
				return can;
			}

			bool can_split(Vector3i node_pos, int lod_index, LodOctree::NodeData &node_data) {
				VOXEL_PROFILE_SCOPE();
				if (!LodOctree::is_below_split_distance(
							node_pos, lod_index, viewer_pos_octree_space, lod_distance_octree_space
					)) {
					return false;
				}
				const int child_lod_index = lod_index - 1;
				const Vector3i offset = block_offset_lod0 >> child_lod_index;
				bool can = true;

				// 只有更高质量的网格准备好显示时才能细分，否则会产生空洞
				for (int i = 0; i < 8; ++i) {
					// 获取相对于区域的数据块位置 + 转换为相对于地形的本地坐标
					const Vector3i child_pos = LodOctree::get_child_position(node_pos, i) + offset;
					// 我们必须请求所有子节点，因为我们在这里的原因就是希望它们被加载
					can &= check_block_loaded_and_meshed(
							state, settings, data, child_pos, child_lod_index, data_blocks_to_load
					);
				}

				// 只有周围存在更高 LOD 索引的数据块时才能细分，
				// 否则会产生裂缝。
				// 需要检查网格而非体素？
				// const int lod_index = child_lod_index + 1;
				// if (lod_index < self->get_lod_count()) {
				// 	const Vector3i parent_offset = block_offset_lod0 >> lod_index;
				// 	const Lod &lod = self->_lods[lod_index];
				// 	can &= self->is_block_surrounded(node_pos + parent_offset, lod_index, lod.map);
				// }

				if (!can) {
					++blocked_count;
				}

				return can;
			}

			bool can_join(Vector3i node_pos, int parent_lod_index) {
				VOXEL_PROFILE_SCOPE();
				if (LodOctree::is_below_split_distance(
							node_pos, parent_lod_index, viewer_pos_octree_space, lod_distance_octree_space
					)) {
					return false;
				}
				// 只有父网格就绪时才能取消细分
				VoxelLodTerrainUpdateData::Lod &lod = state.lods[parent_lod_index];

				Vector3i bpos = node_pos + (block_offset_lod0 >> parent_lod_index);
				auto mesh_block_it = lod.mesh_map_state.map.find(bpos);

				if (mesh_block_it == lod.mesh_map_state.map.end()) {
					// 数据块已被卸载。例外地，我们可以合并。
					// 总会有一个祖父节点，因为当它们分裂时我们从不销毁它们，
					// 而且我们从不先创建子节点而不创建父节点。
					return true;
				}

				// 数据块已加载（？）但网格不是最新的，我们需要请求并等待。
				const bool can = check_block_mesh_updated(
						state, data, mesh_block_it->second, bpos, parent_lod_index, data_blocks_to_load, settings
				);

				if (!can) {
					++blocked_count;
				}

				return can;
			}
		};

		const Vector3i block_pos_maxlod = octree_it->first;
		const Vector3i block_offset_lod0 = block_pos_maxlod << (lod_count - 1);
		const Vector3 relative_viewer_pos = p_viewer_pos - Vector3(mesh_block_size * block_offset_lod0);

		OctreeActions octree_actions{ state,
									  settings,
									  data,
									  data_blocks_to_load,
									  block_offset_lod0,
									  0,
									  lod_distance_octree_space,
									  relative_viewer_pos / octree_leaf_node_size,
									  lods_to_update_transitions };
		VoxelLodTerrainUpdateData::OctreeItem &item = octree_it->second;
		item.octree.update(octree_actions);

		blocked_octree_nodes += octree_actions.blocked_count;
	}

	// 理想情况下，此统计值应稳定为零。
	// 若非如此，则数据块管理中的某些东西阻止了 LOD 正常显示，应予以修复。
	state.stats.blocked_lods = blocked_octree_nodes;
	state.octree_streaming.had_blocked_octree_nodes_previous_update = blocked_octree_nodes > 0;

	update_transition_masks(state, lods_to_update_transitions, lod_count, false);
}

} // namespace

void process_octree_streaming(
		VoxelLodTerrainUpdateData::State &state,
		VoxelData &data,
		Vector3 viewer_pos,
		StdVector<VoxelData::BlockToSave> *data_blocks_to_save,
		StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &data_blocks_to_load,
		const VoxelLodTerrainUpdateData::Settings &settings,
		bool stream_enabled
) {
	VOXEL_PROFILE_SCOPE();

	// 卸载超出数据块区域范围的已加载数据块。
	// 仅当数据流式加载启用时才卸载数据。否则数据始终加载。
	if (data.is_streaming_enabled()) {
		process_unload_data_blocks_sliding_box(state, data, viewer_pos, data_blocks_to_save, settings);
	}

	// 卸载超出网格数据块区域范围的已加载网格数据块
	process_unload_mesh_blocks_sliding_box(state, viewer_pos, settings, data);

	// 在观察者周围的网格中创建和移除八叉树。
	// 网格数据块驱动体素数据和视觉的加载。
	process_octrees_sliding_box(state, viewer_pos, settings, data);

	state.stats.blocked_lods = 0;

	// 在每个八叉树内找出我们需要加载和查看的数据块
	if (stream_enabled) {
		process_octrees_fitting(state, settings, data, viewer_pos, data_blocks_to_load);
	}
}

} // namespace voxel
