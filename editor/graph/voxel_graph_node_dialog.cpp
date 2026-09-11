#include "voxel_graph_node_dialog.h"
#include "../../constants/voxel_string_names.h"
#include "../../generators/graph/node_type_db.h"
#include "../../util/containers/std_unordered_set.h"
#include "../../util/godot/classes/button.h"
#include <servers/display/display_server.h>
#include "../../util/godot/classes/editor_file_dialog.h"
#include "../../util/godot/classes/editor_quick_open.h"
#include <editor/editor_node.h>
#include <scene/resources/font.h>
#include "../../util/godot/classes/input_event_key.h"
#include <scene/gui/label.h>
#include <scene/gui/line_edit.h>
#include "../../util/godot/classes/object.h"
#include <scene/gui/rich_text_label.h>
#include <scene/gui/tree.h>
#include "../../util/godot/classes/tree_item.h"
#include <scene/gui/box_container.h>
#include <scene/gui/split_container.h>
#include <core/variant/array.h>
#include "../../util/godot/core/keyboard.h"
#include "../../util/godot/core/string.h"
#include <core/version.h>
#include <editor/themes/editor_scale.h>
#include "graph_nodes_doc_data.h"
#include <servers/display/display_server.h>
#include <scene/resources/font.h>
#include <scene/gui/label.h>
#include <scene/gui/line_edit.h>
#include <scene/gui/rich_text_label.h>
#include <scene/gui/tree.h>
#include <scene/gui/box_container.h>
#include <scene/gui/split_container.h>
#include <core/variant/array.h>
#include <editor/themes/editor_scale.h>
#include <core/version.h>
#include <core/object/callable_mp.h>





