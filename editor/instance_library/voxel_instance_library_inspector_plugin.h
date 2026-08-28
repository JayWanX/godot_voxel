#ifndef VOXEL_INSTANCE_LIBRARY_INSPECTOR_PLUGIN_H
#define VOXEL_INSTANCE_LIBRARY_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"
#include "../../util/godot/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class Control)

namespace voxel {

class VoxelInstanceLibraryEditorPlugin;

class VoxelInstanceLibraryInspectorPlugin : public voxel::godot::Voxel_EditorInspectorPlugin {
	GDCLASS(VoxelInstanceLibraryInspectorPlugin, voxel::godot::Voxel_EditorInspectorPlugin)
public:
	Control *icon_provider = nullptr;
	VoxelInstanceLibraryEditorPlugin *plugin = nullptr;

protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	void _voxel_parse_begin(Object *p_object) override;

	bool _voxel_parse_property(
			Object *p_object,
			const Variant::Type p_type,
			const String &p_path,
			const PropertyHint p_hint,
			const String &p_hint_text,
			const BitField<PropertyUsageFlags> p_usage,
			const bool p_wide
	) override;

private:
	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_INSTANCE_LIBRARY_INSPECTOR_PLUGIN_H
