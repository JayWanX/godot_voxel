#ifndef VOXEL_INSTANCE_LIBRARY_EDITOR_PLUGIN_H
#define VOXEL_INSTANCE_LIBRARY_EDITOR_PLUGIN_H

#include "../../terrain/instancing/voxel_instance_library.h"
#include "../../util/godot/classes/editor_plugin.h"
#include "voxel_instance_library_inspector_plugin.h"

VOXEL_GODOT_FORWARD_DECLARE(class Control)
VOXEL_GODOT_FORWARD_DECLARE(class MenuButton)
VOXEL_GODOT_FORWARD_DECLARE(class ConfirmationDialog)
VOXEL_GODOT_FORWARD_DECLARE(class AcceptDialog)

namespace voxel {

class VoxelInstanceLibraryEditorPlugin : public voxel::godot::Voxel_EditorPlugin {
	GDCLASS(VoxelInstanceLibraryEditorPlugin, voxel::godot::Voxel_EditorPlugin)
public:
	VoxelInstanceLibraryEditorPlugin();

	// Because this is protected in the base class when compiling as a module
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
