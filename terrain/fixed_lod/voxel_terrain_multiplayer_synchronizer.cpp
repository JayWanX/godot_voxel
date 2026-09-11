#include "voxel_terrain_multiplayer_synchronizer.h"
#include "../../constants/voxel_string_names.h"
#include "../../storage/voxel_buffer.h"
#include "../../streams/voxel_block_serializer.h"
#include "../../util/containers/container_funcs.h"
#include <scene/main/multiplayer_api.h>
#include <scene/main/multiplayer_peer.h>
#include <scene/main/scene_tree.h>
#include <core/variant/array.h>
#include "../../util/io/serialization.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"
#include "voxel_terrain.h"
#include <scene/main/multiplayer_api.h>
#include <scene/main/multiplayer_peer.h>
#include <scene/main/scene_tree.h>
#include <core/variant/array.h>

#ifdef TOOLS_ENABLED
#include "../../util/godot/core/packed_arrays.h"
#include "../../util/godot/core/string.h"
#endif

namespace voxel {

VoxelTerrainMultiplayerSynchronizer::VoxelTerrainMultiplayerSynchronizer() {
	Dictionary config;
	config["rpc_mode"] = MultiplayerAPI::RPC_MODE_AUTHORITY;
	config["transfer_mode"] = MultiplayerPeer::TRANSFER_MODE_RELIABLE;
	config["call_local"] = false;
	config["channel"] = _rpc_channel;

	rpc_config(VoxelStringNames::get_singleton()._rpc_receive_blocks, config);
	rpc_config(VoxelStringNames::get_singleton()._rpc_receive_area, config);

	set_process(true);
}

// 辅助函数
bool VoxelTerrainMultiplayerSynchronizer::is_server() const {
	VOXEL_ASSERT_RETURN_V(is_inside_tree(), false);
	// 注意：当场景树中有多个 Multiplayer 分支时，`get_multiplayer` 可能会明显变慢，
	// 因为它需要构造一个 NodePath，然后查询一个 HashMap<NodePah,V> 来检查哪些节点具有
	// 自定义的多人游戏设置。在 Godot 考虑到的场景中这不是大问题，但在我们的场景中，
	// 这可能会产生显著影响，因为当观察者加入或传送时，`is_server()` 在一帧内可能会被调用上千次。
	Ref<MultiplayerAPI> mp = get_multiplayer();
	VOXEL_ASSERT_RETURN_V(mp.is_valid(), false);
	return mp->is_server();
}

void VoxelTerrainMultiplayerSynchronizer::send_block(
		int viewer_peer_id,
		const VoxelDataBlock &data_block,
		Vector3i bpos
) {
	VOXEL_PROFILE_SCOPE();

	BlockSerializer::SerializeResult result =
			BlockSerializer::serialize_and_compress(data_block.get_voxels_const(), CompressedData::COMPRESSION_LZ4);
	VOXEL_ASSERT_RETURN(result.success);

	PackedByteArray message_data;
	message_data.resize(4 * sizeof(int16_t) + result.data.size());

	ByteSpanWithPosition mw_span(Span<uint8_t>(message_data.ptrw(), message_data.size()), 0);
	MemoryWriterExistingBuffer mw(mw_span, ENDIANNESS_LITTLE_ENDIAN);

	mw.store_16(bpos.x);
	mw.store_16(bpos.y);
	mw.store_16(bpos.z);
	VOXEL_ASSERT_RETURN(result.data.size() <= 65535);
	mw.store_16(result.data.size());
	mw.store_buffer(to_span(result.data));

	// print_line(String("Server: send block {0}").format(varray(bpos)));

	// rpc_id(viewer_peer_id, VoxelStringNames::get_singleton().receive_block, data);
	// 与其立即发送，不如推迟到地形处理完成后再发送。用 RPC 系统逐个发送数据块太慢了。
	_deferred_block_messages_per_peer[viewer_peer_id].push_back(DeferredBlockMessage{ message_data });
}

// TODO 是否有办法实现"幽灵编辑"（ghost edits）？
// 如果有人想要通过快速连续编辑让客户端看起来流畅，这会对网络性能产生严重影响。
// 因此也许服务器需要将相近的编辑聚簇，并批量发送整个区域。
// 反之，客户端需要在本地应用编辑，同时如果服务器在一段时间内没有确认，需要有办法回滚。

void VoxelTerrainMultiplayerSynchronizer::send_area(Box3i voxel_box) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(_terrain != nullptr);

