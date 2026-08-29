#ifndef VOXEL_LOD_TERRAIN_UPDATE_OCTREE_STREAMING_H
#define VOXEL_LOD_TERRAIN_UPDATE_OCTREE_STREAMING_H

#include "../../storage/voxel_data.h"
#include "voxel_lod_terrain_update_data.h"

namespace voxel {

// 限制：
// - 仅支持一个观察者
// - 若实际没有观察者，仍假定世界原点处存在一个观察者
// - 不支持观察者标志（单独的碰撞/视觉/体素需求）

void process_octree_streaming(
		VoxelLodTerrainUpdateData::State &state,
		VoxelData &data,
		Vector3 viewer_pos,
		StdVector<VoxelData::BlockToSave> *data_blocks_to_save,
		StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &data_blocks_to_load,
		const VoxelLodTerrainUpdateData::Settings &settings,
		bool stream_enabled
);

} // namespace voxel

#endif // VOXEL_LOD_TERRAIN_UPDATE_OCTREE_STREAMING_H
