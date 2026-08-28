#ifndef VOXEL_GODOT_GRAPH_EDIT_CONNECTION_H
#define VOXEL_GODOT_GRAPH_EDIT_CONNECTION_H

#include "../core/string_name.h"

namespace voxel::godot {

struct GraphEditConnection {
	StringName from;
	StringName to;
	int from_port = 0;
	int to_port = 0;
	// float activity = 0.0;

	inline bool is_valid() const {
		return !is_empty(from);
	}
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_GRAPH_EDIT_CONNECTION_H
