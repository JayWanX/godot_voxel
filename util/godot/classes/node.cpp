#include "node.h"
#include <core/version.h>
#include <core/version.h>


namespace voxel::godot {

template <typename F>
void for_each_node_depth_first(Node *parent, F f) {
	ERR_FAIL_COND(parent == nullptr);
	f(parent);
	for (int i = 0; i < parent->get_child_count(); ++i) {
		for_each_node_depth_first(parent->get_child(i), f);
	}
}

void set_nodes_owner(Node *root, Node *owner) {
	for_each_node_depth_first(root, [owner](Node *node) { //
		node->set_owner(owner);
	});
}

void set_nodes_owner_except_root(Node *root, Node *owner) {
	ERR_FAIL_COND(root == nullptr);
	for (int i = 0; i < root->get_child_count(); ++i) {
		set_nodes_owner(root->get_child(i), owner);
	}
}

void get_node_groups(const Node &node, StdVector<StringName> &out_groups) {
	List<Node::GroupInfo> gi;
	node.get_groups(&gi);
	for (const Node::GroupInfo &g : gi) {
		out_groups.push_back(g.name);
	}

}

Node::AutoTranslateMode to_godot_auto_translate_mode(const AutoTranslateMode voxel_mode) {
	switch (voxel_mode) {
		case AUTO_TRANSLATE_MODE_INHERIT:
			return Node::AUTO_TRANSLATE_MODE_INHERIT;
		case AUTO_TRANSLATE_MODE_ALWAYS:
			return Node::AUTO_TRANSLATE_MODE_ALWAYS;
		case AUTO_TRANSLATE_MODE_DISABLED:
			return Node::AUTO_TRANSLATE_MODE_DISABLED;
		default:
			VOXEL_PRINT_ERROR("Unhandled enum");
			return Node::AUTO_TRANSLATE_MODE_INHERIT;
	}
}

void set_node_auto_translate_mode(Node &node, const AutoTranslateMode mode) {
	node.set_auto_translate_mode(to_godot_auto_translate_mode(mode));
}

} // namespace voxel::godot