	StdVector<ViewerID> viewers;
	_terrain->get_viewers_in_area(viewers, voxel_box);

	// 对单个体素编辑来说效率不高，但面对更大的盒子应该能较好地扩展
	VoxelBuffer voxels(VoxelBuffer::ALLOCATOR_POOL);
	voxels.create(voxel_box.size);
	_terrain->get_storage().copy(voxel_box.position, voxels, 0xff, true);

	BlockSerializer::SerializeResult result =
			BlockSerializer::serialize_and_compress(voxels, CompressedData::COMPRESSION_LZ4);
	VOXEL_ASSERT_RETURN(result.success);

	PackedByteArray pba;
	pba.resize(4 * sizeof(int32_t) + result.data.size());

	ByteSpanWithPosition mw_span(Span<uint8_t>(pba.ptrw(), pba.size()), 0);
	MemoryWriterExistingBuffer mw(mw_span, ENDIANNESS_LITTLE_ENDIAN);

	mw.store_32(voxel_box.position.x);
	mw.store_32(voxel_box.position.y);
	mw.store_32(voxel_box.position.z);
	mw.store_32(result.data.size());
	mw.store_buffer(to_span(result.data));
	for (const ViewerID viewer_id : viewers) {
		const int peer_id = VoxelEngine::get_singleton().get_viewer_network_peer_id(viewer_id);
		// TODO 如果周围没有联网的观察者，是否不必费心复制和序列化？
		if (peer_id != -1 && peer_id != MultiplayerPeer::TARGET_PEER_SERVER) {
			rpc_id(peer_id, VoxelStringNames::get_singleton()._rpc_receive_area, pba);
		}
	}
}

void VoxelTerrainMultiplayerSynchronizer::_notification(int p_what) {
	if (p_what == NOTIFICATION_PARENTED) {
		VoxelTerrain *terrain = Object::cast_to<VoxelTerrain>(get_parent());
		if (terrain != nullptr && terrain->get_multiplayer_synchronizer() == nullptr) {
			terrain->set_multiplayer_synchronizer(this);
			_terrain = terrain;
		}

	} else if (p_what == NOTIFICATION_UNPARENTED) {
		if (_terrain != nullptr && _terrain->get_multiplayer_synchronizer() == this) {
			_terrain->set_multiplayer_synchronizer(nullptr);
		}
		_terrain = nullptr;

	} else if (p_what == NOTIFICATION_PROCESS) {
		process();
	}
}

// template <typename T, typename F>
// inline void for_chunks(const StdVector<T> &vec, unsigned int chunk_size, F f) {
// 	for (unsigned int i = 0; i < vec.size(); i += chunk_size) {
// 		f(to_span_from_position_and_size(vec, i, i + chunk_size > vec.size() ? vec.size() - i : chunk_size));
// 	}
// }

void VoxelTerrainMultiplayerSynchronizer::process() {
	VOXEL_PROFILE_SCOPE();

	for (auto it = _deferred_block_messages_per_peer.begin(); it != _deferred_block_messages_per_peer.end(); ++it) {
		StdVector<DeferredBlockMessage> &messages = it->second;

		if (messages.size() == 0) {
			continue;
		}

		PackedByteArray pba;
		// 每帧、每个对端（peer）生成一条大而粗的消息，因为用 Godot 的 ENet 多人游戏发送大量小消息
		// 超级慢。它在每次 RPC 时都会调用 flush()，这很耗时，而且高层特性还会带来额外开销……

		unsigned int size = 0;
		for (const DeferredBlockMessage &message : messages) {
			size += message.data.size();
		}

		pba.resize(1 * sizeof(uint32_t) + size);

		ByteSpanWithPosition mw_span(Span<uint8_t>(pba.ptrw(), pba.size()), 0);
		MemoryWriterExistingBuffer mw(mw_span, ENDIANNESS_LITTLE_ENDIAN);
		mw.store_32(messages.size());

		for (const DeferredBlockMessage &message : messages) {
			mw.store_buffer(Span<const uint8_t>(message.data.ptr(), message.data.size()));
		}
		VOXEL_ASSERT(mw.data.size() == mw.data.pos);

		messages.clear();

		const int peer_id = it->first;
		VOXEL_PRINT_VERBOSE(format("Sending {} bytes of block data to peer {}", pba.size(), peer_id));
		// print_data_hex(Span<const uint8_t>(pba.ptr(), pba.size()));
		rpc_id(peer_id, VoxelStringNames::get_singleton()._rpc_receive_blocks, pba);
	}
}

