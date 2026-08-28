#ifndef VOXEL_INSTANCE_LIBRARY_MULTIMESH_ITEM_INSPECTOR_PLUGIN_H
#define VOXEL_INSTANCE_LIBRARY_MULTIMESH_ITEM_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"

namespace voxel {

class VoxelInstanceLibraryMultiMeshItemEditorPlugin;

class VoxelInstanceLibraryMultiMeshItemInspectorPlugin : public voxel::godot::VOXEL_EditorInspectorPlugin {
	GDCLASS(VoxelInstanceLibraryMultiMeshItemInspectorPlugin, voxel::godot::VOXEL_EditorInspectorPlugin)
public:
	VoxelInstanceLibraryMultiMeshItemEditorPlugin *listener = nullptr;

protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	void _voxel_parse_group(Object *p_object, const String &p_group) override;
	bool _voxel_parse_property(Object *p_object, const Variant::Type p_type, const String &p_path,
			const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage,
			const bool p_wide = false) override;

private:
	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_INSTANCE_LIBRARY_MULTIMESH_ITEM_INSPECTOR_PLUGIN_H
