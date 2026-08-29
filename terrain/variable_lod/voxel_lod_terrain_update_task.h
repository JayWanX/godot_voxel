#ifndef VOXEL_LOD_TERRAIN_UPDATE_TASK_H
#define VOXEL_LOD_TERRAIN_UPDATE_TASK_H

#include "../../engine/ids.h"
#include "../../engine/priority_dependency.h"
#include "../../storage/voxel_data.h"
#include "../../util/containers/std_vector.h"
#include "../../util/tasks/threaded_task.h"
#include "voxel_lod_terrain_update_data.h"

namespace voxel {

struct StreamingDependency;
struct MeshingDependency;
class BufferedTaskScheduler;

// 运行 VoxelLodTerrain 更新循环的一部分。
// 这部分可以在另一个线程上运行，因此多个地形可以并行更新。
// 每个地形同一时间只能有一个在运行。
// 注意，此任务不包含网格化和体素生成。这些由不同的任务完成。
//
// 重要提示：此任务所做的工作不得直接或间接涉及对 Godot 服务器的任何调用。
// 这些都被延迟到主线程执行。
//
class VoxelLodTerrainUpdateTask : public IThreadedTask {
public:
	VoxelLodTerrainUpdateTask( 
			std::shared_ptr<VoxelData> p_data, 
			std::shared_ptr<VoxelLodTerrainUpdateData> p_update_data, 
			std::shared_ptr<StreamingDependency> p_streaming_dependency, 
			std::shared_ptr<MeshingDependency> p_meshing_dependency, 
			std::shared_ptr<PriorityDependency::ViewersData> p_shared_viewers_data, 
			const Vector3 p_viewer_pos, 
			const VolumeID p_volume_id, 
			const Transform3D p_volume_transform 
			) :
			_data(p_data),
			_update_data(p_update_data),
			_streaming_dependency(p_streaming_dependency),
			_meshing_dependency(p_meshing_dependency),
			_shared_viewers_data(p_shared_viewers_data),
			_viewer_pos(p_viewer_pos),
			_volume_id(p_volume_id),
			_volume_transform(p_volume_transform) {}

	const char *get_debug_name() const override {
		return "VoxelLodTerrainUpdate";
	}

	void run(ThreadedTaskContext &ctx) override;

	// 也可在此任务之外使用的函数

	static void flush_pending_lod_edits(
			VoxelLodTerrainUpdateData::State &state,
			VoxelData &data,
			const int mesh_block_size
	);

	static uint8_t get_transition_mask(
			const VoxelLodTerrainUpdateData::State &state,
			const Vector3i block_pos,
			unsigned int lod_index,
			unsigned int lod_count
	);

	// 用于已加载的数据块
	static inline void schedule_mesh_update(
			VoxelLodTerrainUpdateData::MeshBlockState &block,
			const Vector3i bpos,
			StdVector<VoxelLodTerrainUpdateData::MeshToUpdate> &blocks_pending_update,
			const bool require_visual
	) {
		if (block.state != VoxelLodTerrainUpdateData::MESH_UPDATE_NOT_SENT) {
			if (block.visual_active || block.collision_active) {
				// 安排一次更新
				block.state = VoxelLodTerrainUpdateData::MESH_UPDATE_NOT_SENT;
				block.update_list_index = blocks_pending_update.size();
				blocks_pending_update.push_back(
						VoxelLodTerrainUpdateData::MeshToUpdate{ bpos, TaskCancellationToken(), require_visual }
				);
			} else {
				// 仅将其标记为需要更新，以便可见性系统在需要时安排其更新。
				block.state = VoxelLodTerrainUpdateData::MESH_NEED_UPDATE;
			}
		}
	}

	static void send_block_save_requests(
			const VolumeID volume_id,
			const Span<VoxelData::BlockToSave> blocks_to_save,
			const std::shared_ptr<StreamingDependency> &stream_dependency,
			BufferedTaskScheduler &task_scheduler,
			const std::shared_ptr<AsyncDependencyTracker> tracker,
			const bool with_flush
	);

private:
	std::shared_ptr<VoxelData> _data;
	std::shared_ptr<VoxelLodTerrainUpdateData> _update_data;
	std::shared_ptr<StreamingDependency> _streaming_dependency;
	std::shared_ptr<MeshingDependency> _meshing_dependency;
	std::shared_ptr<PriorityDependency::ViewersData> _shared_viewers_data;
	Vector3 _viewer_pos;
	VolumeID _volume_id;
	Transform3D _volume_transform;
};

void update_transition_masks(
		VoxelLodTerrainUpdateData::State &state,
		const uint32_t lods_to_update_transitions,
		const unsigned int lod_count,
		const bool use_refcounts
);

void add_unloaded_saving_blocks(VoxelLodTerrainUpdateData::Lod &lod, Span<const VoxelData::BlockToSave> src);

} // namespace voxel

#endif // VOXEL_LOD_TERRAIN_UPDATE_TASK_H