void VoxelTerrainMultiplayerSynchronizer::_b_receive_blocks(PackedByteArray message_data) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(_terrain != nullptr);

	// print_line(String("Client: receive blocks data {1}").format(varray(data.size())));
	//  print_data_hex(Span<const uint8_t>(data.ptr(), data.size()));

	MemoryReader mr(Span<const uint8_t>(message_data.ptr(), message_data.size()), ENDIANNESS_LITTLE_ENDIAN);

	const unsigned int block_count = mr.get_32();

	for (unsigned int i = 0; i < block_count; ++i) {
		Vector3i bpos;
		// 这实际上将体积大小限制为 1,048,576。如果确实需要，我们可以将此数据加倍以覆盖
		// 更多范围。
		bpos.x = int16_t(mr.get_16());
		bpos.y = int16_t(mr.get_16());
		bpos.z = int16_t(mr.get_16());
		const int voxel_data_size = mr.get_16();
		// print_line(String("Client: receive block {0} data {1}").format(varray(bpos, voxel_data_size)));

		VoxelBuffer voxels(VoxelBuffer::ALLOCATOR_POOL);
		VOXEL_ASSERT_RETURN(BlockSerializer::decompress_and_deserialize(mr.data.sub(mr.pos, voxel_data_size), voxels));

		mr.pos += voxel_data_size;

		std::shared_ptr<VoxelBuffer> voxels_p = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
		*voxels_p = std::move(voxels);

		VOXEL_ASSERT_RETURN(_terrain != nullptr);
		_terrain->try_set_block_data(bpos, voxels_p);
	}
}

void VoxelTerrainMultiplayerSynchronizer::_b_receive_area(PackedByteArray message_data) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(_terrain != nullptr);

	MemoryReader mr(Span<const uint8_t>(message_data.ptr(), message_data.size()), ENDIANNESS_LITTLE_ENDIAN);

	Vector3i pos;
	pos.x = int32_t(mr.get_32());
	pos.y = int32_t(mr.get_32());
	pos.z = int32_t(mr.get_32());
	const int voxel_data_size = mr.get_32();

	VoxelBuffer voxels(VoxelBuffer::ALLOCATOR_POOL);
	VOXEL_ASSERT_RETURN(BlockSerializer::decompress_and_deserialize(mr.data.sub(mr.pos, voxel_data_size), voxels));

	_terrain->get_storage().paste(pos, voxels, 0xff, false, true);
	_terrain->post_edit_area(
			Box3i(pos, voxels.get_size()),
			// 暂时先不区分，无论如何都更新网格。如有必要，我们需要在消息中添加一个标志
			// 来告知它实际上并未改变体素（如果是元数据变化），但可能不值得
			true
	);
}

#ifdef TOOLS_ENABLED

PackedStringArray VoxelTerrainMultiplayerSynchronizer::get_configuration_warnings() const {
	PackedStringArray warnings;
	get_configuration_warnings(warnings);
	return warnings;
}

void VoxelTerrainMultiplayerSynchronizer::get_configuration_warnings(PackedStringArray &warnings) const {
	if (is_inside_tree()) {
		if (_terrain == nullptr) {
			warnings.append(VOXEL_TTR("This node must be child of {0}").format(varray(VoxelTerrain::get_class_static())));
		}

		const Node *parent_node = get_parent();

		if (parent_node != nullptr) {
			const VoxelTerrain *terrain = Object::cast_to<VoxelTerrain>(parent_node);
			if (terrain != nullptr && terrain->get_multiplayer_synchronizer() != this) {
				warnings.append(VOXEL_TTR("Only one instance of {0} should exist under a {1}")
										.format(
												varray(VoxelTerrainMultiplayerSynchronizer::get_class_static(),
													   VoxelTerrain::get_class_static())
										));
			}
		}
	}
}

#endif

void VoxelTerrainMultiplayerSynchronizer::_bind_methods() {
	// TODO 这些方法本不该被暴露。它们存在只是为了 Godot 的高层多人游戏能够找到
	// 它们。
	ClassDB::bind_method(
			D_METHOD("_rpc_receive_blocks", "data"), &VoxelTerrainMultiplayerSynchronizer::_b_receive_blocks
	);
	ClassDB::bind_method(D_METHOD("_rpc_receive_area", "data"), &VoxelTerrainMultiplayerSynchronizer::_b_receive_area);
}

} // namespace voxel
