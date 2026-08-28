#include "shape_3d.h"
#include "../../math/color.h"
#include "project_settings.h"
#include "scene_tree.h"

namespace voxel::godot {

#ifdef DEBUG_ENABLED

Color get_shape_3d_default_color(const SceneTree &scene_tree) {
#if defined(VOXEL_GODOT)
	return scene_tree.get_debug_collisions_color();

#elif defined(VOXEL_GODOT_EXTENSION)
	// TODO GDX: SceneTree.get_debug_collisions_color is not exposed
	return ProjectSettings::get_singleton()->get_setting(
			"debug/shapes/collision/shape_color", Color(0.0, 0.6, 0.7, 0.42)
	);
#endif
}

#endif

} // namespace voxel::godot
