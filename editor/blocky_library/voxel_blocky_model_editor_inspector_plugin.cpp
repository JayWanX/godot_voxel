#include "voxel_blocky_model_editor_inspector_plugin.h"
#include "voxel_blocky_model_viewer.h"

namespace voxel {

void VoxelBlockyModelEditorInspectorPlugin::set_undo_redo(EditorUndoRedoManager *urm) {
	_undo_redo = urm;
}

bool VoxelBlockyModelEditorInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return Object::cast_to<VoxelBlockyModel>(p_object) != nullptr;
}

void VoxelBlockyModelEditorInspectorPlugin::_voxel_parse_begin(Object *p_object) {
	const VoxelBlockyModel *model_ptr = Object::cast_to<VoxelBlockyModel>(p_object);
	VOXEL_ASSERT_RETURN(model_ptr != nullptr);

	Ref<VoxelBlockyModel> model(model_ptr);

	VoxelBlockyModelViewer *viewer = memnew(VoxelBlockyModelViewer);
	viewer->set_model(model);
	viewer->set_undo_redo(_undo_redo);
	add_custom_control(viewer);
	return;
}

} // namespace voxel
