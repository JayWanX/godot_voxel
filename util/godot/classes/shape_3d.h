#include <core/version.h>
#ifndef VOXEL_GODOT_SHAPE_3D_H
#define VOXEL_GODOT_SHAPE_3D_H

#include <core/version.h>
#include "../macros.h"


#include <scene/resources/3d/shape_3d.h>


class SceneTree;

namespace voxel::godot {

#ifdef DEBUG_ENABLED

inline void set_shape_3d_debug_color(Shape3D &shape, const Color color) {
	// `set_debug_color` 仅在 Godot 4.4+ 中存在。
	shape.set_debug_color(color);
}

// 这个函数主要用于实现一个变通方案……
// 参见 https://github.com/godotengine/godot/pull/100328
Color get_shape_3d_default_color(const SceneTree &scene_tree);

#endif

} // namespace voxel::godot

#endif // VOXEL_GODOT_SHAPE_3D_H
