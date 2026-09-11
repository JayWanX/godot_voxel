#include "voxel_instance_library_list_editor.h"
#include "../../constants/voxel_string_names.h"
#include "../../terrain/instancing/voxel_instance_library_multimesh_item.h"
#include "../../terrain/instancing/voxel_instance_library_scene_item.h"
#include <core/version.h>
#include <scene/resources/3d/primitive_meshes.h>
#include "../../util/godot/classes/button.h"
#include <scene/gui/dialogs.h>
#include "../../util/godot/classes/editor_file_dialog.h"
#include <core/version.h>
#include <editor/editor_interface.h>
#include <editor/editor_undo_redo_manager.h>
#include <scene/gui/item_list.h>
#include <scene/gui/menu_button.h>
#include <scene/gui/popup_menu.h>
#include "../../util/godot/classes/resource_loader.h"
#include <scene/gui/box_container.h>
#include <core/variant/array.h>
#include <core/version.h>
#include <editor/themes/editor_scale.h>
#include "../../util/profiling.h"
#include "voxel_instance_library_editor_plugin.h"
#include <scene/resources/3d/primitive_meshes.h>
#include <scene/gui/dialogs.h>
#include <editor/plugins/editor_plugin.h>
#include <editor/editor_undo_redo_manager.h>
#include <scene/gui/item_list.h>
#include <scene/gui/menu_button.h>
#include <scene/gui/popup_menu.h>
#include <scene/gui/box_container.h>
#include <core/variant/array.h>
#include <editor/themes/editor_scale.h>
#include <core/version.h>
#include <core/object/callable_mp.h>


namespace voxel {

VoxelInstanceLibraryListEditor::VoxelInstanceLibraryListEditor() {}

void VoxelInstanceLibraryListEditor::setup(const Control *icon_provider, VoxelInstanceLibraryEditorPlugin *plugin) {
	using Self = VoxelInstanceLibraryListEditor;

	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(_item_list == nullptr);
	VOXEL_ASSERT_RETURN(icon_provider != nullptr);
	VOXEL_ASSERT_RETURN(plugin != nullptr);

	const VoxelStringNames &sn = VoxelStringNames::get_singleton();
	const float editor_scale = EDSCALE;

	set_custom_minimum_size(Vector2(10, 200 * editor_scale));

	_item_list = memnew(ItemList);
	_item_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_item_list->connect("item_selected", callable_mp(this, &Self::on_list_item_selected));
	add_child(_item_list);

	// Buttons
	{
		VBoxContainer *buttons_container = memnew(VBoxContainer);

		MenuButton *button_add = memnew(MenuButton);
		godot::set_button_icon(*button_add, icon_provider->get_theme_icon(sn.Add, sn.EditorIcons));
		button_add->get_popup()->add_item("MultiMesh item (fast)", BUTTON_ADD_MULTIMESH_ITEM);
		button_add->get_popup()->add_item("Scene item (slow)", BUTTON_ADD_SCENE_ITEM);
		button_add->get_popup()->connect("id_pressed", callable_mp(this, &Self::on_button_pressed));
		buttons_container->add_child(button_add);

		Button *button_remove = memnew(Button);
		godot::set_button_icon(*button_remove, icon_provider->get_theme_icon(sn.Remove, sn.EditorIcons));
		button_remove->set_flat(true);
		button_remove->connect("pressed", callable_mp(this, &Self::on_remove_item_button_pressed));
		buttons_container->add_child(button_remove);

		add_child(buttons_container);
	}

	// 对话框
	// TODO 可能需要优化。
	// 在编辑器中，对话框是独立的窗口，而这里每次检查一个库时它们都会被重新创建。
	{
		VOXEL_PROFILE_SCOPE_NAMED("Dialogs");

		Control *base_control = plugin->get_editor_interface()->get_base_control();

		_confirmation_dialog = memnew(ConfirmationDialog);
		_confirmation_dialog->connect("confirmed", callable_mp(this, &Self::on_remove_item_confirmed));
		base_control->add_child(_confirmation_dialog);

		_open_scene_dialog = memnew(EditorFileDialog);
		const PackedStringArray extensions = godot::get_recognized_extensions_for_type(PackedScene::get_class_static());
		for (int i = 0; i < extensions.size(); ++i) {
			_open_scene_dialog->add_filter("*." + extensions[i]);
		}
		_open_scene_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
		base_control->add_child(_open_scene_dialog);
		_open_scene_dialog->connect("file_selected", callable_mp(this, &Self::on_open_scene_dialog_file_selected));
	}

	_plugin = plugin;
}

void VoxelInstanceLibraryListEditor::set_library(Ref<VoxelInstanceLibrary> library) {
	if (_library == library) {
		return;
	}

	// if (_library.is_valid()) {
	// 	_library->remove_listener(this);
	// }

	_library = library;

	// if (_library.is_valid()) {
	// 	_library->add_listener(this);
	// }

	update_list_from_library();
}

void VoxelInstanceLibraryListEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
			set_process(true);
			break;

