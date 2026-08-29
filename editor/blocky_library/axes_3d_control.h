#ifndef VOXEL_AXES_3D_CONTROL_H
#define VOXEL_AXES_3D_CONTROL_H

#include "../../util/godot/classes/control.h"

namespace voxel {

// 仅使用 2D 绘制在 Control 节点中显示 3D 坐标轴。
// 类似 `ViewportRotationControl`，但体积小得多，适合较小的编辑器。
class Voxel_Axes3DControl : public Control {
	GDCLASS(Voxel_Axes3DControl, Control)
public:
	void set_basis_3d(Basis basis);

private:
	void _notification(int p_what);
	void draw();

	static void _bind_methods() {}

	Basis _basis;
};

} // namespace voxel

#endif // VOXEL_AXES_3D_CONTROL_H
