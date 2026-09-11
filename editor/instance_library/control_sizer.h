#include <scene/gui/control.h>
#ifndef VOXEL_GODOT_CONTROL_SIZER_H
#define VOXEL_GODOT_CONTROL_SIZER_H

#include <scene/gui/control.h>
#include "../../util/godot/object_weak_ref.h"

namespace voxel {

// 实现与 SplitContainer 中间调整大小的手柄类似的逻辑，但作用于目标控件上
class Voxel_ControlSizer : public Control {
	GDCLASS(Voxel_ControlSizer, Control)
public:
	Voxel_ControlSizer();

	void set_target_control(Control *control);

	void gui_input(const Ref<InputEvent> &p_event) override;

private:
	static void _bind_methods();

	void _notification(int p_what);

	void cache_theme();

	voxel::godot::ObjectWeakRef<Control> _target_control;
	bool _dragging = false;
	bool _mouse_inside = false;
	float _min_size = 10.0;
	float _max_size = 1000.0;
	Ref<Texture2D> _hover_icon;
};

} // namespace voxel

#endif // VOXEL_GODOT_CONTROL_SIZER_H
