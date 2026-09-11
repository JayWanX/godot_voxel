#include "shape_3d.h"
#include <core/math/color.h>
#include "project_settings.h"
#include <scene/main/scene_tree.h>
#include "../../math/color.h"
#include <scene/main/scene_tree.h>

namespace voxel::godot {

#ifdef DEBUG_ENABLED

Color get_shape_3d_default_color(const SceneTree &scene_tree) {
	return scene_tree.get_debug_collisions_color();

}

#endif

} // namespace voxel::godot
