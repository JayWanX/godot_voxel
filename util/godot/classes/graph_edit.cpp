#include "graph_edit.h"

namespace voxel::godot {

void get_graph_edit_connections(const GraphEdit &self, StdVector<GraphEditConnection> &out_connections) {
#if defined(VOXEL_GODOT)

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 2
	List<GraphEdit::Connection> connections_list;
	self.get_connection_list(&connections_list);

	for (const GraphEdit::Connection &src_con : connections_list) {
		GraphEditConnection dst_con;

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 1
		dst_con.from = src_con.from;
		dst_con.to = src_con.to;
#else
		dst_con.from = src_con.from_node;
		dst_con.to = src_con.to_node;
#endif
		dst_con.from_port = src_con.from_port;
		dst_con.to_port = src_con.to_port;

		out_connections.push_back(dst_con);
	}

#else // Godot 4.3+

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 3
	const List<Ref<GraphEdit::Connection>> &connections_list = self.get_connection_list();
#else
	const Vector<Ref<GraphEdit::Connection>> &connections_list = self.get_connections();
#endif

	for (const Ref<GraphEdit::Connection> &src_con : connections_list) {
		GraphEditConnection dst_con;

		dst_con.from = src_con->from_node;
		dst_con.to = src_con->to_node;
		dst_con.from_port = src_con->from_port;
		dst_con.to_port = src_con->to_port;

		out_connections.push_back(dst_con);
	}
#endif

#endif
}

Vector2 get_graph_edit_scroll_offset(const GraphEdit &self) {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 1
	return self.get_scroll_ofs();
#else
	return self.get_scroll_offset();
#endif
}

bool is_graph_edit_using_snapping(const GraphEdit &self) {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 1
	return self.is_using_snap();
#else
	return self.is_snapping_enabled();
#endif
}

int get_graph_edit_snapping_distance(const GraphEdit &self) {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 1
	return self.get_snap();
#else
	return self.get_snapping_distance();
#endif
}

GraphEditConnection get_graph_edit_closest_connection_at_point(
		const GraphEdit &self,
		const Vector2 point,
		const real_t max_distance
) {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR < 3
	// Not exposed. Could probably re-implement ourselves, but it doesn't seem worth it.
	return GraphEditConnection();

#else

#if defined(VOXEL_GODOT)
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

#endif

#endif
}

} // namespace voxel::godot