namespace voxel {

namespace {

const GraphNodesDocData::Node *get_graph_node_documentation(String name) {
	for (unsigned int i = 0; i < GraphNodesDocData::COUNT; ++i) {
		const GraphNodesDocData::Node &node = GraphNodesDocData::g_data[i];
		if (node.name == name) {
			return &node;
		}
	}
	return nullptr;
}

void get_graph_node_documentation_category_names(StdVector<String> &out_category_names) {
	StdUnorderedSet<String> categories;
	for (unsigned int i = 0; i < GraphNodesDocData::COUNT; ++i) {
		const GraphNodesDocData::Node &node = GraphNodesDocData::g_data[i];
		if (categories.insert(node.category).second) {
			out_category_names.push_back(node.category);
		}
	}
}

// 这是 `Tree::_up` 的简化版重新实现，因为这些功能没有暴露...
void select_up(Tree &tree) {
	TreeItem *selected_item = tree.get_selected();

	if (selected_item == nullptr) {
		VOXEL_PRINT_VERBOSE("No item selected in tree, can't select down");
		return;
	}

	TreeItem *prev = selected_item->get_prev_visible();

	const int col = 0;
	while (prev != nullptr && !prev->is_selectable(col)) {
		prev = prev->get_prev_visible();
	}
	if (prev == nullptr) {
		return;
	}

	prev->select(col);

	tree.ensure_cursor_is_visible();
	// tree.accept_event();
}

// 这是 `Tree::_down` 的简化版重新实现，因为这些功能没有暴露...
void select_down(Tree &tree) {
	TreeItem *selected_item = tree.get_selected();

	if (selected_item == nullptr) {
		VOXEL_PRINT_VERBOSE("No item selected in tree, can't select down");
		return;
	}

	TreeItem *next = selected_item->get_next_visible();

	const int col = 0;

	while (next != nullptr && !next->is_selectable(col)) {
		next = next->get_next_visible();
	}
	if (next == nullptr) {
		return;
	}

	next->select(col);

	tree.ensure_cursor_is_visible();
	// tree.accept_event();
}

} // namespace

const char *VoxelGraphNodeDialog::SIGNAL_NODE_SELECTED = "node_selected";
const char *VoxelGraphNodeDialog::SIGNAL_FILE_SELECTED = "file_selected";

VoxelGraphNodeDialog::VoxelGraphNodeDialog() {
	set_title(VOXEL_TTR("Create Graph Node"));
	set_exclusive(false);

	set_ok_button_text(VOXEL_TTR("Create"));
	get_ok_button()->connect("pressed", callable_mp(this, &VoxelGraphNodeDialog::on_ok_pressed));
	get_ok_button()->set_disabled(true);
	// connect("canceled", callable_mp(this, &VisualShaderEditor::_member_cancel));

	VBoxContainer *vb_container = memnew(VBoxContainer);
	vb_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	LineEdit *filter_line_edit = memnew(LineEdit);
	filter_line_edit->connect("text_changed", callable_mp(this, &VoxelGraphNodeDialog::on_filter_text_changed));
	filter_line_edit->connect("gui_input", callable_mp(this, &VoxelGraphNodeDialog::on_filter_gui_input));
	filter_line_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	filter_line_edit->set_placeholder(VOXEL_TTR("Search"));
	vb_container->add_child(filter_line_edit);
	_filter_line_edit = filter_line_edit;

	const float editor_scale = EDSCALE;

	VSplitContainer *vsplit_container = memnew(VSplitContainer);
	vsplit_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	Tree *tree = memnew(Tree);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tree->set_hide_root(true);
	tree->set_allow_reselect(true);
	tree->set_hide_folding(false);
	tree->set_custom_minimum_size(Size2(180 * editor_scale, 200 * editor_scale));
	tree->connect("item_activated", callable_mp(this, &VoxelGraphNodeDialog::on_tree_item_activated));
	tree->connect("item_selected", callable_mp(this, &VoxelGraphNodeDialog::on_tree_item_selected));
	tree->connect("nothing_selected", callable_mp(this, &VoxelGraphNodeDialog::on_tree_nothing_selected));
	vsplit_container->add_child(tree);
	_tree = tree;

	RichTextLabel *description_label = memnew(RichTextLabel);
	description_label->set_v_size_flags(Control::SIZE_FILL);
	description_label->set_custom_minimum_size(Size2(0, 70 * editor_scale));
	description_label->set_use_bbcode(true);
	description_label->connect(
			"meta_clicked", callable_mp(this, &VoxelGraphNodeDialog::on_description_label_meta_clicked)
	);
	vsplit_container->add_child(description_label);
	_description_label = description_label;

	vb_container->add_child(vsplit_container);

	add_child(vb_container);

	_function_file_dialog = memnew(EditorFileDialog);
	_function_file_dialog->set_access(EditorFileDialog::ACCESS_RESOURCES);
	_function_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
	// TODO 可用性：没有办法将文件对话框限制为特定 TYPE 的资源，只能限制文件扩展名。所以
	// 这没什么用，因为文本资源几乎都使用 `.tres`...
	_function_file_dialog->add_filter("*.tres", VOXEL_TTR("Text Resource"));
	_function_file_dialog->add_filter("*.res", VOXEL_TTR("Binary Resource"));
	_function_file_dialog->connect(
			"file_selected", callable_mp(this, &VoxelGraphNodeDialog::on_function_file_dialog_file_selected)
	);
	add_child(_function_file_dialog);

	// TODO 用直接在对话框中列出项目函数来取代 QuickOpen

	// 在这个编辑器中，分类来自文档，可能与内部节点分类无关。
	// 它们服务于不同的目的。
	get_graph_node_documentation_category_names(_category_names);
	{
		SortArray<String> sorter;
		sorter.sort(_category_names.data(), _category_names.size());
	}

	// TODO 可用性：基于 I/O 定义将 CustomInput 和 CustomOutput 作为子分类，并为
	// 未绑定的提供“新建”选项
	const pg::NodeTypeDB &type_db = pg::NodeTypeDB::get_singleton();
	for (int type_index = 0; type_index < type_db.get_type_count(); ++type_index) {
		const pg::NodeType &type = type_db.get_type(type_index);

		int category_index = -1;
		String description;

		const GraphNodesDocData::Node *doc = get_graph_node_documentation(type.name);
		if (doc != nullptr) {
			for (unsigned int i = 0; i < _category_names.size(); ++i) {
				if (_category_names[i] == doc->category) {
					category_index = i;
					break;
				}
			}
			description = doc->description;
		}

		if (type_index == pg::VoxelGraphFunction::NODE_FUNCTION) {
			{
				Item item;
				item.name = VOXEL_TTR("Browse Custom Function...");
				item.description = description;
				item.category = category_index;
				item.id = ID_FUNCTION_BROWSE;
				_items.push_back(item);
			}
			{
				Item item;
				item.name = VOXEL_TTR("Quick Open Custom Function...");
				item.description = description;
				item.category = category_index;
				item.id = ID_FUNCTION_QUICK_OPEN;
				_items.push_back(item);
			}
			continue;
		}

		const pg::NodeType &node_type = pg::NodeTypeDB::get_singleton().get_type(type_index);

		Item item;
		item.name = node_type.name;
		item.category = category_index;
		item.description = description;
		item.id = type_index;
		_items.push_back(item);
	}

	update_tree(false);
}

void VoxelGraphNodeDialog::popup_at_screen_position(Vector2 screen_pos) {
	// 与 VisualShaderEditor::_show_members_dialog 中的弹窗类似

	VoxelGraphNodeDialog &dialog = *this;

	dialog.set_position(screen_pos);

	dialog.popup();

	_filter_line_edit->call_deferred(VoxelStringNames::get_singleton().grab_focus); // 此时还不可见
	_filter_line_edit->select_all();

	// 保持在屏幕边界内。
	// 看来我们必须在显示窗口后做这件事，因为 Godot 无法在不先使其可见的情况下
	// 更新窗口大小……
	// TODO 我们不是应该检查屏幕大小而不是窗口大小吗？
	const Rect2 window_rect =
			Rect2(DisplayServer::get_singleton()->window_get_position(),
				  DisplayServer::get_singleton()->window_get_size());
	const Rect2 dialog_rect = Rect2(dialog.get_position(), get_size());
	const Vector2 difference = (dialog_rect.get_end() - window_rect.get_end()).max(Vector2());
	dialog.set_position(dialog.get_position() - difference);
}

void VoxelGraphNodeDialog::update_tree(bool autoselect) {
	_tree->clear();
	TreeItem *root = _tree->create_item();

	// 过滤条目

	const String filter = _filter_line_edit->get_text().strip_edges();
	const bool use_filter = !filter.is_empty();

	StdVector<unsigned int> filtered_items;

	for (unsigned int i = 0; i < _items.size(); ++i) {
		const Item &item = _items[i];
		if (!use_filter || item.name.findn(filter) != -1) {
			filtered_items.push_back(i);
		}
	}

	// 填充树

	StdVector<TreeItem *> category_tree_items;
	category_tree_items.resize(_category_names.size(), nullptr);

	bool autoselected = false;

	for (const unsigned int item_index : filtered_items) {
		const Item &item = _items[item_index];

		TreeItem *parent_tree_item = root;

		if (item.category != -1) {
			parent_tree_item = category_tree_items[item.category];

			if (parent_tree_item == nullptr) {
				parent_tree_item = _tree->create_item(root);
				parent_tree_item->set_text(0, _category_names[item.category]);
				parent_tree_item->set_selectable(0, false);
				parent_tree_item->set_collapsed(!use_filter);

				category_tree_items[item.category] = parent_tree_item;
			}
		}

		TreeItem *tree_item = _tree->create_item(parent_tree_item);

		voxel::godot::TreeItemUtilities::set_auto_translate_mode(
				*tree_item, 0, voxel::godot::AUTO_TRANSLATE_MODE_DISABLED
		);

		tree_item->set_text(0, item.name);

		if (autoselected == false && autoselect) {
			tree_item->select(0);
			autoselected = true;
		}

		tree_item->set_metadata(0, item.id);
	}
}

void VoxelGraphNodeDialog::on_filter_text_changed(String new_text) {
	update_tree(true);
}

void VoxelGraphNodeDialog::on_filter_gui_input(Ref<InputEvent> event) {
	Ref<InputEventKey> key_event = event;
	if (key_event.is_valid()) {
		// 不能直接调用 `gui_input()` 来转发事件，所以手动处理按键
		// （就像 `VisualShaderEditor::_sbox_input` 那样）。
		//
		// _tree->gui_input(key_event);

		if (key_event->is_pressed()) {
			switch (key_event->get_keycode()) {
				case ::godot::KEY_UP:
					select_up(*_tree);
					_filter_line_edit->accept_event();
					break;

				case ::godot::KEY_DOWN:
					select_down(*_tree);
					_filter_line_edit->accept_event();
					break;

				case ::godot::KEY_ENTER:
					on_tree_item_activated();
					break;

				default:
					break;
			}
		}
	}
}

void VoxelGraphNodeDialog::on_tree_item_activated() {
	const TreeItem *item = _tree->get_selected();
	if (item == nullptr) {
		return;
	}
	const int id = item->get_metadata(0);
	VOXEL_ASSERT_RETURN(id >= 0);

	if (id < pg::VoxelGraphFunction::NODE_TYPE_COUNT) {
		// 节点被选中
		emit_signal(SIGNAL_NODE_SELECTED, id);
		hide();

	} else if (id == ID_FUNCTION_BROWSE) {
		// 浏览函数节点
		voxel::godot::popup_file_dialog(*_function_file_dialog);

	} else if (id == ID_FUNCTION_QUICK_OPEN) {
		Vector<StringName> base_types;
		base_types.append(pg::VoxelGraphFunction::get_class_static());
		EditorQuickOpenDialog *quick_open_dialog = EditorNode::get_singleton()->get_quick_open_dialog();
		quick_open_dialog->popup_dialog(
				base_types, callable_mp(this, &VoxelGraphNodeDialog::on_function_quick_open_dialog_item_selected)
		);

	} else {
		WARN_PRINT(String("Unknown ID {} picked in {}").format(varray(id, get_class())));
	}
}

void VoxelGraphNodeDialog::on_tree_item_selected() {
	const TreeItem *tree_item = _tree->get_selected();
	if (tree_item == nullptr) {
		get_ok_button()->set_disabled(true);
		return;
	}
	const int id = tree_item->get_metadata(0);
	VOXEL_ASSERT_RETURN(id >= 0);

	const Item *item = nullptr;
	for (const Item &i : _items) {
		if (i.id == id) {
			item = &i;
			break;
		}
	}
	VOXEL_ASSERT_RETURN(item != nullptr);

	_description_label->set_text(item->description);

	if (id < pg::VoxelGraphFunction::NODE_TYPE_COUNT) {
		get_ok_button()->set_disabled(false);

	} else {
		get_ok_button()->set_disabled(true);
	}
}

void VoxelGraphNodeDialog::on_tree_nothing_selected() {}

void VoxelGraphNodeDialog::on_ok_pressed() {
	on_tree_item_activated();
	// hide();
}

void VoxelGraphNodeDialog::on_function_file_dialog_file_selected(String fpath) {
	emit_signal(SIGNAL_FILE_SELECTED, fpath);
	hide();
}


void VoxelGraphNodeDialog::on_function_quick_open_dialog_item_selected(String fpath) {
	if (fpath.is_empty()) {
		return;
	}
	emit_signal(SIGNAL_FILE_SELECTED, fpath);
	hide();
}

void VoxelGraphNodeDialog::on_description_label_meta_clicked(Variant meta) {
	// TODO 点击类名时打开文档
}

void VoxelGraphNodeDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		_filter_line_edit->set_clear_button_enabled(true);

	} else if (p_what == NOTIFICATION_THEME_CHANGED) {
		const VoxelStringNames &sn = VoxelStringNames::get_singleton();
		_filter_line_edit->set_right_icon(get_theme_icon(sn.Search, sn.EditorIcons));

		const Ref<Font> mono_font = get_theme_font(sn.source, sn.EditorFonts);
		if (mono_font.is_valid()) {
			_description_label->add_theme_font_override("mono_font", mono_font);
		}
	}
}

void VoxelGraphNodeDialog::_bind_methods() {
	ADD_SIGNAL(MethodInfo(SIGNAL_NODE_SELECTED, PropertyInfo(Variant::INT, "node_type_id")));
	ADD_SIGNAL(MethodInfo(SIGNAL_FILE_SELECTED, PropertyInfo(Variant::STRING, "file_path")));
}

} // namespace voxel
