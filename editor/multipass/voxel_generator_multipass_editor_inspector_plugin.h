#ifndef VOXEL_GENERATOR_MULTIPASS_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_GENERATOR_MULTIPASS_EDITOR_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"

namespace voxel {

class VoxelGeneratorMultipassEditorInspectorPlugin : public voxel::godot::VOXEL_EditorInspectorPlugin {
	GDCLASS(VoxelGeneratorMultipassEditorInspectorPlugin, voxel::godot::VOXEL_EditorInspectorPlugin)
protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	void _voxel_parse_begin(Object *p_object) override;

private:
	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_GENERATOR_MULTIPASS_EDITOR_INSPECTOR_PLUGIN_H
