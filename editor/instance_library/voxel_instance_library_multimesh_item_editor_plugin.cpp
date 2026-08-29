#include "voxel_instance_library_multimesh_item_editor_plugin.h"
#include "../../util/godot/classes/control.h"
#include "../../util/godot/classes/editor_file_dialog.h"
#include "../../util/godot/classes/editor_interface.h"
#include "../../util/godot/classes/editor_undo_redo_manager.h"
#include "../../util/godot/classes/resource_loader.h"

#ifdef VOXEL_GODOT
#include "../../util/godot/core/callable_mp.h"
#endif

namespace voxel {

VoxelInstanceLibraryMultiMeshItemEditorPlugin::VoxelInstanceLibraryMultiMeshItemEditorPlugin() {}

// 插件构造期间 EditorNode 尚未就绪，因此将工作推迟到 `init()`。
void VoxelInstanceLibraryMultiMeshItemEditorPlugin::init() {
	Control *base_control = get_editor_interface()->get_base_control();

	_open_scene_dialog = memnew(EditorFileDialog);
	PackedStringArray extensions = godot::get_recognized_extensions_for_type(PackedScene::get_class_static());
	for (int i = 0; i < extensions.size(); ++i) {
		_open_scene_dialog->add_filter("*." + extensions[i]);
	}
	_open_scene_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	base_control->add_child(_open_scene_dialog);
	_open_scene_dialog->connect(
			"file_selected",
			callable_mp(this, &VoxelInstanceLibraryMultiMeshItemEditorPlugin::_on_open_scene_dialog_file_selected)
	);
}

bool VoxelInstanceLibraryMultiMeshItemEditorPlugin::_voxel_handles(const Object *p_object) const {
	// TODO 制作一个处理 `VoxelInstanceLibrary` 子资源的插件会破坏检查器。
	// 使用多个子检查器时也有一些注意事项。为了支持在库中同时打开多个子检查器，
	// 我们不能依赖 `edit` 提供给我们的被编辑资源。
	// 参见 https://github.com/godotengine/godot/issues/64700
	return false;
	// const VoxelInstanceLibraryMultiMeshItem *item = Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(p_object);
	// return item != nullptr;
}

void VoxelInstanceLibraryMultiMeshItemEditorPlugin::_voxel_edit(Object *p_object) {
	// VoxelInstanceLibraryMultiMeshItem *item = Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(p_object);
	// _item.reference_ptr(item);
}

void VoxelInstanceLibraryMultiMeshItemEditorPlugin::_voxel_make_visible(bool visible) {
	// if (!visible) {
	// 	_item.unref();
	// }
}

void VoxelInstanceLibraryMultiMeshItemEditorPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		init();

		_inspector_plugin.instantiate();
		_inspector_plugin->listener = this;
		add_inspector_plugin(_inspector_plugin);

	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		remove_inspector_plugin(_inspector_plugin);
	}
}

#if defined(VOXEL_GODOT)
void VoxelInstanceLibraryMultiMeshItemEditorPlugin::_on_update_from_scene_button_pressed(
		VoxelInstanceLibraryMultiMeshItem *item
) {
#endif
	_item.reference_ptr(item);
	ERR_FAIL_COND(_item.is_null());
	voxel::godot::popup_file_dialog(*_open_scene_dialog);
}

namespace {

void update_multimesh_item_from_scene(
		VoxelInstanceLibraryMultiMeshItem &item,
		String scene_file_path,
		EditorUndoRedoManager &ur
) {
	Ref<PackedScene> scene = godot::load_resource(scene_file_path);
	ERR_FAIL_COND(scene.is_null());

	Node *node = scene->instantiate();
	ERR_FAIL_COND(node == nullptr);

	Variant data_before = item.serialize_multimesh_item_properties();
	item.setup_from_template(node);
	Variant data_after = item.serialize_multimesh_item_properties();

	memdelete(node);

	ur.create_action("Update Multimesh Item From Scene");
	ur.add_do_method(&item, "_deserialize_multimesh_item_properties", data_after);
	ur.add_undo_method(&item, "_deserialize_multimesh_item_properties", data_before);
	ur.commit_action(
			// 我们之前已经使用了 `setup_from_template`，它做了与 `do` 相同的工作，
			// 所以提交操作时不需要再运行一次。
			false
	);
}

} // namespace

void VoxelInstanceLibraryMultiMeshItemEditorPlugin::_on_open_scene_dialog_file_selected(String fpath) {
	ERR_FAIL_COND(_item.is_null());
	update_multimesh_item_from_scene(**_item, fpath, *get_undo_redo());
	// 我们已经处理完这个条目
	_item.unref();
}

void VoxelInstanceLibraryMultiMeshItemEditorPlugin::_bind_methods() {}

} // namespace voxel
