#ifndef VOXEL_GODOT_GRAPH_NODE_H
#define VOXEL_GODOT_GRAPH_NODE_H

#if defined(VOXEL_GODOT)
#include <scene/gui/graph_node.h>
#endif

namespace voxel::godot {

Vector2 get_graph_node_input_port_position(GraphNode &node, int port_index);
Vector2 get_graph_node_output_port_position(GraphNode &node, int port_index);
Color get_graph_node_input_port_color(GraphNode &node, int port_index);

} // namespace voxel::godot

#endif // VOXEL_GODOT_GRAPH_NODE_H
