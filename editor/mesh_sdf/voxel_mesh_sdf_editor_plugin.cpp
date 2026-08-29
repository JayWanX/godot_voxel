#include "voxel_mesh_sdf_editor_plugin.h"
#include "../../edition/voxel_mesh_sdf_gd.h"
#include "voxel_mesh_sdf_viewer.h"

namespace voxel {

bool VoxelMeshSDFInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return Object::cast_to<VoxelMeshSDF>(p_object) != nullptr;
}

void VoxelMeshSDFInspectorPlugin::_voxel_parse_begin(Object *p_object) {
	VoxelMeshSDFViewer *viewer = memnew(VoxelMeshSDFViewer);
	add_custom_control(viewer);
	VoxelMeshSDF *mesh_sdf = Object::cast_to<VoxelMeshSDF>(p_object);
	ERR_FAIL_COND(mesh_sdf == nullptr);
	viewer->set_mesh_sdf(mesh_sdf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VoxelMeshSDFEditorPlugin::VoxelMeshSDFEditorPlugin() {}

bool VoxelMeshSDFEditorPlugin::_voxel_handles(const Object *p_object) const {
	ERR_FAIL_COND_V(p_object == nullptr, false);
	return Object::cast_to<VoxelMeshSDF>(p_object) != nullptr;
}

void VoxelMeshSDFEditorPlugin::_voxel_edit(Object *p_object) {
	//_mesh_sdf = p_object;
}

void VoxelMeshSDFEditorPlugin::_voxel_make_visible(bool visible) {
	//_mesh_sdf.unref();
}

void VoxelMeshSDFEditorPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		_inspector_plugin.instantiate();
		// TODO 为什么其他 Godot 插件可以在构造函数中做这件事？？
		// 我发现我不能把它放在构造函数中，
		// 否则 `add_inspector_plugin` 会导致另一个编辑器插件在退出时泄漏……唉
		add_inspector_plugin(_inspector_plugin);

	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		remove_inspector_plugin(_inspector_plugin);
	}
}

} // namespace voxel