		case NOTIFICATION_EXIT_TREE:
			_confirmation_dialog->queue_free();
			_confirmation_dialog = nullptr;

			_open_scene_dialog->queue_free();
			_open_scene_dialog = nullptr;

			// 从库中注销，我们不想留下悬空指针
			set_library(Ref<VoxelInstanceLibrary>());
			break;

		case NOTIFICATION_PROCESS: {
			// 用轮询检测差异，而不是信号
			const int item_list_count = _item_list->get_item_count();
			if (_library.is_null()) {
				if (item_list_count > 0) {
					update_list_from_library();
				}
			} else {
				const int lib_item_count = _library->get_item_count();
				if (item_list_count != lib_item_count) {
					update_list_from_library();
				} else {
					VOXEL_ASSERT_RETURN(static_cast<int>(_name_cache.size()) == item_list_count);
					for (int i = 0; i < item_list_count; ++i) {
						const int id = _item_list->get_item_metadata(i);
						Ref<VoxelInstanceLibraryItem> item = _library->get_item(id);
						if (item.is_null()) {
							// 有东西失去同步了？
							update_list_from_library();
							break;
						}
						const String item_name = item->get_name();
						if (_name_cache[i] != item_name) {
							update_list_from_library();
							break;
						}
					}
				}
			}
		} break;

		default:
			break;
	}
}

void VoxelInstanceLibraryListEditor::on_list_item_selected(int index) {
	VOXEL_ASSERT_RETURN(_library.is_valid());
	const int item_id = _item_list->get_item_metadata(index);
	_library->set_selected_item_id(item_id);
}

void VoxelInstanceLibraryListEditor::on_button_pressed(int button_id) {
	_last_used_button = static_cast<ButtonID>(button_id);

	switch (button_id) {
		case BUTTON_ADD_MULTIMESH_ITEM:
			add_multimesh_item();
			break;

		case BUTTON_ADD_SCENE_ITEM:
			voxel::godot::popup_file_dialog(*_open_scene_dialog);
			break;

		case BUTTON_REMOVE_ITEM: {
			ERR_FAIL_COND(_library.is_null());
			const PackedInt32Array selected_items = _item_list->get_selected_items();
			if (selected_items.size() > 0) {
				const int ui_index = selected_items[0];
				const int item_id = _item_list->get_item_metadata(ui_index);
				_item_id_to_remove = item_id;
				_confirmation_dialog->set_text(VOXEL_TTR("Remove item {0}?").format(varray(_item_id_to_remove)));
				_confirmation_dialog->popup_centered();
			}
		} break;

		default:
			ERR_PRINT("Unknown menu item");
			break;
	}
}

void VoxelInstanceLibraryListEditor::on_open_scene_dialog_file_selected(String fpath) {
	switch (_last_used_button) {
		case BUTTON_ADD_SCENE_ITEM:
			add_scene_item(fpath);
			break;

		default:
			ERR_PRINT("Invalid menu option");
			break;
	}
}

void VoxelInstanceLibraryListEditor::on_remove_item_button_pressed() {
	on_button_pressed(BUTTON_REMOVE_ITEM);
}

