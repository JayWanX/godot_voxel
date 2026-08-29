#include "voxel_blocky_type_library_editor_inspector_plugin.h"
#include "../../../util/godot/classes/button.h"
#include "../../../util/godot/core/string.h"
#include "voxel_blocky_type_library_ids_dialog.h"

#ifdef VOXEL_GODOT
#include "../../../util/godot/core/callable_mp.h"
#include "../../../util/godot/core/class_db.h"
#endif

namespace voxel {

bool VoxelBlockyTypeLibraryEditorInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return Object::cast_to<VoxelBlockyTypeLibrary>(p_object) != nullptr;
}

void VoxelBlockyTypeLibraryEditorInspectorPlugin::_voxel_parse_end(Object *p_object) {
	const VoxelBlockyTypeLibrary *library_ptr = Object::cast_to<VoxelBlockyTypeLibrary>(p_object);
	VOXEL_ASSERT_RETURN(library_ptr != nullptr);
	Ref<VoxelBlockyTypeLibrary> library(library_ptr);

	Button *button = memnew(Button);
	button->set_text(VOXEL_TTR("Inspect IDs..."));

	button->connect(
			"pressed",
			callable_mp(this, &VoxelBlockyTypeLibraryEditorInspectorPlugin::_on_inspect_ids_button_pressed)
					.bind(library)
	);

	// TODO 我想把这个按钮放在检查器中 VoxelBlockyTypeLibrary 部分的末尾，
	// 而不是最底部……但我该怎么做？
	add_custom_control(button);
}

void VoxelBlockyTypeLibraryEditorInspectorPlugin::set_ids_dialog(VoxelBlockyTypeLibraryIDSDialog *ids_dialog) {
	_ids_dialog = ids_dialog;
}

void VoxelBlockyTypeLibraryEditorInspectorPlugin::_on_inspect_ids_button_pressed(Ref<VoxelBlockyTypeLibrary> library) {
	VOXEL_ASSERT_RETURN(_ids_dialog != nullptr);
	_ids_dialog->set_library(library);
	_ids_dialog->popup_centered();
}

void VoxelBlockyTypeLibraryEditorInspectorPlugin::_bind_methods() {}

} // namespace voxel
