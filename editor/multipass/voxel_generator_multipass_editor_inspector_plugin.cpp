#include "voxel_generator_multipass_editor_inspector_plugin.h"
#include "../../generators/multipass/voxel_generator_multipass_cb.h"
#include "voxel_generator_multipass_cache_viewer.h"

namespace voxel {

bool VoxelGeneratorMultipassEditorInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return Object::cast_to<VoxelGeneratorMultipassCB>(p_object) != nullptr;
}

void VoxelGeneratorMultipassEditorInspectorPlugin::_voxel_parse_begin(Object *p_object) {
	VoxelGeneratorMultipassCacheViewer *viewer = memnew(VoxelGeneratorMultipassCacheViewer);
	add_custom_control(viewer);
	VoxelGeneratorMultipassCB *mesh_sdf = Object::cast_to<VoxelGeneratorMultipassCB>(p_object);
	ERR_FAIL_COND(mesh_sdf == nullptr);
	viewer->set_generator(mesh_sdf);
}

} // namespace voxel
