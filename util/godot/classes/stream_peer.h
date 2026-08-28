#ifndef VOXEL_GODOT_STREAM_PEER_H
#define VOXEL_GODOT_STREAM_PEER_H

#if defined(VOXEL_GODOT)
#include <core/io/stream_peer.h>
#endif

#include "../../containers/span.h"

namespace voxel::godot {

inline void stream_peer_put_data(StreamPeer &peer, Span<const uint8_t> src) {
#if defined(VOXEL_GODOT)
	peer.put_data(src.data(), src.size());
#endif
}

inline Error stream_peer_get_data(StreamPeer &peer, Span<uint8_t> dst) {
#if defined(VOXEL_GODOT)
	return peer.get_data(dst.data(), dst.size());
#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_STREAM_PEER_H
