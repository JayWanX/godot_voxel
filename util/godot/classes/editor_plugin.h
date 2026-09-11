#ifndef VOXEL_GODOT_EDITOR_PLUGIN_H
#define VOXEL_GODOT_EDITOR_PLUGIN_H

#include <core/version.h>

#include <editor/plugins/editor_plugin.h>


namespace voxel::godot {

class Voxel_EditorPlugin : public EditorPlugin {
	GDCLASS(Voxel_EditorPlugin, EditorPlugin)
public:
	bool handles(Object *p_object) const override;
	void edit(Object *p_object) override;
	void make_visible(bool visible) override;
	EditorPlugin::AfterGUIInput forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event) override;
	void save_external_data() override;

	String get_plugin_name() const override;


protected:
	virtual bool _voxel_handles(const Object *p_object) const;
	virtual void _voxel_edit(Object *p_object);
	virtual void _voxel_make_visible(bool visible);
	virtual EditorPlugin::AfterGUIInput _voxel_forward_3d_gui_input(Camera3D *p_camera, const Ref<InputEvent> &p_event);
	virtual void _voxel_save_external_data();
	virtual String _voxel_get_plugin_name() const;

private:
	static void _bind_methods() {}
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_EDITOR_PLUGIN_H