void VoxelInstanceLibraryListEditor::on_remove_item_confirmed() {
	ERR_FAIL_COND(_library.is_null());
	ERR_FAIL_COND(_item_id_to_remove == -1);

	Ref<VoxelInstanceLibraryItem> item = _library->get_item(_item_id_to_remove);

	EditorUndoRedoManager &ur = _plugin->get_undo_redo2();
	ur.create_action("Remove item");
	ur.add_do_method(*_library, "remove_item", _item_id_to_remove);
	ur.add_undo_method(*_library, "add_item", _item_id_to_remove, item);
	ur.commit_action();

	_item_id_to_remove = -1;
}

// void VoxelInstanceLibraryListEditor::on_library_item_changed(
// 	int id, IInstanceLibraryItemListener::ChangeType change) {
// 	switch (change) {
// 		case IInstanceLibraryItemListener::CHANGE_ADDED:
// 		case IInstanceLibraryItemListener::CHANGE_REMOVED:
// 			update_list_from_library();
// 			break;
// 		case IInstanceLibraryItemListener::CHANGE_LOD_INDEX:
// 		case IInstanceLibraryItemListener::CHANGE_GENERATOR:
// 		case IInstanceLibraryItemListener::CHANGE_VISUAL:
// 		case IInstanceLibraryItemListener::CHANGE_SCENE:
// 			break;
// 		default:
// 			VOXEL_PRINT_ERROR("Unhandled change type");
// 			break;
// 	}
// }

void VoxelInstanceLibraryListEditor::add_multimesh_item() {
	ERR_FAIL_COND(_library.is_null());

	Ref<VoxelInstanceLibraryMultiMeshItem> item;
	item.instantiate();
	// 设置一些默认值
	Ref<BoxMesh> mesh;
	mesh.instantiate();
	item->set_mesh(mesh, 0);

	// 如果能够检测到使用该库的实例化器所在的地形是否带有 LOD，我们本可以在这里决定使用不同的默认值。
	// 至少在没有 LOD 支持时它应该始终是 0，否则看起来会坏掉。0 是默认值。
	// item->set_lod_index(2);

	Ref<VoxelInstanceGenerator> generator;
	generator.instantiate();
	item->set_generator(generator);

	const int item_id = _library->get_next_available_id();

	EditorUndoRedoManager &ur = _plugin->get_undo_redo2();
	ur.create_action("Add multimesh item");
	ur.add_do_method(*_library, "add_item", item_id, item);
	ur.add_undo_method(*_library, "remove_item", item_id);
	ur.commit_action();

	// update_list_from_library();
}

void VoxelInstanceLibraryListEditor::add_scene_item(String fpath) {
	ERR_FAIL_COND(_library.is_null());

	Ref<PackedScene> scene = godot::load_resource(fpath);
	ERR_FAIL_COND(scene.is_null());

	Ref<VoxelInstanceLibrarySceneItem> item;
	item.instantiate();
	// 设置一些默认值
	item->set_lod_index(2);
	item->set_scene(scene);
	Ref<VoxelInstanceGenerator> generator;
	generator.instantiate();
	generator->set_density(0.01f); // 场景的密度较低，因为它更重
	item->set_generator(generator);

	const int item_id = _library->get_next_available_id();

	EditorUndoRedoManager &ur = _plugin->get_undo_redo2();
	ur.create_action("Add scene item");
	ur.add_do_method(_library.ptr(), "add_item", item_id, item);
	ur.add_undo_method(_library.ptr(), "remove_item", item_id);
	ur.commit_action();

	// update_list_from_library();
}

void VoxelInstanceLibraryListEditor::update_list_from_library() {
	_item_list->clear();
	_name_cache.clear();

	if (_library.is_null()) {
		return;
	}

	ItemList *item_list = _item_list;
	StdVector<String> &name_cache = _name_cache;

	_library->for_each_item([item_list, &name_cache](const int item_id, const VoxelInstanceLibraryItem &item) {
		const String item_name = item.get_name();
		const int ui_index = item_list->add_item(String("[{0}] - {1}").format(varray(item_id, item_name)));
		item_list->set_item_metadata(ui_index, item_id);
		name_cache.push_back(item_name);
	});
}

void VoxelInstanceLibraryListEditor::_bind_methods() {
	// ClassDB::bind_method(
	// 		D_METHOD("update_list_from_library"), &VoxelInstanceLibraryListEditor::update_list_from_library
	// );
}

} // namespace voxel
