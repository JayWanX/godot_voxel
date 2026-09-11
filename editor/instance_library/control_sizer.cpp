#include "control_sizer.h"
#include "../../util/errors.h"
#include <core/input/input_event.h>
#include <core/input/input_event.h>
#include "../../util/godot/core/input_enums.h"
#include <core/version.h>
#include <editor/themes/editor_scale.h>
#include "../../util/math/funcs.h"
#include <core/input/input_event.h>
#include <core/input/input_event.h>
#include <editor/themes/editor_scale.h>

namespace voxel {

Voxel_ControlSizer::Voxel_ControlSizer() {
	set_default_cursor_shape(Control::CURSOR_VSIZE);
	const real_t editor_scale = EDSCALE;
	set_custom_minimum_size(Vector2(0, editor_scale * 5));
}

void Voxel_ControlSizer::set_target_control(Control *control) {
	_target_control.set(control);
}

void Voxel_ControlSizer::gui_input(const Ref<InputEvent> &p_event) {

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		Control *target_control = _target_control.get();
		VOXEL_ASSERT_RETURN(target_control != nullptr);

		if (mb->is_pressed()) {
			if (mb->get_button_index() == ::godot::MOUSE_BUTTON_LEFT) {
				_dragging = true;
			}
		} else {
			_dragging = false;
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (_dragging) {
			Control *target_control = _target_control.get();
			VOXEL_ASSERT_RETURN(target_control != nullptr);

			const Vector2 ms = target_control->get_custom_minimum_size();
			// 假设 UI 未缩放
			const Vector2 rel = mm->get_relative();
			// 目前假设是垂直方向
			// TODO 将 min_size 限制为 `target.get_minimum_size()`？
			target_control->set_custom_minimum_size(
					Vector2(ms.x, math::clamp<real_t>(ms.y + rel.y, _min_size, _max_size))
			);
		}
	}
}

void Voxel_ControlSizer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_MOUSE_ENTER: {
			_mouse_inside = true;
			queue_redraw();
		} break;

		case NOTIFICATION_MOUSE_EXIT: {
			_mouse_inside = false;
			queue_redraw();
		} break;

		case NOTIFICATION_ENTER_TREE:
			cache_theme();
			break;

		case NOTIFICATION_THEME_CHANGED:
			cache_theme();
			break;

		case NOTIFICATION_DRAW: {
			if (_dragging || _mouse_inside) {
				draw_texture(_hover_icon, (get_size() - _hover_icon->get_size()) / 2);
			}
		} break;
	}
}

void Voxel_ControlSizer::cache_theme() {
	// TODO 我想缓存这个主题图标查找。
	// TODO 有一个框架级的 StringName 缓存单例
	_hover_icon = get_theme_icon("v_grabber", "SplitContainer");
}

void Voxel_ControlSizer::_bind_methods() {}

} // namespace voxel
