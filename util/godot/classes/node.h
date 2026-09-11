#include <core/version.h>
#ifndef VOXEL_GODOT_NODE_H
#define VOXEL_GODOT_NODE_H

#include <scene/main/node.h>

#include "../../containers/std_vector.h"
#include "../../errors.h"
#include <core/version.h>

namespace voxel::godot {

void set_nodes_owner(Node *root, Node *owner);
void set_nodes_owner_except_root(Node *root, Node *owner);

template <typename T>
inline T *get_node_typed(const Node &self, const NodePath &path) {
	return Object::cast_to<T>(self.get_node(path));
}

void get_node_groups(const Node &node, StdVector<StringName> &out_groups);

enum AutoTranslateMode {
	AUTO_TRANSLATE_MODE_INHERIT,
	AUTO_TRANSLATE_MODE_ALWAYS,
	AUTO_TRANSLATE_MODE_DISABLED,
};

Node::AutoTranslateMode to_godot_auto_translate_mode(const AutoTranslateMode voxel_mode);

void set_node_auto_translate_mode(Node &node, const AutoTranslateMode mode);

} // namespace voxel::godot

#endif // VOXEL_GODOT_NODE_H
