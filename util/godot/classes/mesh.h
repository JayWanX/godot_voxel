#ifndef VOXEL_GODOT_MESH_H
#define VOXEL_GODOT_MESH_H

#include "../../containers/span.h"

#if defined(VOXEL_GODOT)
#include <scene/resources/mesh.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/mesh.hpp>
using namespace godot;
#endif

namespace voxel::godot {

// Mesh utilities

bool is_surface_triangulated(const Array &surface);
bool is_mesh_empty(Span<const Array> surfaces);

void scale_vec3_array(PackedVector3Array &array, float scale);
void offset_vec3_array(PackedVector3Array &array, Vector3 offset);
void scale_surface(Array &surface, float scale);
void offset_surface(Array &surface, Vector3 offset);

} // namespace voxel::godot

#endif // VOXEL_GODOT_MESH_H
