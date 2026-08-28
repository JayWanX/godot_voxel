#ifndef VOXEL_MESH_SDF_EDITOR_PLUGIN_H
#define VOXEL_MESH_SDF_EDITOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"
#include "../../util/godot/classes/editor_plugin.h"

namespace voxel {

class VoxelMeshSDFInspectorPlugin : public voxel::godot::Voxel_EditorInspectorPlugin {
	GDCLASS(VoxelMeshSDFInspectorPlugin, voxel::godot::Voxel_EditorInspectorPlugin)
protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	void _voxel_parse_begin(Object *p_object) override;

private:
	static void _bind_methods() {}
};

class VoxelMeshSDFEditorPlugin : public voxel::godot::Voxel_EditorPlugin {
	GDCLASS(VoxelMeshSDFEditorPlugin, voxel::godot::Voxel_EditorPlugin)
public:
	VoxelMeshSDFEditorPlugin();

protected:
	bool _voxel_handles(const Object *p_object) const override;
	void _voxel_edit(Object *p_object) override;
	void _voxel_make_visible(bool visible) override;

private:
	void _notification(int p_what);

	static void _bind_methods() {}

	Ref<VoxelMeshSDFInspectorPlugin> _inspector_plugin;
};

} // namespace voxel

#endif // VOXEL_MESH_SDF_EDITOR_PLUGIN_H
