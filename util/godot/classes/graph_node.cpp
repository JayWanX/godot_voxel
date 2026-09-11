#include "graph_node.h"
#include <core/version.h>

namespace voxel::godot {

// Godot PR #79311 2167694965ca2f4f16cfc1362d32a2fa01e817a2 中 GraphNode 有一些改动

// 出于某种原因，这些 getter 不能是 const……

Vector2 get_graph_node_input_port_position(GraphNode &node, int port_index) {
	return node.get_input_port_position(port_index);
}

Vector2 get_graph_node_output_port_position(GraphNode &node, int port_index) {
	return node.get_output_port_position(port_index);
}

Color get_graph_node_input_port_color(GraphNode &node, int port_index) {
	return node.get_input_port_color(port_index);
}

} // namespace voxel::godot
