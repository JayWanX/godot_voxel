#ifndef VOXEL_GENERATOR_MULTIPASS_EDITOR_PLUGIN_H
#define VOXEL_GENERATOR_MULTIPASS_EDITOR_PLUGIN_H

#include "../../util/godot/classes/editor_plugin.h"
#include "voxel_generator_multipass_editor_inspector_plugin.h"

namespace voxel {

class VoxelGeneratorMultipassEditorPlugin : public voxel::godot::Voxel_EditorPlugin {
	GDCLASS(VoxelGeneratorMultipassEditorPlugin, voxel::godot::Voxel_EditorPlugin)
public:
	VoxelGeneratorMultipassEditorPlugin();

private:
	void _notification(int p_what);

	static void _bind_methods() {}

	Ref<VoxelGeneratorMultipassEditorInspectorPlugin> _inspector_plugin;
};

} // namespace voxel

#endif // VOXEL_GENERATOR_MULTIPASS_EDITOR_PLUGIN_H
