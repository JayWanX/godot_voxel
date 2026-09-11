#include "editor_plugin.h"

namespace voxel::godot {


bool Voxel_EditorPlugin::handles(Object *p_object) const {
	return _voxel_handles(p_object);
}

void Voxel_EditorPlugin::edit(Object *p_object) {
	return _voxel_edit(p_object);
}

void Voxel_EditorPlugin::make_visible(bool visible) {
	return _voxel_make_visible(visible);
}

EditorPlugin::AfterGUIInput Voxel_EditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	return _voxel_forward_3d_gui_input(p_camera, p_event);
}

void Voxel_EditorPlugin::save_external_data() {
	_voxel_save_external_data();
}

String Voxel_EditorPlugin::get_plugin_name() const {
	return _voxel_get_plugin_name();
}


bool Voxel_EditorPlugin::_voxel_handles(const Object *p_object) const {
	return false;
}

void Voxel_EditorPlugin::_voxel_edit(Object *p_object) {}

void Voxel_EditorPlugin::_voxel_make_visible(bool visible) {}

void Voxel_EditorPlugin::_voxel_save_external_data() {}

EditorPlugin::AfterGUIInput Voxel_EditorPlugin::_voxel_forward_3d_gui_input(
		Camera3D *p_camera,
		const Ref<InputEvent> &p_event
) {
	return AFTER_GUI_INPUT_PASS;
}

String Voxel_EditorPlugin::_voxel_get_plugin_name() const {
	return get_class();
}

} // namespace voxel::godot
