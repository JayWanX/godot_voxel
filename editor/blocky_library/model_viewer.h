#include <scene/gui/control.h>
#include <core/input/input_event.h>
#ifndef VOXEL_GODOT_MODEL_VIEWER_H
#define VOXEL_GODOT_MODEL_VIEWER_H

#include <scene/gui/control.h>

// 头文件中需要用到虚方法声明，因此必须包含此头文件。
#include <core/input/input_event.h>

#include "../../util/godot/macros.h"

class Camera3D;
class SubViewport;

namespace voxel {

class Voxel_Axes3DControl;

// 嵌入在 Control 中用于查看 3D 内容的基础 SubViewport。
// 实现了围绕原点旋转的相机控制。
// Godot 自带 `MeshEditor`，但它专用于 Mesh 资源，无法访问场景层级。
class Voxel_ModelViewer : public Control {
	GDCLASS(Voxel_ModelViewer, Control)
public:
	Voxel_ModelViewer();

	void set_camera_distance(float d);

	// 要查看的内容可以作为该节点的子节点实例化
	Node *get_viewer_root_node() const;

	void gui_input(const Ref<InputEvent> &p_event) override;

private:
	void update_camera();

	static void _bind_methods() {}

	Camera3D *_camera = nullptr;
	float _pitch = 0.f;
	float _yaw = 0.f;
	float _distance = 1.9f;
	Voxel_Axes3DControl *_axes_3d_control = nullptr;
	SubViewport *_viewport;
};

} // namespace voxel

#endif // VOXEL_GODOT_MODEL_VIEWER_H
