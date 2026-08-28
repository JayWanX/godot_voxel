#include "editor_plugin.h"

namespace voxel::godot {

#if defined(VOXEL_GODOT)

bool VOXEL_EditorPlugin::handles(Object *p_object) const {
	return _voxel_handles(p_object);
}

void VOXEL_EditorPlugin::edit(Object *p_object) {
	return _voxel_edit(p_object);
}

void VOXEL_EditorPlugin::make_visible(bool visible) {
	return _voxel_make_visible(visible);
}

EditorPlugin::AfterGUIInput VOXEL_EditorPlugin::forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) {
	return _voxel_forward_3d_gui_input(p_camera, p_event);
}

void VOXEL_EditorPlugin::save_external_data() {
	_voxel_save_external_data();
}

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 3
String VOXEL_EditorPlugin::get_name() const {
	return _voxel_get_plugin_name();
}
#else
String VOXEL_EditorPlugin::get_plugin_name() const {
	return _voxel_get_plugin_name();
}
#endif

#endif

bool VOXEL_EditorPlugin::_voxel_handles(const Object *p_object) const {
	return false;
}

void VOXEL_EditorPlugin::_voxel_edit(Object *p_object) {}

void VOXEL_EditorPlugin::_voxel_make_visible(bool visible) {}

void VOXEL_EditorPlugin::_voxel_save_external_data() {}

EditorPlugin::AfterGUIInput VOXEL_EditorPlugin::_voxel_forward_3d_gui_input(
		Camera3D *p_camera,
		const Ref<InputEvent> &p_event
) {
	return AFTER_GUI_INPUT_PASS;
}

String VOXEL_EditorPlugin::_voxel_get_plugin_name() const {
	return get_class();
}

} // namespace voxel::godot
