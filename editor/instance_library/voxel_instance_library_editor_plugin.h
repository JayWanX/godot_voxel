#ifndef VOXEL_INSTANCE_LIBRARY_EDITOR_PLUGIN_H
#define VOXEL_INSTANCE_LIBRARY_EDITOR_PLUGIN_H

#include "../../terrain/instancing/voxel_instance_library.h"
#include "../../util/godot/classes/editor_plugin.h"
#include "voxel_instance_library_inspector_plugin.h"

class Control;
class MenuButton;
class ConfirmationDialog;
class AcceptDialog;

namespace voxel {

class VoxelInstanceLibraryEditorPlugin : public voxel::godot::Voxel_EditorPlugin {
	GDCLASS(VoxelInstanceLibraryEditorPlugin, voxel::godot::Voxel_EditorPlugin)
public:
	VoxelInstanceLibraryEditorPlugin();

	// 因为编译为模块时这在基类中是 protected
	EditorUndoRedoManager &get_undo_redo2();

protected:
	bool _voxel_handles(const Object *p_object) const override;
	void _voxel_edit(Object *p_object) override;

	String _voxel_get_plugin_name() const override {
		return "VoxelInstanceLibrary";
	}

private:
	void init();
	void _notification(int p_what);

	static void _bind_methods();

	Ref<VoxelInstanceLibraryInspectorPlugin> _inspector_plugin;
};

} // namespace voxel

#endif // VOXEL_INSTANCE_LIBRARY_EDITOR_PLUGIN_H
