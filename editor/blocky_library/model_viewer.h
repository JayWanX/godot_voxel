#ifndef VOXEL_GODOT_MODEL_VIEWER_H
#define VOXEL_GODOT_MODEL_VIEWER_H

#include "../../util/godot/classes/control.h"

// Required in header for GDExtension builds, due to how virtual methods are implemented
#include "../../util/godot/classes/input_event.h"

#include "../../util/godot/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class Camera3D);
VOXEL_GODOT_FORWARD_DECLARE(class SubViewport);

namespace voxel {

class VOXEL_Axes3DControl;

// Basic SubViewport embedded in a Control for viewing 3D stuff.
// Implements camera controls orbitting around the origin.
// Godot has `MeshEditor` but it is specialized for Mesh resources without access to the hierarchy.
class VOXEL_ModelViewer : public Control {
	GDCLASS(VOXEL_ModelViewer, Control)
public:
	VOXEL_ModelViewer();

	void set_camera_distance(float d);

	// Stuff to view can be instanced under this node
	Node *get_viewer_root_node() const;

#if defined(VOXEL_GODOT)
	void gui_input(const Ref<InputEvent> &p_event) override;
#elif defined(VOXEL_GODOT_EXTENSION)
	void _gui_input(const Ref<InputEvent> &p_event) override;
#endif

private:
	void update_camera();

	// When compiling with GodotCpp, `_bind_methods` isn't optional.
	static void _bind_methods() {}

	Camera3D *_camera = nullptr;
	float _pitch = 0.f;
	float _yaw = 0.f;
	float _distance = 1.9f;
	VOXEL_Axes3DControl *_axes_3d_control = nullptr;
	SubViewport *_viewport;
};

} // namespace voxel

#endif // VOXEL_GODOT_MODEL_VIEWER_H
