#ifndef VOXEL_GODOT_PHYSICS_SERVER_3D_H
#define VOXEL_GODOT_PHYSICS_SERVER_3D_H

#if defined(VOXEL_GODOT)
#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 5
#include <servers/physics_server_3d.h>
#else
#include <servers/physics_3d/physics_server_3d.h>
#endif

// Starting from Godot 4.7, enums of `PhysicsServer3D` were moved to a separate namespace, for... C++ reasons.
// https://github.com/godotengine/godot/pull/120983
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 7
// Make the same code work in older versions
using PhysicsServer3DEnums = PhysicsServer3D;
#endif

#elif defined(VOXEL_GODOT_EXTENSION)

#include <godot_cpp/classes/physics_server3d.hpp>
using namespace godot;
using PhysicsServer3DEnums = godot::PhysicsServer3D;

#endif

namespace voxel::godot {

inline void free_physics_server_rid(PhysicsServer3D &ps, const RID &rid) {
#if defined(VOXEL_GODOT)
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 5
	ps.free(rid);
#else
	ps.free_rid(rid);
#endif

#elif defined(VOXEL_GODOT_EXTENSION)
	ps.free_rid(rid);
#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_PHYSICS_SERVER_3D_H
