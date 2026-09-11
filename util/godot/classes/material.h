#ifndef VOXEL_GODOT_MATERIAL_H
#define VOXEL_GODOT_MATERIAL_H

#include <scene/resources/material.h>

namespace voxel::godot {

// 只暴露 3D 材质类型需要特殊的提示字符串，因为基类是 Material。
extern const char *MATERIAL_3D_PROPERTY_HINT_STRING;

} // namespace voxel::godot

#endif // VOXEL_GODOT_MATERIAL_H
