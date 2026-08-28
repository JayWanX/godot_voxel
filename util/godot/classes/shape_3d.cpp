#include "shape_3d.h"
#include "../../math/color.h"
#include "project_settings.h"
#include "scene_tree.h"

namespace voxel::godot {

#ifdef DEBUG_ENABLED

Color get_shape_3d_default_color(const SceneTree &scene_tree) {
#if defined(VOXEL_GODOT)
	return scene_tree.get_debug_collisions_color();

#endif
}

#endif

} // namespace voxel::godot
