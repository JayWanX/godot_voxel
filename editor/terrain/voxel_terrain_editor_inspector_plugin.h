#ifndef VOXEL_TERRAIN_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_TERRAIN_EDITOR_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"

namespace voxel {

class VoxelTerrainEditorInspectorPlugin : public voxel::godot::Voxel_EditorInspectorPlugin {
	GDCLASS(VoxelTerrainEditorInspectorPlugin, voxel::godot::Voxel_EditorInspectorPlugin)
protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	bool _voxel_parse_property(Object *p_object, const Variant::Type p_type, const String &p_path,
			const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage,
			const bool p_wide = false) override;

private:
	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_TERRAIN_EDITOR_INSPECTOR_PLUGIN_H
