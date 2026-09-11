#ifndef VOXEL_GODOT_TREE_ITEM_H
#define VOXEL_GODOT_TREE_ITEM_H

#include <scene/gui/tree.h>

#include "node.h"

namespace voxel::godot {
namespace TreeItemUtilities {

inline void set_auto_translate_mode(TreeItem &item, const unsigned int column, const AutoTranslateMode mode) {
	item.set_auto_translate_mode(column, to_godot_auto_translate_mode(mode));
}

} // namespace TreeItemUtilities
} // namespace voxel::godot

#endif // VOXEL_GODOT_TREE_ITEM_H
