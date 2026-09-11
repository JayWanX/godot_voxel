#include "graph_edit.h"

namespace voxel::godot {

void get_graph_edit_connections(const GraphEdit &self, StdVector<GraphEditConnection> &out_connections) {


	const Vector<Ref<GraphEdit::Connection>> &connections_list = self.get_connections();

	for (const Ref<GraphEdit::Connection> &src_con : connections_list) {
		GraphEditConnection dst_con;

		dst_con.from = src_con->from_node;
		dst_con.to = src_con->to_node;
		dst_con.from_port = src_con->from_port;
		dst_con.to_port = src_con->to_port;

		out_connections.push_back(dst_con);
	}

}

Vector2 get_graph_edit_scroll_offset(const GraphEdit &self) {
	return self.get_scroll_offset();
}

bool is_graph_edit_using_snapping(const GraphEdit &self) {
	return self.is_snapping_enabled();
}

int get_graph_edit_snapping_distance(const GraphEdit &self) {
	return self.get_snapping_distance();
}

GraphEditConnection get_graph_edit_closest_connection_at_point(
		const GraphEdit &self,
		const Vector2 point,
		const real_t max_distance
) {

	Ref<GraphEdit::Connection> gd_connection = self.get_closest_connection_at_point(point, max_distance);
	if (gd_connection.is_null()) {
		return GraphEditConnection();
	}
	GraphEditConnection connection;
	connection.from = gd_connection->from_node;
	connection.from_port = gd_connection->from_port;
	connection.to = gd_connection->to_node;
	connection.to_port = gd_connection->to_port;
	return connection;


}

} // namespace voxel::godot
