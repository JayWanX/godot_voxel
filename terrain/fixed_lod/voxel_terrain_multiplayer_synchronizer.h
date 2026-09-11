#include <core/version.h>
#ifndef VOXEL_NETWORK_TERRAIN_SYNC_H
#define VOXEL_NETWORK_TERRAIN_SYNC_H

#include "../../storage/voxel_data_block.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/node.h"
#include "../../util/math/box3i.h"

#ifdef TOOLS_ENABLED
#include <core/version.h>
#endif

namespace voxel {

class VoxelTerrain;

// 为 `VoxelTerrain` 实现多人在线复制
class VoxelTerrainMultiplayerSynchronizer : public Node {
	GDCLASS(VoxelTerrainMultiplayerSynchronizer, Node)
public:
	VoxelTerrainMultiplayerSynchronizer();

	// 当前实例是否为服务器
	bool is_server() const;

	// 向指定对等体发送数据块
	void send_block(int viewer_peer_id, const VoxelDataBlock &data_block, Vector3i bpos);
	// 向客户端发送指定区域的数据
	void send_area(Box3i voxel_box);

#ifdef TOOLS_ENABLED
	PackedStringArray get_configuration_warnings() const override;
	void get_configuration_warnings(PackedStringArray &warnings) const;
#endif

private:
	void _notification(int p_what);

	void process();

	void _b_receive_blocks(PackedByteArray message_data);
	void _b_receive_area(PackedByteArray message_data);

	static void _bind_methods();

	VoxelTerrain *_terrain = nullptr;
	int _rpc_channel = 0;

	struct DeferredBlockMessage {
		PackedByteArray data;
	};

	StdUnorderedMap<int, StdVector<DeferredBlockMessage>> _deferred_block_messages_per_peer;
};

} // namespace voxel

#endif // VOXEL_NETWORK_TERRAIN_SYNC_H
