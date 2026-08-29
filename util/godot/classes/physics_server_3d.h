#ifndef VOXEL_GODOT_PHYSICS_SERVER_3D_H
#define VOXEL_GODOT_PHYSICS_SERVER_3D_H

#if defined(VOXEL_GODOT)
#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 5
#include <servers/physics_server_3d.h>
#else
#include <servers/physics_3d/physics_server_3d.h>
#endif

// 从 Godot 4.7 开始，`PhysicsServer3D` 的枚举被移到了单独的命名空间，出于……C++ 的原因。
// https://github.com/godotengine/godot/pull/120983
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 7
// 让同一段代码在旧版本上也能工作
using PhysicsServer3DEnums = PhysicsServer3D;
#endif

#endif

namespace voxel::godot {

inline void free_physics_server_rid(PhysicsServer3D &ps, const RID &rid) {
#if defined(VOXEL_GODOT)
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 5
	ps.free(rid);
#else
	ps.free_rid(rid);
#endif

#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_PHYSICS_SERVER_3D_H
