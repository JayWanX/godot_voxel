#ifndef VOXEL_GODOT_MATERIAL_H
#define VOXEL_GODOT_MATERIAL_H

#if defined(VOXEL_GODOT)
#include <scene/resources/material.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/material.hpp>
using namespace godot;
#endif

namespace voxel::godot {

// Exposing exclusively 3D material types requires a special hint string, because the base class is Material.
extern const char *MATERIAL_3D_PROPERTY_HINT_STRING;

} // namespace voxel::godot

#endif // VOXEL_GODOT_MATERIAL_H
