#ifndef VOXEL_AXES_3D_CONTROL_H
#define VOXEL_AXES_3D_CONTROL_H

#include "../../util/godot/classes/control.h"

namespace voxel {

// Displays 3D axes in a Control node using only 2D drawing.
// Similar to `ViewportRotationControl`, but much smaller to fit in smaller editors.
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
