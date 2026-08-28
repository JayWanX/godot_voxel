#ifndef VOXEL_GODOT_TREE_ITEM_H
#define VOXEL_GODOT_TREE_ITEM_H

#if defined(VOXEL_GODOT)
#include <scene/gui/tree.h>
#endif

#include "node.h"

namespace voxel::godot {
namespace TreeItemUtilities {

inline void set_auto_translate_mode(TreeItem &item, const unsigned int column, const AutoTranslateMode mode) {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 4
	item.set_auto_translate_mode(column, to_godot_auto_translate_mode(mode));
#endif
}

} // namespace TreeItemUtilities
} // namespace voxel::godot

#endif // VOXEL_GODOT_TREE_ITEM_H
