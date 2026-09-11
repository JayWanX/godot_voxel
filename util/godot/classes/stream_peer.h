#ifndef VOXEL_GODOT_STREAM_PEER_H
#define VOXEL_GODOT_STREAM_PEER_H

#include <core/io/stream_peer.h>

#include "../../containers/span.h"

namespace voxel::godot {

inline void stream_peer_put_data(StreamPeer &peer, Span<const uint8_t> src) {
	peer.put_data(src.data(), src.size());
}

inline Error stream_peer_get_data(StreamPeer &peer, Span<uint8_t> dst) {
	return peer.get_data(dst.data(), dst.size());
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_STREAM_PEER_H
