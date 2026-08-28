#ifndef VOXEL_BLOCKY_MODEL_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_BLOCKY_MODEL_EDITOR_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"
#include "../../util/godot/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class EditorUndoRedoManager);

namespace voxel {

class VoxelBlockyModelEditorInspectorPlugin : public voxel::godot::VOXEL_EditorInspectorPlugin {
	GDCLASS(VoxelBlockyModelEditorInspectorPlugin, voxel::godot::VOXEL_EditorInspectorPlugin)
public:
	// `EditorUndoRedoManager` isn't a singleton, so it has to be injected.
	void set_undo_redo(EditorUndoRedoManager *urm);

protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	void _voxel_parse_begin(Object *p_object) override;

private:
	static void _bind_methods() {}

	EditorUndoRedoManager *_undo_redo = nullptr;
};

} // namespace voxel

#endif // VOXEL_BLOCKY_MODEL_EDITOR_INSPECTOR_PLUGIN_H
