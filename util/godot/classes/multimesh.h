#ifndef VOXEL_GODOT_MULTIMESH_H
#define VOXEL_GODOT_MULTIMESH_H

#if defined(VOXEL_GODOT)
#include <scene/resources/multimesh.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/multi_mesh.hpp>
using namespace godot;
#endif

namespace voxel::godot {

// This API can be confusing so I made a wrapper
int get_visible_instance_count(const MultiMesh &mm);

} // namespace voxel::godot

#endif // VOXEL_GODOT_MULTIMESH_H
