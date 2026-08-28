#ifndef VOXEL_BLOCKY_TYPE_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_BLOCKY_TYPE_EDITOR_INSPECTOR_PLUGIN_H

#include "../../../util/godot/classes/editor_inspector_plugin.h"
#include "../../../util/godot/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class EditorInterface);
VOXEL_GODOT_FORWARD_DECLARE(class EditorUndoRedoManager);

namespace voxel {

class VoxelBlockyTypeEditorInspectorPlugin : public voxel::godot::VOXEL_EditorInspectorPlugin {
	GDCLASS(VoxelBlockyTypeEditorInspectorPlugin, voxel::godot::VOXEL_EditorInspectorPlugin)
public:
	void set_editor_interface(EditorInterface *ed);
	void set_undo_redo(EditorUndoRedoManager *urm);

protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	void _voxel_parse_begin(Object *p_object) override;
	bool _voxel_parse_property(Object *p_object, const Variant::Type p_type, const String &p_path,
			const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage,
			const bool p_wide) override;

private:
	// When compiling with GodotCpp, `_bind_methods` isn't optional.
	static void _bind_methods() {}

	EditorInterface *_editor_interface = nullptr;
	EditorUndoRedoManager *_undo_redo = nullptr;
};

} // namespace voxel

#endif // VOXEL_BLOCKY_TYPE_EDITOR_INSPECTOR_PLUGIN_H
