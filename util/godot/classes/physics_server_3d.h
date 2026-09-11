#include <core/version.h>
#ifndef VOXEL_GODOT_PHYSICS_SERVER_3D_H
#define VOXEL_GODOT_PHYSICS_SERVER_3D_H

#include <core/version.h>

#include <servers/physics_3d/physics_server_3d.h>

// 从 Godot 4.7 开始，`PhysicsServer3D` 的枚举被移到了单独的命名空间，出于……C++ 的原因。
// https://github.com/godotengine/godot/pull/120983
// 让同一段代码在旧版本上也能工作
using PhysicsServer3DEnums = PhysicsServer3D;


namespace voxel::godot {

inline void free_physics_server_rid(PhysicsServer3D &ps, const RID &rid) {
	ps.free_rid(rid);

}

} // namespace voxel::godot

#endif // VOXEL_GODOT_PHYSICS_SERVER_3D_H
