#ifndef VOXEL_EDITOR_CAMERA_CACHE_H
#define VOXEL_EDITOR_CAMERA_CACHE_H

#include "../util/godot/classes/camera_3d.h"
#include "../util/godot/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class Camera3D);

namespace voxel::godot {

// 这是一个临时解决方案。
// 在 Godot 编辑器中，`get_viewport()->get_camera_3d()` 始终返回 `nullptr`，因此如果被编辑场景中的节点
// 需要相机，它就无法获取。此外，编辑器可以有多个相机（渲染同一场景，或不同的
// 场景！）。EditorPlugin 可以连同空间输入事件一起访问这些相机，但要从未被编辑的节点
// 获取相机非常不实用，而且还需要选中该节点。因此，我们改为缓存相机。

Vector3 get_3d_editor_camera_position();
void set_3d_editor_camera_cache(Camera3D *camera);

} // namespace voxel::godot

#endif // VOXEL_EDITOR_CAMERA_CACHE_H
