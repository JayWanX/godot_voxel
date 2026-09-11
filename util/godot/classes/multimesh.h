#ifndef VOXEL_GODOT_MULTIMESH_H
#define VOXEL_GODOT_MULTIMESH_H

#include <scene/resources/multimesh.h>

namespace voxel::godot {

// 这个 API 可能令人困惑，所以我做了一个封装
int get_visible_instance_count(const MultiMesh &mm);

} // namespace voxel::godot

#endif // VOXEL_GODOT_MULTIMESH_H
