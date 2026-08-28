#ifndef VOXEL_GODOT_SHAPE_3D_H
#define VOXEL_GODOT_SHAPE_3D_H

#include "../core/version.h"
#include "../macros.h"

#if defined(VOXEL_GODOT)

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 2
#include <scene/resources/shape_3d.h>
#else
#include <scene/resources/3d/shape_3d.h>
#endif

#endif

VOXEL_GODOT_FORWARD_DECLARE(class SceneTree);

namespace voxel::godot {

#ifdef DEBUG_ENABLED

inline void set_shape_3d_debug_color(Shape3D &shape, const Color color) {
	// `set_debug_color` only exists in Godot 4.4+.
#if defined(VOXEL_GODOT)
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 4
	shape.set_debug_color(color);
#endif
#endif
}

// This function is primarily to implement a workaround...
// See https://github.com/godotengine/godot/pull/100328
Color get_shape_3d_default_color(const SceneTree &scene_tree);

#endif

} // namespace voxel::godot

#endif // VOXEL_GODOT_SHAPE_3D_H
