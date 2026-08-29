#include "graph_node.h"
#include "../core/version.h"

namespace voxel::godot {

// Godot PR #79311 2167694965ca2f4f16cfc1362d32a2fa01e817a2 中 GraphNode 有一些改动

// 出于某种原因，这些 getter 不能是 const……

Vector2 get_graph_node_input_port_position(GraphNode &node, int port_index) {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 1
	// 不能直接使用输入和输出位置……Godot 会预先缩放它们，导致在 NOTIFICATION_DRAW 内部无法使用，
	// 因为此时节点已经被缩放过
	const Vector2 scale = node.get_global_transform().get_scale();
	return node.get_connection_input_position(port_index) / scale;
#else
	return node.get_input_port_position(port_index);
#endif
}

Vector2 get_graph_node_output_port_position(GraphNode &node, int port_index) {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 1
	const Vector2 scale = node.get_global_transform().get_scale();
	return node.get_connection_output_position(port_index) / scale;
#else
	return node.get_output_port_position(port_index);
#endif
}

Color get_graph_node_input_port_color(GraphNode &node, int port_index) {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 1
	return node.get_connection_input_color(port_index);
#else
	return node.get_input_port_color(port_index);
#endif
}

} // namespace voxel::godot
