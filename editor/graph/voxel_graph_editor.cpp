#include "voxel_graph_editor.h"
#include "../../constants/voxel_string_names.h"
#include "../../generators/graph/node_type_db.h"
#include "../../generators/graph/voxel_generator_graph.h"
#include "../../terrain/voxel_node.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/button.h"
#include <scene/main/canvas_item.h>
#include <scene/gui/check_box.h>
#include <scene/gui/control.h>
#include "../../util/godot/classes/editor_file_dialog.h"
#include "../../util/godot/classes/editor_quick_open.h"
#include "../../util/godot/classes/graph_edit.h"
#include <scene/gui/box_container.h>
#include <core/input/input_event.h>
#include <core/input/input_event.h>
#include <scene/gui/label.h>
#include <scene/gui/menu_button.h>
#include "../../util/godot/classes/node.h"
#include <scene/gui/option_button.h>
#include <scene/gui/popup_menu.h>
#include "../../util/godot/classes/resource_loader.h"
#include <scene/main/scene_tree.h>
#include <core/os/time.h>
#include <core/version.h>
#include <scene/resources/3d/world_3d.h>
#include <core/variant/array.h>
#include "../../util/godot/core/input_enums.h"
#include "../../util/godot/core/mouse_button.h"
#include <core/version.h>
#include <editor/themes/editor_scale.h>
#include "../../util/io/log.h"
#include "../../util/macros.h"
#include "../../util/math/conv.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"
#include "graph_editor_adapter.h"
#include "voxel_graph_editor_node.h"
#include "voxel_graph_editor_node_preview.h"
#include "voxel_graph_editor_shader_dialog.h"
#include "voxel_graph_node_dialog.h"
#include "voxel_range_analysis_dialog.h"
#include <scene/main/canvas_item.h>
#include <scene/gui/check_box.h>
#include <scene/gui/control.h>
#include <scene/gui/box_container.h>
#include <core/input/input_event.h>
#include <core/input/input_event.h>
#include <scene/gui/label.h>
#include <scene/gui/menu_button.h>
#include <scene/gui/option_button.h>
#include <scene/gui/popup_menu.h>
#include <scene/main/scene_tree.h>
#include <core/os/time.h>
#include <scene/resources/3d/world_3d.h>
#include <core/variant/array.h>
#include <editor/themes/editor_scale.h>
#include <core/version.h>
#include <core/object/callable_mp.h>


namespace voxel {

using namespace pg;
using namespace voxel::godot;

const char *VoxelGraphEditor::SIGNAL_NODE_SELECTED = "node_selected";
const char *VoxelGraphEditor::SIGNAL_NOTHING_SELECTED = "nothing_selected";
const char *VoxelGraphEditor::SIGNAL_NODES_DELETED = "nodes_deleted";
const char *VoxelGraphEditor::SIGNAL_REGENERATE_REQUESTED = "regenerate_requested";
const char *VoxelGraphEditor::SIGNAL_POPOUT_REQUESTED = "popout_requested";

enum ToolbarMenuIDs {
	MENU_UPDATE_PREVIEWS = 0,
	MENU_PROFILE,
	MENU_ANALYZE_RANGE,
	MENU_LIVE_UPDATE,
	MENU_PREVIEW_AXES,
	MENU_PREVIEW_AXES_XY,
	MENU_PREVIEW_AXES_XZ,
	MENU_PREVIEW_RESET_LOCATION,
	MENU_GENERATE_SHADER,

	MENU_ADD_NODE,
	MENU_REMOVE_SELECTED_NODES,
	MENU_REMOVE_CONNECTION
};

// 工具函数
namespace {

NodePath to_node_path(const StringName &sn) {
	return NodePath(String(sn));
}

bool is_nothing_selected(GraphEdit *graph_edit) {
	for (int i = 0; i < graph_edit->get_child_count(); ++i) {
		GraphNode *node = Object::cast_to<GraphNode>(graph_edit->get_child(i));
		if (node != nullptr && node->is_selected()) {
			return false;
		}
	}
	return true;
}

void update_menu_radio_checkable_items(PopupMenu &menu, int checked_id) {
	for (int i = 0; i < menu.get_item_count(); ++i) {
		if (menu.is_item_radio_checkable(i)) {
			const int item_id = menu.get_item_id(i);
			menu.set_item_checked(i, item_id == checked_id);
		}
	}
}

} // namespace

VoxelGraphEditor::VoxelGraphEditor() {
	using Self = VoxelGraphEditor;

	VBoxContainer *vbox_container = memnew(VBoxContainer);
	vbox_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);

	{
		HBoxContainer *toolbar = memnew(HBoxContainer);

		{
			MenuButton *menu_button = memnew(MenuButton);
			menu_button->set_text(VOXEL_TTR("Graph"));
			menu_button->set_switch_on_hover(true);

			PopupMenu *popup_menu = menu_button->get_popup();
#ifdef VOXEL_ENABLE_GPU
			popup_menu->add_item(VOXEL_TTR("Generate Shader"), MENU_GENERATE_SHADER);
#endif

			popup_menu->connect("id_pressed", callable_mp(this, &Self::_on_menu_id_pressed));

			toolbar->add_child(menu_button);
			_graph_menu_button = menu_button;
		}
		{
			MenuButton *menu_button = memnew(MenuButton);
			menu_button->set_text(VOXEL_TTR("Debug"));
			menu_button->set_switch_on_hover(true);

			PopupMenu *popup_menu = menu_button->get_popup();
			popup_menu->add_item(VOXEL_TTR("Update Previews"), MENU_UPDATE_PREVIEWS);
			popup_menu->add_item(VOXEL_TTR("Profile"), MENU_PROFILE);
			popup_menu->add_item(VOXEL_TTR("Analyze Range..."), MENU_ANALYZE_RANGE);

			{
				const int idx = popup_menu->get_item_count();
				popup_menu->add_check_item(VOXEL_TTR("Live Update"), MENU_LIVE_UPDATE);
				popup_menu->set_item_tooltip(
						idx, VOXEL_TTR("Automatically re-generate the terrain when the generator is modified")
				);
				popup_menu->set_item_checked(idx, _live_update_enabled);
			}

			{
				PopupMenu *sub_menu = memnew(PopupMenu);
				sub_menu->set_name("PreviewAxisMenu");
				sub_menu->add_radio_check_item("XY", MENU_PREVIEW_AXES_XY);
				sub_menu->add_radio_check_item("XZ", MENU_PREVIEW_AXES_XZ);
				sub_menu->add_item("Reset location", MENU_PREVIEW_RESET_LOCATION);
				sub_menu->connect("id_pressed", callable_mp(this, &Self::_on_menu_id_pressed));
				popup_menu->add_child(sub_menu);
				popup_menu->add_submenu_item(VOXEL_TTR("Preview Axes"), sub_menu->get_name(), MENU_PREVIEW_AXES);
				_preview_axes_menu = sub_menu;
				update_preview_axes_menu();
			}

			popup_menu->connect("id_pressed", callable_mp(this, &Self::_on_menu_id_pressed));

			toolbar->add_child(menu_button);
			_debug_menu_button = menu_button;
		}

		_profile_label = memnew(Label);
		toolbar->add_child(_profile_label);

		_compile_result_label = memnew(Label);
		_compile_result_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		_compile_result_label->set_clip_text(true);
		_compile_result_label->hide();
		toolbar->add_child(_compile_result_label);

		_no_graph_open_label = memnew(Label);
		_no_graph_open_label->set_text("[No graph open]");
		_no_graph_open_label->set_modulate(Color(1, 1, 0));
		toolbar->add_child(_no_graph_open_label);

		Control *spacer = memnew(Control);
		spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		toolbar->add_child(spacer);

		_pin_button = memnew(Button);
		_pin_button->set_flat(true);
		_pin_button->set_toggle_mode(true);
		_pin_button->set_tooltip_text(VOXEL_TTR("Keep visible regardless of selection"));
		toolbar->add_child(_pin_button);

		_popout_button = memnew(Button);
		_popout_button->set_flat(true);
		_popout_button->set_tooltip_text(VOXEL_TTR("Pop-out as separate window"));
		_popout_button->connect("pressed", callable_mp(this, &Self::_on_popout_button_pressed));
		toolbar->add_child(_popout_button);

		vbox_container->add_child(toolbar);
	}

	_graph_edit = memnew(GraphEdit);
	_graph_edit->set_anchors_preset(Control::PRESET_FULL_RECT);
	_graph_edit->set_right_disconnects(true);
	// TODO 性能：抱歉，不得不关闭抗锯齿，因为 Godot 当前的实现慢得惊人。
	// 当图形有很多连线时会严重拖慢编辑器。因为尽管 Godot 4 现在支持
	// 2D MSAA，它仍然依赖一种假的抗锯齿方法，会生成更多几何图形并即时
	// 分配内存（malloc）。参见 `RendererCanvasCull::canvas_item_add_polyline`。
	// 2D MSAA 也只在项目设置中暴露，不适用于编辑器 UI……（也不应该，但
	// 应该在编辑器设置中提供该项设置）。
	_graph_edit->set_connection_lines_antialiased(false);
	_graph_edit->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	_graph_edit->connect("gui_input", callable_mp(this, &Self::_on_graph_edit_gui_input));
	_graph_edit->connect("connection_request", callable_mp(this, &Self::_on_graph_edit_connection_request));
	_graph_edit->connect("delete_nodes_request", callable_mp(this, &Self::_on_graph_edit_delete_nodes_request));
	_graph_edit->connect("disconnection_request", callable_mp(this, &Self::_on_graph_edit_disconnection_request));
	_graph_edit->connect("node_selected", callable_mp(this, &Self::_on_graph_edit_node_selected));
	_graph_edit->connect("node_deselected", callable_mp(this, &Self::_on_graph_edit_node_deselected));
	_graph_edit->connect("copy_nodes_request", callable_mp(this, &Self::_on_graph_edit_copy_nodes_request));
	_graph_edit->connect("paste_nodes_request", callable_mp(this, &Self::_on_graph_edit_paste_nodes_request));
	vbox_container->add_child(_graph_edit);

	add_child(vbox_container);

	_node_dialog = memnew(VoxelGraphNodeDialog);
	_node_dialog->connect(
			VoxelGraphNodeDialog::SIGNAL_NODE_SELECTED, callable_mp(this, &Self::_on_node_dialog_node_selected)
	);
	_node_dialog->connect(
			VoxelGraphNodeDialog::SIGNAL_FILE_SELECTED, callable_mp(this, &Self::_on_node_dialog_file_selected)
	);
	// 最初这个弹窗被设计为不在任务栏中显示为独立窗口，并在点击
	// 外部时关闭。后来后者（点击外部关闭）不再生效（？），导致了与 VisualShader
	// 编辑器相同的问题：
	// https://github.com/godotengine/godot/issues/83805
	// 因此目前我们采用相同的修复方式，将其设为独占...
	_node_dialog->set_exclusive(true);
	add_child(_node_dialog);

	_context_menu = memnew(PopupMenu);
	_context_menu->connect("id_pressed", callable_mp(this, &Self::_on_menu_id_pressed));
	_context_menu->hide();
	add_child(_context_menu);

	_range_analysis_dialog = memnew(VoxelRangeAnalysisDialog);
	_range_analysis_dialog->connect("analysis_toggled", callable_mp(this, &Self::_on_range_analysis_toggled));
	_range_analysis_dialog->connect("area_changed", callable_mp(this, &Self::_on_range_analysis_area_changed));
	add_child(_range_analysis_dialog);

	_shader_dialog = memnew(VoxelGraphEditorShaderDialog);
	add_child(_shader_dialog);

	update_buttons_availability();
}

void VoxelGraphEditor::set_generator(Ref<VoxelGeneratorGraph> generator) {
	if (_generator == generator) {
		return;
	}

	_generator = generator;

	if (_generator.is_valid()) {
		Ref<VoxelGraphFunction> graph = generator->get_main_function();

		// 创建新图形时加载默认预设。
		// 缺点是空的图形无法显示。但 Godot 不会告诉我们资源是从检查器
		// 创建的还是不是，因此我们不得不引入一个特殊布尔值...
		if (graph->get_nodes_count() == 0 && graph->can_load_default_graph()) {
			_generator->load_plane_preset();
		}

		set_graph(graph);

	} else {
		set_graph(Ref<VoxelGraphFunction>());
	}

	schedule_preview_update();

	update_buttons_availability();
}

void VoxelGraphEditor::set_graph(Ref<VoxelGraphFunction> graph) {
	using Self = VoxelGraphEditor;

	if (_graph == graph) {
		return;
	}

	if (_graph.is_valid()) {
		_graph->disconnect(VoxelStringNames::get_singleton().changed, callable_mp(this, &Self::_on_graph_changed));
		_graph->disconnect(
				VoxelGeneratorGraph::SIGNAL_NODE_NAME_CHANGED, callable_mp(this, &Self::_on_graph_node_name_changed)
		);
	}

	_graph = graph;

	if (_graph.is_valid()) {
		_graph->connect(VoxelStringNames::get_singleton().changed, callable_mp(this, &Self::_on_graph_changed));
		_graph->connect(
				VoxelGeneratorGraph::SIGNAL_NODE_NAME_CHANGED, callable_mp(this, &Self::_on_graph_node_name_changed)
		);

	} else {
		_graph = Ref<VoxelGraphFunction>();
	}

	_debug_renderer.clear();

	build_gui_from_graph();

	if (_graph.is_valid()) {
		update_functions();
	}

	_no_graph_open_label->set_visible(!_graph.is_valid());

	// schedule_preview_update();
}

Ref<VoxelGraphFunction> VoxelGraphEditor::get_graph() const {
	return _graph;
}

void VoxelGraphEditor::set_undo_redo(EditorUndoRedoManager *undo_redo) {
	_undo_redo = undo_redo;
}

EditorUndoRedoManager *VoxelGraphEditor::get_undo_redo() const {
	return _undo_redo;
}

void VoxelGraphEditor::set_voxel_node(VoxelNode *node) {
	_terrain_node.set(node);
	if (node == nullptr) {
		VOXEL_PRINT_VERBOSE("Reference node for VoxelGraph gizmos: null");
		_debug_renderer.set_world(nullptr);
	} else {
		VOXEL_PRINT_VERBOSE(format("Reference node for VoxelGraph gizmos: {}", String(node->get_path())));
		_debug_renderer.set_world(node->get_world_3d().ptr());
		update_range_analysis_gizmo();
	}
}


void VoxelGraphEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS:
			process(get_process_delta_time());
			break;

		case NOTIFICATION_VISIBILITY_CHANGED:
			set_process_internal(is_visible());
			break;

		case NOTIFICATION_THEME_CHANGED: {
			const VoxelStringNames &sn = VoxelStringNames::get_singleton();
			set_button_icon(*_pin_button, get_theme_icon(sn.Pin, sn.EditorIcons));
			set_button_icon(*_popout_button, get_theme_icon(sn.ExternalLink, sn.EditorIcons));
		} break;
	}
}

void VoxelGraphEditor::process(float delta) {
	if (_time_before_preview_update > 0.f) {
		_time_before_preview_update -= delta;
		if (_time_before_preview_update < 0.f) {
			if (_graph.is_valid()) {
				update_previews(true);
			}
		}
	}

	// 我决定用轮询来在图形节点上显示一些内容，这样所有代码都在这里，无需
	// 折腾各种信号。
	if (_graph.is_valid() && is_visible_in_tree()) {
		for (int child_node_index = 0; child_node_index < _graph_edit->get_child_count(); ++child_node_index) {
			Node *node = _graph_edit->get_child(child_node_index);
			VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(node);
			if (node_view != nullptr) {
				node_view->poll(**_graph);
			}
		}
	}
}

void VoxelGraphEditor::clear() {
	_graph_edit->clear_connections();
	for (int i = 0; i < _graph_edit->get_child_count(); ++i) {
		Node *node = _graph_edit->get_child(i);
		GraphNode *node_view = Object::cast_to<GraphNode>(node);
		if (node_view != nullptr) {
			memdelete(node_view);
			--i;
		}
	}
	_profile_label->set_text("");
	_compile_result_label->hide();
}

inline String node_to_gui_name(uint32_t node_id) {
	return String("{0}").format(varray(node_id));
}

void VoxelGraphEditor::build_gui_from_graph() {
	// 重建整个图形 GUI

	clear();

	if (_graph.is_null()) {
		return;
	}

	const VoxelGraphFunction &graph = **_graph;

	// Nodes

	PackedInt32Array node_ids = graph.get_node_ids();
	for (int i = 0; i < node_ids.size(); ++i) {
		const uint32_t node_id = node_ids[i];
		create_node_gui(node_id);
	}

	// 连接

	StdVector<ProgramGraph::Connection> all_connections;
	graph.get_connections(all_connections);

	for (size_t i = 0; i < all_connections.size(); ++i) {
		const ProgramGraph::Connection &con = all_connections[i];
		const String from_node_name = node_to_gui_name(con.src.node_id);
		const String to_node_name = node_to_gui_name(con.dst.node_id);
		VoxelGraphEditorNode *to_node_view = get_node_typed<VoxelGraphEditorNode>(*_graph_edit, NodePath(to_node_name));
		ERR_FAIL_COND(to_node_view == nullptr);
		const Error err = _graph_edit->connect_node(
				from_node_name, con.src.port_index, to_node_view->get_name(), con.dst.port_index
		);

		ERR_FAIL_COND(err != OK);
	}
}

void VoxelGraphEditor::create_node_gui(uint32_t node_id) {
	// 构建一个 GUI 节点

	CRASH_COND(_graph.is_null());
	// 检查是因为创建新节点时，即使其中一个失败，UndoRedo 方法也会继续执行
	ERR_FAIL_COND(_graph->has_node(node_id) == false);

	const String ui_node_name = node_to_gui_name(node_id);
	ERR_FAIL_COND(_graph_edit->has_node(ui_node_name));

	VoxelGraphEditorNode *node_view = VoxelGraphEditorNode::create(**_graph, node_id);
	node_view->set_name(ui_node_name);

	node_view->connect("dragged", callable_mp(this, &VoxelGraphEditor::_on_graph_node_dragged).bind(node_id));
	node_view->connect("resize_request", callable_mp(this, &VoxelGraphEditor::_on_node_resize_request).bind(node_id));

	VoxelGraphEditorNodePreview *preview = node_view->get_preview();
	if (preview != nullptr) {
		preview->update_display_settings(**_graph, node_id);

		preview->connect("gui_input", callable_mp(this, &VoxelGraphEditor::_on_graph_node_preview_gui_input));
	}

	_graph_edit->add_child(node_view);
}

void remove_connections_from_and_to(GraphEdit &graph_edit, StringName node_name) {
	// 获取连接列表的副本
	StdVector<GraphEditConnection> connections;
	get_graph_edit_connections(graph_edit, connections);

	for (const GraphEditConnection &con : connections) {
		if (con.from == node_name || con.to == node_name) {
			graph_edit.disconnect_node(con.from, con.from_port, con.to, con.to_port);
		}
	}
}

void VoxelGraphEditor::remove_node_gui(StringName gui_node_name) {
	// 从 UI 中移除连接，因为 GraphNode 不会自动处理...
	remove_connections_from_and_to(*_graph_edit, gui_node_name);

	Node *node_view = get_node_typed<Node>(*_graph_edit, to_node_path(gui_node_name));
	ERR_FAIL_COND(Object::cast_to<GraphNode>(node_view) == nullptr);
	memdelete(node_view);
}

// 没有用于此目的的 API（也没有内部函数），因此我像实现那样复制粘贴了它
// static const GraphNode *get_graph_node_under_mouse(const GraphEdit *graph_edit) {
// 	for (int i = graph_edit->get_child_count() - 1; i >= 0; i--) {
// 		const GraphNode *gn = Object::cast_to<GraphNode>(graph_edit->get_child(i));
// 		if (gn != nullptr) {
// 			Rect2 r = gn->get_rect();
// 			r.size *= graph_edit->get_zoom();
// 			if (r.has_point(graph_edit->get_local_mouse_position())) {
// 				return gn;
// 			}
// 		}
// 	}
// 	return nullptr;
// }

void VoxelGraphEditor::update_node_layout(uint32_t node_id) {
	ERR_FAIL_COND(_graph.is_null());

	GraphEdit &graph_edit = *_graph_edit;
	const String view_name = node_to_gui_name(node_id);
	VoxelGraphEditorNode *view = get_node_typed<VoxelGraphEditorNode>(graph_edit, view_name);
	ERR_FAIL_COND(view == nullptr);

	// 移除所有连接到该节点的 GUI 连接

	StdVector<GraphEditConnection> old_connections;
	get_graph_edit_connections(graph_edit, old_connections);

	for (const GraphEditConnection &con : old_connections) {
		const NodePath to = to_node_path(con.to);
		const VoxelGraphEditorNode *to_view = get_node_typed<VoxelGraphEditorNode>(graph_edit, to);
		if (to_view == nullptr) {
			continue;
		}
		if (to_view == view) {
			graph_edit.disconnect_node(con.from, con.from_port, con.to, con.to_port);
		}
	}

	// 更新节点布局

	view->update_layout(**_graph);

	// TODO 输出连接怎么办？
	// 目前假设表达式节点总是只有一个输出，因此可能没问题？

	// 通过读取图形重新添加连接

	// TODO 优化：图形存储了邻接表，我们可以利用它
	StdVector<ProgramGraph::Connection> all_connections;
	_graph->get_connections(all_connections);

	for (size_t i = 0; i < all_connections.size(); ++i) {
		const ProgramGraph::Connection &con = all_connections[i];

		if (con.dst.node_id == node_id) {
			graph_edit.connect_node(
					node_to_gui_name(con.src.node_id),
					con.src.port_index,
					node_to_gui_name(con.dst.node_id),
					con.dst.port_index
			);
		}
	}
}

void VoxelGraphEditor::update_node_comment(uint32_t node_id) {
	ERR_FAIL_COND(_graph.is_null());

	GraphEdit &graph_edit = *_graph_edit;
	const String view_name = node_to_gui_name(node_id);
	VoxelGraphEditorNode *view = get_node_typed<VoxelGraphEditorNode>(graph_edit, view_name);
	ERR_FAIL_COND(view == nullptr);

	view->update_comment_text(**_graph);
}

bool VoxelGraphEditor::is_pinned_hint() const {
	return _pin_button->is_pressed();
}

static void get_selected_nodes(const GraphEdit &graph_edit, StdVector<VoxelGraphEditorNode *> &out_nodes) {
	for (int i = 0; i < graph_edit.get_child_count(); ++i) {
		VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(graph_edit.get_child(i));
		if (node_view != nullptr) {
			if (node_view->is_selected()) {
				out_nodes.push_back(node_view);
			}
		}
	}
}

void VoxelGraphEditor::_on_graph_edit_gui_input(Ref<InputEvent> event) {
	Ref<InputEventMouseButton> mb = event;

	if (mb.is_valid()) {
		if (mb->is_pressed()) {
			if (mb->get_button_index() == ::godot::MOUSE_BUTTON_RIGHT) {
				_click_position = mb->get_position();

				StdVector<VoxelGraphEditorNode *> selected_nodes;
				get_selected_nodes(*_graph_edit, selected_nodes);

				const Vector2 menu_pos = _graph_edit->get_local_mouse_position();
				_context_connection = get_graph_edit_closest_connection_at_point(*_graph_edit, menu_pos);

				if (_context_connection.is_valid() || selected_nodes.size() > 0) {
					// 显示上下文菜单

					const Vector2 global_pos =
							_graph_edit->get_screen_position() + _graph_edit->get_local_mouse_position();

					_context_menu->clear();

					_context_menu->add_item(VOXEL_TTR("Add Node"), MENU_ADD_NODE);

					_context_menu->add_separator();

					if (selected_nodes.size() > 0) {
						const String delete_node_text =
								selected_nodes.size() > 1 ? VOXEL_TTR("Delete Nodes") : VOXEL_TTR("Delete Node");
						_context_menu->add_item(delete_node_text, MENU_REMOVE_SELECTED_NODES);
					}

					if (_context_connection.is_valid()) {
						_context_menu->add_item(VOXEL_TTR("Delete Connection"), MENU_REMOVE_CONNECTION);
					}

					_context_menu->set_position(global_pos);
					// VisualShaderEditor 用了这个，但它没有暴露出来。我不知道它有什么用途。
					// _context_menu->reset_size();
					_context_menu->popup();

				} else {
					// 注意位置的计算方式，一些用户有多个显示器，但操作系统处理它们的方式
					// 不同，要么是两个独立桌面，要么是一个扩展桌面。这会影响鼠标
					// 位置。我参考了 `filesystem_dock.cpp` 中上下文菜单的做法。
					_node_dialog->popup_at_screen_position(_graph_edit->get_screen_position() + mb->get_position());
				}
			}
		}
	}
}

void VoxelGraphEditor::_on_graph_edit_connection_request(
		String from_node_name,
		int from_slot,
		String to_node_name,
		int to_slot
) {
	VoxelGraphEditorNode *src_node_view = get_node_typed<VoxelGraphEditorNode>(*_graph_edit, from_node_name);
	VoxelGraphEditorNode *dst_node_view = get_node_typed<VoxelGraphEditorNode>(*_graph_edit, to_node_name);
	ERR_FAIL_COND(src_node_view == nullptr);
	ERR_FAIL_COND(dst_node_view == nullptr);

	const uint32_t src_node_id = src_node_view->get_generator_node_id();
	const uint32_t dst_node_id = dst_node_view->get_generator_node_id();

	// print("Connection attempt from ", from, ":", from_slot, " to ", to, ":", to_slot)

	if (!_graph->is_valid_connection(src_node_id, from_slot, dst_node_id, to_slot)) {
		VOXEL_PRINT_VERBOSE("Connection is invalid");
		return;
	}

	_undo_redo->create_action(VOXEL_TTR("Connect Nodes"));

	ProgramGraph::PortLocation prev_src_port;
	String prev_src_node_name;
	const bool replacing =
			_graph->try_get_connection_to(ProgramGraph::PortLocation{ dst_node_id, uint32_t(to_slot) }, prev_src_port);

	if (replacing) {
		// 移除现有连接，以便用新连接替换
		prev_src_node_name = node_to_gui_name(prev_src_port.node_id);
		_undo_redo->add_do_method(
				_graph.ptr(), "remove_connection", prev_src_port.node_id, prev_src_port.port_index, dst_node_id, to_slot
		);
		_undo_redo->add_do_method(
				_graph_edit, "disconnect_node", prev_src_node_name, prev_src_port.port_index, to_node_name, to_slot
		);
	}

	_undo_redo->add_do_method(_graph.ptr(), "add_connection", src_node_id, from_slot, dst_node_id, to_slot);
	_undo_redo->add_do_method(_graph_edit, "connect_node", from_node_name, from_slot, to_node_name, to_slot);

	_undo_redo->add_undo_method(_graph.ptr(), "remove_connection", src_node_id, from_slot, dst_node_id, to_slot);
	_undo_redo->add_undo_method(_graph_edit, "disconnect_node", from_node_name, from_slot, to_node_name, to_slot);

	if (replacing) {
		// 撤销我们添加的连接后，恢复被我们替换掉的连接
		_undo_redo->add_undo_method(
				_graph.ptr(), "add_connection", prev_src_port.node_id, prev_src_port.port_index, dst_node_id, to_slot
		);
		_undo_redo->add_undo_method(
				_graph_edit, "connect_node", prev_src_node_name, prev_src_port.port_index, to_node_name, to_slot
		);
	}

	_undo_redo->commit_action();
}

void VoxelGraphEditor::_on_graph_edit_disconnection_request(
		String from_node_name,
		int from_slot,
		String to_node_name,
		int to_slot
) {
	remove_connection(from_node_name, from_slot, to_node_name, to_slot);
}

void VoxelGraphEditor::remove_connection(
		const String from_node_name,
		const int from_slot,
		const String to_node_name,
		const int to_slot
) {
	VoxelGraphEditorNode *src_node_view = get_node_typed<VoxelGraphEditorNode>(*_graph_edit, from_node_name);
	VoxelGraphEditorNode *dst_node_view = get_node_typed<VoxelGraphEditorNode>(*_graph_edit, to_node_name);
	ERR_FAIL_COND(src_node_view == nullptr);
	ERR_FAIL_COND(dst_node_view == nullptr);

	const uint32_t src_node_id = src_node_view->get_generator_node_id();
	const uint32_t dst_node_id = dst_node_view->get_generator_node_id();

	_undo_redo->create_action(VOXEL_TTR("Disconnect Nodes"));

	_undo_redo->add_do_method(_graph.ptr(), "remove_connection", src_node_id, from_slot, dst_node_id, to_slot);
	_undo_redo->add_do_method(_graph_edit, "disconnect_node", from_node_name, from_slot, to_node_name, to_slot);

	_undo_redo->add_undo_method(_graph.ptr(), "add_connection", src_node_id, from_slot, dst_node_id, to_slot);
	_undo_redo->add_undo_method(_graph_edit, "connect_node", from_node_name, from_slot, to_node_name, to_slot);

	_undo_redo->commit_action();
}

void VoxelGraphEditor::_on_graph_edit_delete_nodes_request(TypedArray<StringName> node_names) {
	// `node_names` 参数是 Godot issue #61112 的结果。虽然它比直接获取
	// 节点本身更方便，但它还有一个缺点：如果你选择不在每个图形节点的
	// 角落显示“关闭”按钮，即使你选中了节点，它也始终是空的。这个行为甚至还有文档。真服了。
	// 所以……我还是沿用老办法。
	delete_selected_nodes();
}

void VoxelGraphEditor::delete_selected_nodes() {
	StdVector<VoxelGraphEditorNode *> to_erase;
	get_selected_nodes(*_graph_edit, to_erase);

	if (to_erase.size() == 0) {
		return;
	}

	_undo_redo->create_action(VOXEL_TTR("Delete Nodes"));

	StdVector<ProgramGraph::Connection> all_connections;
	_graph->get_connections(all_connections);

	for (size_t i = 0; i < to_erase.size(); ++i) {
		const VoxelGraphEditorNode *node_view = to_erase[i];
		const uint32_t node_id = node_view->get_generator_node_id();
		const uint32_t node_type_id = _graph->get_node_type_id(node_id);

		_undo_redo->add_do_method(_graph.ptr(), "remove_node", node_id);
		_undo_redo->add_do_method(this, "remove_node_gui", node_view->get_name());

		if (node_type_id == VoxelGraphFunction::NODE_FUNCTION) {
			Ref<VoxelGraphFunction> func = _graph->get_node_param(node_id, 0);
			_undo_redo->add_undo_method(
					_graph.ptr(), "create_function_node", func, _graph->get_node_gui_position(node_id), node_id
			);
		} else {
			_undo_redo->add_undo_method(
					_graph.ptr(), "create_node", node_type_id, _graph->get_node_gui_position(node_id), node_id
			);
		}

		// 参数撤销
		const size_t param_count = NodeTypeDB::get_singleton().get_type(node_type_id).params.size();
		for (size_t j = 0; j < param_count; ++j) {
			Variant param_value = _graph->get_node_param(node_id, j);
			_undo_redo->add_undo_method(_graph.ptr(), "set_node_param", node_id, VOXEL_SIZE_T_TO_VARIANT(j), param_value);
		}

		_undo_redo->add_undo_method(this, "create_node_gui", node_id);

		// 连接撤销
		for (size_t j = 0; j < all_connections.size(); ++j) {
			const ProgramGraph::Connection &con = all_connections[j];

			if (con.src.node_id == node_id || con.dst.node_id == node_id) {
				_undo_redo->add_undo_method(
						_graph.ptr(),
						"add_connection",
						con.src.node_id,
						con.src.port_index,
						con.dst.node_id,
						con.dst.port_index
				);

				const String src_node_name = node_to_gui_name(con.src.node_id);
				const String dst_node_name = node_to_gui_name(con.dst.node_id);
				_undo_redo->add_undo_method(
						_graph_edit,
						"connect_node",
						src_node_name,
						con.src.port_index,
						dst_node_name,
						con.dst.port_index
				);
			}
		}
	}

	_undo_redo->commit_action();

	emit_signal(SIGNAL_NODES_DELETED);
}

void VoxelGraphEditor::_on_menu_id_pressed(int id) {
	switch (id) {
		case MENU_ANALYZE_RANGE:
			_range_analysis_dialog->popup_centered();
			break;

		case MENU_PREVIEW_AXES_XY:
			_node_preview_mode = GraphEditorPreview::VIEW_SLICE_XY;
			schedule_preview_update();
			update_preview_axes_menu();
			break;

		case MENU_PREVIEW_AXES_XZ:
			_node_preview_mode = GraphEditorPreview::VIEW_SLICE_XZ;
			schedule_preview_update();
			update_preview_axes_menu();
			break;

		case MENU_PREVIEW_RESET_LOCATION:
			set_preview_transform(Vector2f(0, 0), 1.f);
			break;

		case MENU_PROFILE:
			profile();
			break;

		case MENU_UPDATE_PREVIEWS:
			update_previews(false);
			break;

		case MENU_LIVE_UPDATE: {
			_live_update_enabled = !_live_update_enabled;
			PopupMenu *menu = _debug_menu_button->get_popup();
			const int idx = menu->get_item_index(id);
			menu->set_item_checked(idx, _live_update_enabled);
		} break;

		case MENU_ADD_NODE:
			_node_dialog->popup_at_screen_position(_graph_edit->get_screen_position() + _click_position);
			break;

		case MENU_REMOVE_SELECTED_NODES:
			delete_selected_nodes();
			break;

		case MENU_REMOVE_CONNECTION:
			if (_context_connection.is_valid()) {
				remove_connection(
						_context_connection.from,
						_context_connection.from_port,
						_context_connection.to,
						_context_connection.to_port
				);
			}
			break;

#ifdef VOXEL_ENABLE_GPU
		case MENU_GENERATE_SHADER: {
			ERR_FAIL_COND(_graph.is_null());
			pg::VoxelGraphFunction::ShaderResult shader_res = _graph->get_shader_source();
			if (!shader_res.compilation.success) {
				return;
			}
			// TODO 在哪个版本中包含 uniform？
			_shader_dialog->set_shader_code(to_godot(shader_res.code_utf8));
			_shader_dialog->popup_centered();
		} break;
#endif

		default:
			ERR_PRINT("Unknown menu item");
			break;
	}
}

void VoxelGraphEditor::_on_graph_node_dragged(Vector2 from, Vector2 to, int id) {
	// 注意，这实际上并不会通过 UndoRedo 修改图形？
	_undo_redo->create_action(VOXEL_TTR("Move nodes"));
	_undo_redo->add_do_method(this, "set_node_position", id, to);
	_undo_redo->add_undo_method(this, "set_node_position", id, from);
	_undo_redo->commit_action();
	// 我完全不知道 VisualScriptEditor 是怎么神奇地让这个工作起来的，
	// 无论它用的是 `create_action` 还是 `commit_action`。
}

void VoxelGraphEditor::set_node_position(int id, Vector2 offset) {
	String node_name = node_to_gui_name(id);
	GraphNode *node_view = get_node_typed<GraphNode>(*_graph_edit, node_name);
	if (node_view != nullptr) {
		node_view->set_position_offset(offset);
	}
	// 我们将 GUI 节点位置独立于编辑器缩放来存储，以便图形无论显示器 DPI 如何都显示一致，
	// 因此这里必须取消应用（缩放）
	_graph->set_node_gui_position(id, offset / EDSCALE);
}

void VoxelGraphEditor::_on_node_resize_request(Vector2 new_size, int node_id) {
	const String node_view_path = node_to_gui_name(node_id);
	Node *node = get_node_typed<Node>(*_graph_edit, node_view_path);
	VOXEL_ASSERT_RETURN(node != nullptr);
	VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(node);
	VOXEL_ASSERT_RETURN(node_view != nullptr);
	VOXEL_ASSERT_RETURN(_graph.is_valid());

	// TODO 不确定在这种情况下是否也要取消应用 EDSCALE？
	_undo_redo->create_action(VOXEL_TTR("Resize Node"), UndoRedo::MERGE_ENDS);
	_undo_redo->add_do_method(this, "set_node_size", node_id, new_size);
	_undo_redo->add_do_method(_graph.ptr(), "set_node_gui_size", node_id, new_size);
	_undo_redo->add_undo_method(this, "set_node_size", node_id, node_view->get_size());
	_undo_redo->add_undo_method(_graph.ptr(), "set_node_gui_size", node_id, node_view->get_size());
	_undo_redo->commit_action();
}

void VoxelGraphEditor::set_node_size(int id, Vector2 size) {
	String node_name = node_to_gui_name(id);
	GraphNode *node_view = get_node_typed<GraphNode>(*_graph_edit, node_name);
	if (node_view != nullptr) {
		node_view->set_size(size);
	}
	// 这个函数仅用于 UI，因为我们不应将节点指针直接传给 UndoRedo，稍后调用撤销操作时
	// 它们可能已被删除
	//_graph->set_node_gui_size(id, size / EDSCALE);
}

void VoxelGraphEditor::_on_graph_node_preview_gui_input(Ref<InputEvent> event) {
	Ref<InputEventMouseMotion> mm = event;
	if (mm.is_valid()) {
		// 在任意预览上方 Ctrl+拖拽 可平移它们渲染的区域。
		if (mm->is_command_or_control_pressed() && mm->get_button_mask().has_flag(VOXEL_GODOT_MouseButtonMask_MIDDLE)) {
			const Vector2 rel = mm->get_relative();
			set_preview_transform(_preview_offset - Vector2f(rel.x, -rel.y) * _preview_scale, _preview_scale);

			// 禁止 GraphEdit 平移
			get_viewport()->set_input_as_handled();
		}
	}

	Ref<InputEventMouseButton> mb = event;
	if (mb.is_valid()) {
		// 在任意预览上方按住 Ctrl+滚轮，可缩放其渲染区域。
		if (mb->is_command_or_control_pressed()) {
			const float base_factor = 1.1f;
			if (mb->get_button_index() == VOXEL_GODOT_MouseButton_WHEEL_UP) {
				set_preview_transform(_preview_offset, _preview_scale / base_factor);
				// 阻止 GraphEdit 平移
				get_viewport()->set_input_as_handled();
			}
			if (mb->get_button_index() == VOXEL_GODOT_MouseButton_WHEEL_DOWN) {
				set_preview_transform(_preview_offset, _preview_scale * base_factor);
				// 阻止 GraphEdit 平移
				get_viewport()->set_input_as_handled();
			}
		}
	}
}

void VoxelGraphEditor::set_preview_transform(Vector2f offset, float scale) {
	if (offset != _preview_offset || scale != _preview_scale) {
		_preview_offset = offset;
		_preview_scale = scale;
		// 快速更新
		if (_time_before_preview_update <= 0.f) {
			_time_before_preview_update = 0.1f;
		}
	}
}

Vector2 get_graph_offset_from_mouse(const GraphEdit *graph_edit, const Vector2 local_mouse_pos) {
	// TODO 请求一个方法，或者至少提供关于如何实现它的文档
	Vector2 offset = get_graph_edit_scroll_offset(*graph_edit) + local_mouse_pos;
	if (is_graph_edit_using_snapping(*graph_edit)) {
		const int snap = get_graph_edit_snapping_distance(*graph_edit);
		offset = offset.snapped(Vector2(snap, snap));
	}
	offset /= EDSCALE;
	offset /= graph_edit->get_zoom();
	return offset;
}

void VoxelGraphEditor::_on_node_dialog_node_selected(int id) {
	// 创建基础节点类型

	const Vector2 pos = get_graph_offset_from_mouse(_graph_edit, _click_position);
	const uint32_t node_type_id = id;

	const uint32_t node_id = _graph->generate_node_id();
	const StringName node_name = node_to_gui_name(node_id);

	_undo_redo->create_action(VOXEL_TTR("Create Node"));
	_undo_redo->add_do_method(_graph.ptr(), "create_node", node_type_id, pos, node_id);
	_undo_redo->add_do_method(this, "create_node_gui", node_id);
	_undo_redo->add_undo_method(_graph.ptr(), "remove_node", node_id);
	_undo_redo->add_undo_method(this, "remove_node_gui", node_name);
	_undo_redo->commit_action();
}

void VoxelGraphEditor::_on_graph_edit_node_selected(Node *p_node) {
	VoxelGraphEditorNode *node = Object::cast_to<VoxelGraphEditorNode>(p_node);
	emit_signal(SIGNAL_NODE_SELECTED, node->get_generator_node_id());
}

void VoxelGraphEditor::_on_graph_edit_node_deselected(Node *p_node) {
	// 仅仅检查现在是否什么都没选中并不可靠，因为用户可能刚刚选中了另一个
	// 节点，而且我不知道 `GraphEdit` 何时会在当前调用栈中更新 `selected` 标志。
	// GraphEdit 没有提供足够上下文来判断这一点的 API，所以只能依靠这种粗糙的变通方法。
	if (!_nothing_selected_check_scheduled) {
		_nothing_selected_check_scheduled = true;
		call_deferred("_check_nothing_selected");
	}
}

void VoxelGraphEditor::_check_nothing_selected() {
	_nothing_selected_check_scheduled = false;
	if (is_nothing_selected(_graph_edit)) {
		emit_signal(SIGNAL_NOTHING_SELECTED);
	}
}

void reset_modulates(GraphEdit &graph_edit) {
	for (int child_index = 0; child_index < graph_edit.get_child_count(); ++child_index) {
		VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(graph_edit.get_child(child_index));
		if (node_view == nullptr) {
			continue;
		}
		node_view->set_modulate(Color(1, 1, 1));
	}
}

void VoxelGraphEditor::update_previews(bool with_live_update) {
	VOXEL_ASSERT_RETURN(_graph.is_valid());

	clear_range_analysis_tooltips();
	hide_profiling_ratios();
	reset_modulates(*_graph_edit);

	update_functions();

	const uint64_t time_before = Time::get_singleton()->get_ticks_usec();

	// VoxelGeneratorGraph 编译时有额外的要求
	const pg::CompilationResult result = _generator.is_valid() ? _generator->compile(true) : _graph->compile(true);

	if (!result.success) {
		ERR_PRINT(String("Graph compilation failed: {0}").format(varray(result.message)));

		_compile_result_label->set_text(result.message);
		_compile_result_label->set_tooltip_text(result.message);
		_compile_result_label->set_modulate(Color(1, 0.3, 0.1));
		_compile_result_label->show();

		if (result.node_id >= 0) {
			String node_view_path = node_to_gui_name(result.node_id);
			VoxelGraphEditorNode *node_view = get_node_typed<VoxelGraphEditorNode>(*_graph_edit, node_view_path);
			// 如果发生这种情况，那么该节点可能是编译器创建的节点，被错误地重映射了
			if (node_view != nullptr) {
				node_view->set_modulate(Color(1, 0.3, 0.1));
			} else {
				VOXEL_PRINT_ERROR("Could not get the node with the error");
			}
		}
		return;

	} else {
		_compile_result_label->hide();
	}

	if (_generator.is_valid()) {
		if (!_generator->is_good()) {
			return;
		}
	} else {
		if (!_graph->is_compiled()) {
			return;
		}
	}

	// 我们假设没有其他线程会尝试修改图形并编译出不好的结果

	// TODO 在可能的情况下，让切片预览支持任意函数
	// TODO 在可能的情况下，让范围分析预览支持任意函数

	update_slice_previews();

	if (_range_analysis_dialog->is_analysis_enabled()) {
		update_range_analysis_previews();
	}

	const uint64_t time_taken = Time::get_singleton()->get_ticks_usec() - time_before;
	VOXEL_PRINT_VERBOSE(format("Previews generated in {} us", time_taken));

	if (_live_update_enabled && with_live_update) {
		// TODO 使用该哈希来避免完全重新编译，因为 `changed` 现在会报告任何更改，包括
		// 那些不需要重新编译的更改...

		// 检查图形是否以实际影响输出的方式发生了变化，
		// 因为重新生成所有体素代价高昂。
		// 注意，涉及的可能是子资源，而不仅仅是节点连接和属性。
		const uint64_t hash = _graph->get_output_graph_hash();

		if (hash != _last_output_graph_hash) {
			_last_output_graph_hash = hash;

			// 不直接调用 `_voxel_node`，因为编辑器可能被固定，而地形实际上并未被
			// 选中。在这种情况下，插件可能会将节点重置为 null。但对于使用了当前图形且
			// 位于被编辑场景中的地形，我们希望它们能够更新，因此这可以委托给编辑器
			// 插件。从这里无法获得足够的上下文来干净地完成这件事。
			emit_signal(SIGNAL_REGENERATE_REQUESTED);
		}
	}
}

void VoxelGraphEditor::update_range_analysis_previews() {
	VOXEL_PRINT_VERBOSE("Updating range analysis previews");
	GraphEditorAdapter adapter(_generator, _graph);
	ERR_FAIL_COND(!adapter.is_good());

	const AABB aabb = _range_analysis_dialog->get_aabb();

	// 计算连接到预览节点的输出端的实际范围
	StdUnorderedMap<uint32_t, math::Interval> actual_ranges;
	{
		StdVector<VoxelGraphEditorNodePreviewInfo> slice_preview_infos = get_slice_previews();

		const Vector3i min_pos = to_vec3i(math::floor(aabb.position));
		const Vector3i max_pos = to_vec3i(math::ceil(aabb.position + aabb.size));
		const Vector3i res = max_pos - min_pos;
		const uint64_t volume64 = Vector3iUtil::get_volume_u64(res);
		const uint64_t max_volume = 256 * 256 * 256;

		if (volume64 < max_volume) {
			const uint32_t volume = static_cast<uint32_t>(volume64);
			const uint32_t chunk_size = 256;
			const uint32_t num_chunks = math::ceildiv(volume, chunk_size);
			VOXEL_ASSERT_RETURN(num_chunks > 0);

			StdVector<float> x_vec;
			StdVector<float> y_vec;
			StdVector<float> z_vec;

			VOXEL_ASSERT_RETURN((num_chunks * chunk_size) >= volume);
			const uint32_t last_chunk_size = (num_chunks * chunk_size) == volume ? chunk_size : volume % chunk_size;
			const uint32_t last_chunk_index = num_chunks - 1;

			Vector3i voxel_pos = min_pos;

			for (uint32_t chunk_index = 0; chunk_index < num_chunks; ++chunk_index) {
				const uint32_t rel_size = chunk_index < last_chunk_index ? chunk_size : last_chunk_size;

				x_vec.resize(rel_size);
				y_vec.resize(rel_size);
				z_vec.resize(rel_size);

				for (unsigned int rel_index = 0; rel_index < rel_size; ++rel_index) {
					++voxel_pos.x;
					if (voxel_pos.x == max_pos.x) {
						voxel_pos.x = min_pos.x;
						++voxel_pos.y;
						if (voxel_pos.y == max_pos.y) {
							voxel_pos.y = min_pos.y;
							++voxel_pos.z;
						}
					}

					x_vec[rel_index] = voxel_pos.x;
					y_vec[rel_index] = voxel_pos.y;
					z_vec[rel_index] = voxel_pos.z;
				}

				{
					Span<float> x_coords = to_span(x_vec);
					Span<float> y_coords = to_span(y_vec);
					Span<float> z_coords = to_span(z_vec);

					adapter.generate_set(x_coords, y_coords, z_coords);
				}

				const pg::Runtime::State &last_state = adapter.get_last_state_from_current_thread();

				for (const VoxelGraphEditorNodePreviewInfo &info : slice_preview_infos) {
					const pg::Runtime::Buffer &buffer = last_state.get_buffer(info.address);

					VOXEL_ASSERT_CONTINUE(buffer.data != nullptr);
					Span<const float> buffer_s(buffer.data, buffer.size);
					VOXEL_ASSERT_CONTINUE(buffer_s.size() > 0);

					math::Interval range = math::Interval::from_single_value(buffer_s[0]);
					for (const float v : buffer_s) {
						range.add_point(v);
					}

					math::Interval &accum_range = actual_ranges[info.address];
					accum_range.add_interval(range);
				}
			}
		}
	}

	adapter.debug_analyze_range(math::floor_to_int(aabb.position), math::floor_to_int(aabb.position + aabb.size));

	const pg::Runtime::State &state = adapter.get_last_state_from_current_thread();

	const Color greyed_out_color(1, 1, 1, 0.5);

	for (int child_index = 0; child_index < _graph_edit->get_child_count(); ++child_index) {
		VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_child(child_index));
		if (node_view == nullptr) {
			continue;
		}

		if (!node_view->has_outputs()) {
			continue;
		}

		// 暂时假设该节点不会运行
		// TODO 如果 GraphEdit 的小地图能考虑这种着色就好了...
		node_view->set_modulate(greyed_out_color);

		node_view->update_range_analysis_tooltips(adapter, state, actual_ranges);
	}

	// 仅高亮那些实际会运行的节点。
	// 注意，由于内部展开，某些节点可能在此映射中出现两次。
	Span<const uint32_t> execution_map = VoxelGeneratorGraph::get_last_execution_map_debug_from_current_thread();
	for (unsigned int i = 0; i < execution_map.size(); ++i) {
		const uint32_t node_id = execution_map[i];
		// 某些返回的节点可能不在面向用户的图形中，因为它们是编译期间生成的
		if (!_graph->has_node(node_id)) {
			VOXEL_PRINT_VERBOSE(
					format("Ignoring node {} from range analysis results, not present in user graph", node_id)
			);
			continue;
		}
		const String node_view_path = node_to_gui_name(node_id);
		Node *node = get_node_typed<Node>(*_graph_edit, node_view_path);
		VOXEL_ASSERT_CONTINUE(node != nullptr);
		VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(node);
		VOXEL_ASSERT_CONTINUE(node_view != nullptr);
		node_view->set_modulate(Color(1, 1, 1));
	}
}

void VoxelGraphEditor::update_range_analysis_gizmo() {
	if (!_range_analysis_dialog->is_analysis_enabled()) {
		_debug_renderer.clear();
		return;
	}

	VoxelNode *terrain_node = _terrain_node.get();

	if (terrain_node == nullptr) {
		return;
	}

	const Transform3D parent_transform = terrain_node->get_global_transform();
	const AABB aabb = _range_analysis_dialog->get_aabb();
	_debug_renderer.begin();
	_debug_renderer.draw_box(
			parent_transform * Transform3D(Basis().scaled(aabb.size), aabb.position), Color(1.0, 1.0, 0.0)
	);
	_debug_renderer.end();
}

StdVector<VoxelGraphEditorNodePreviewInfo> VoxelGraphEditor::get_slice_previews() const {
	GraphEditorAdapter adapter(_generator, _graph);
	StdVector<VoxelGraphEditorNodePreviewInfo> previews;

	// 收集预览节点
	for (int i = 0; i < _graph_edit->get_child_count(); ++i) {
		const VoxelGraphEditorNode *node = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_child(i));
		if (node == nullptr || node->get_preview() == nullptr) {
			continue;
		}
		ProgramGraph::PortLocation dst;
		dst.node_id = node->get_generator_node_id();
		dst.port_index = 0;
		ProgramGraph::PortLocation src;
		if (!_graph->try_get_connection_to(dst, src)) {
			// 未连接？
			continue;
		}
		VoxelGraphEditorNodePreviewInfo info;
		info.control = node->get_preview();
		if (!adapter.try_get_output_port_address(src, info.address)) {
			// 不属于编译结果
			continue;
		}
		info.node_id = dst.node_id;
		previews.push_back(info);
	}

	return previews;
}

void VoxelGraphEditor::update_slice_previews() {
	StdVector<VoxelGraphEditorNodePreviewInfo> previews = get_slice_previews();
	GraphEditorAdapter adapter(_generator, _graph);
	VoxelGraphEditorNodePreview::update_previews(
			adapter, to_span(previews), _node_preview_mode, _preview_scale, _preview_offset
	);
}

void VoxelGraphEditor::clear_range_analysis_tooltips() {
	for (int child_index = 0; child_index < _graph_edit->get_child_count(); ++child_index) {
		VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_child(child_index));
		if (node_view == nullptr) {
			continue;
		}
		node_view->clear_range_analysis_tooltips();
	}
}

void VoxelGraphEditor::schedule_preview_update() {
	_time_before_preview_update = 0.5f;
}

void VoxelGraphEditor::_on_graph_changed() {
	schedule_preview_update();
}

void VoxelGraphEditor::_on_graph_node_name_changed(int node_id) {
	ERR_FAIL_COND(_graph.is_null());

	const uint32_t node_type_id = _graph->get_node_type_id(node_id);

	const String ui_node_name = node_to_gui_name(node_id);
	VoxelGraphEditorNode *node_view = get_node_typed<VoxelGraphEditorNode>(*_graph_edit, ui_node_name);
	ERR_FAIL_COND(node_view == nullptr);

	if (node_type_id != VoxelGraphFunction::NODE_EXPRESSION) {
		node_view->update_title(**_graph);
	}
}

void VoxelGraphEditor::profile() {
	if (_generator.is_null() || !_generator->is_good()) {
		return;
	}

	StdVector<VoxelGeneratorGraph::NodeProfilingInfo> nodes_profiling_info;
	const float us = _generator->debug_measure_microseconds_per_voxel(false, &nodes_profiling_info);
	_profile_label->set_text(String("{0} microseconds per voxel").format(varray(us)));

	struct NodeRatio {
		uint32_t node_id;
		float ratio;
	};

	StdVector<NodeRatio> node_ratios;

	// 去重条目并获取最大值
	float max_individual_time = 0.f;
	for (const VoxelGeneratorGraph::NodeProfilingInfo &info : nodes_profiling_info) {
		unsigned int i = 0;
		for (; i < node_ratios.size(); ++i) {
			if (node_ratios[i].node_id == info.node_id) {
				break;
			}
		}
		if (i == node_ratios.size()) {
			node_ratios.push_back(NodeRatio{ info.node_id, float(info.microseconds) });
		} else {
			node_ratios[i].ratio += info.microseconds;
		}
		max_individual_time = math::max(max_individual_time, float(info.microseconds));
	}

	if (max_individual_time > 0.f) {
		for (NodeRatio &nr : node_ratios) {
			nr.ratio = math::clamp(nr.ratio / max_individual_time, 0.f, 1.f);
		}
	} else {
		for (NodeRatio &nr : node_ratios) {
			nr.ratio = 1.f;
		}
	}

	for (const NodeRatio &nr : node_ratios) {
		// 某些编译期间生成的节点不存在于面向用户的图形中
		if (!_graph->has_node(nr.node_id)) {
			VOXEL_PRINT_VERBOSE(format("Ignoring node {} from profiling results, not present in user graph", nr.node_id));
			continue;
		}
		const String ui_node_name = node_to_gui_name(nr.node_id);
		VoxelGraphEditorNode *node_view = get_node_typed<VoxelGraphEditorNode>(*_graph_edit, ui_node_name);
		ERR_CONTINUE(node_view == nullptr);
		node_view->set_profiling_ratio_visible(true);
		node_view->set_profiling_ratio(nr.ratio);
	}
}

void VoxelGraphEditor::update_preview_axes_menu() {
	// 根据当前设置更新菜单状态
	ERR_FAIL_COND(_preview_axes_menu == nullptr);
	ToolbarMenuIDs id;
	switch (_node_preview_mode) {
		case GraphEditorPreview::VIEW_SLICE_XY:
			id = MENU_PREVIEW_AXES_XY;
			break;
		case GraphEditorPreview::VIEW_SLICE_XZ:
			id = MENU_PREVIEW_AXES_XZ;
			break;
		default:
			ERR_PRINT("Unknown preview axes");
			return;
	}
	update_menu_radio_checkable_items(*_preview_axes_menu, id);
}

void VoxelGraphEditor::hide_profiling_ratios() {
	for (int child_index = 0; child_index < _graph_edit->get_child_count(); ++child_index) {
		VoxelGraphEditorNode *node_view = Object::cast_to<VoxelGraphEditorNode>(_graph_edit->get_child(child_index));
		if (node_view == nullptr) {
			continue;
		}
		node_view->set_profiling_ratio_visible(false);
	}
}

void VoxelGraphEditor::update_buttons_availability() {
	// 某些功能目前仅在使用生成器时才可用（暂时如此）
	// _debug_menu_button->set_disabled(_generator.is_null());
	// _graph_menu_button->set_disabled(_generator.is_null());

	// TODO 在任意图形上实现性能分析
	PopupMenu &menu = *_debug_menu_button->get_popup();
	{
		const int index = menu.get_item_index(MENU_PROFILE);
		menu.set_item_disabled(index, _generator.is_null());
	}
	{
		const int index = menu.get_item_index(MENU_LIVE_UPDATE);
		menu.set_item_disabled(index, _generator.is_null());
	}
}

void VoxelGraphEditor::_on_range_analysis_toggled(bool enabled) {
	schedule_preview_update();
	update_range_analysis_gizmo();
}

void VoxelGraphEditor::_on_range_analysis_area_changed() {
	schedule_preview_update();
	update_range_analysis_gizmo();
}

void VoxelGraphEditor::_on_popout_button_pressed() {
	emit_signal(SIGNAL_POPOUT_REQUESTED);
}

void VoxelGraphEditor::set_popout_button_enabled(bool enable) {
	_popout_button->set_visible(enable);
}

void VoxelGraphEditor::_on_node_dialog_file_selected(String fpath) {
	create_function_node(fpath);
}

void VoxelGraphEditor::create_function_node(String fpath) {
	Ref<Resource> res = load_resource(fpath);
	if (res.is_null()) {
		ERR_PRINT("Could not instantiate function, resource could not be loaded.");
		return;
	}
	Ref<VoxelGraphFunction> func = res;
	if (func.is_null()) {
		ERR_PRINT(String("Could not instantiate function, resource is not a {0}.")
						  .format(varray(VoxelGraphFunction::get_class_static())));
		return;
	}

	if (func == _graph) {
		ERR_PRINT("Adding a function to itself is not allowed.");
		return;
	}

	if (func->contains_reference_to_function(_graph)) {
		ERR_PRINT("Adding a function to itself is not allowed, detected an indirect cycle.");
		return;
	}

	const Vector2 pos = get_graph_offset_from_mouse(_graph_edit, _click_position);

	const uint32_t node_id = _graph->generate_node_id();
	const StringName node_name = node_to_gui_name(node_id);

	_undo_redo->create_action(VOXEL_TTR("Create Function Node"));
	_undo_redo->add_do_method(_graph.ptr(), "create_function_node", func, pos, node_id);
	_undo_redo->add_do_method(this, "create_node_gui", node_id);
	_undo_redo->add_undo_method(_graph.ptr(), "remove_node", node_id);
	_undo_redo->add_undo_method(this, "remove_node_gui", node_name);
	_undo_redo->commit_action();
}

void VoxelGraphEditor::update_functions() {
	struct L {
		static void try_update_node_view(
				VoxelGraphFunction &graph,
				GraphEdit &graph_edit,
				uint32_t node_id,
				const String &node_view_name
		) {
			if (graph.get_node_type_id(node_id) == VoxelGraphFunction::NODE_FUNCTION) {
				VoxelGraphEditorNode *node_view = get_node_typed<VoxelGraphEditorNode>(graph_edit, node_view_name);
				ERR_FAIL_COND(node_view == nullptr);
				node_view->update_layout(graph);
			}
		}
	};

	ERR_FAIL_COND(_graph.is_null());

	StdVector<ProgramGraph::Connection> removed_connections;
	_graph->update_function_nodes(&removed_connections);
	// TODO 这可能会干扰撤销/重做并移除连接，但我不确定是否值得处理它。
	// 一种变通方法是引入“无效端口”的概念，让函数节点保留其旧端口，
	// 直到用户通过某个操作显式移除它们（撤销后又会恢复）。

	for (const ProgramGraph::Connection &con : removed_connections) {
		const String from_node_name = node_to_gui_name(con.src.node_id);
		const String to_node_name = node_to_gui_name(con.dst.node_id);

		_graph_edit->disconnect_node(from_node_name, con.src.port_index, to_node_name, con.dst.port_index);

		L::try_update_node_view(**_graph, *_graph_edit, con.src.node_id, from_node_name);
		L::try_update_node_view(**_graph, *_graph_edit, con.dst.node_id, to_node_name);
	}
}

void VoxelGraphEditor::copy_selected_nodes_to_clipboard() {
	VOXEL_ASSERT_RETURN(_graph.is_valid());

	StdVector<VoxelGraphEditorNode *> node_views;
	get_selected_nodes(*_graph_edit, node_views);

	if (node_views.size() == 0) {
		VOXEL_PRINT_VERBOSE("No selected nodes to copy");
		return;
	}

	StdVector<uint32_t> node_ids;
	node_ids.reserve(node_views.size());

	for (VoxelGraphEditorNode *node_view : node_views) {
		node_ids.push_back(node_view->get_generator_node_id());
	}

	Ref<VoxelGraphFunction> dup;
	dup.instantiate();
	_graph->duplicate_subgraph(to_span(node_ids), Span<const uint32_t>(), **dup, Vector2());

	_clipboard.graph = dup;
	VOXEL_ASSERT_RETURN(dup.is_valid());

	VOXEL_PRINT_VERBOSE(format("Copied {} nodes", _clipboard.graph->get_nodes_count()));
}

void VoxelGraphEditor::_on_graph_edit_copy_nodes_request() {
	copy_selected_nodes_to_clipboard();
}

void VoxelGraphEditor::_on_graph_edit_paste_nodes_request() {
	paste_clipboard();
}

void VoxelGraphEditor::paste_clipboard() {
	VOXEL_ASSERT_RETURN(_graph.is_valid());

	if (!_clipboard.graph.is_valid()) {
		VOXEL_PRINT_VERBOSE("No nodes to paste, clipboard is empty");
		return;
	}

	// 我们需要预先生成节点 ID，以便撤销/重做能够工作
	PackedInt32Array pre_generated_ids;
	for (unsigned int i = 0; i < _clipboard.graph->get_nodes_count(); ++i) {
		pre_generated_ids.append(_graph->generate_node_id());
	}

	// 大致将节点居中到鼠标指向的位置
	Vector2 gui_offset;
	{
		PackedInt32Array node_ids = _clipboard.graph->get_node_ids();
		VOXEL_ASSERT_RETURN(node_ids.size() > 0);
		Vector2 clipboard_center;
		for (int i = 0; i < node_ids.size(); ++i) {
			const int id = node_ids[i];
			clipboard_center += _clipboard.graph->get_node_gui_position(id);
		}
		clipboard_center /= float(node_ids.size());

		const Vector2 mouse_position =
				get_graph_offset_from_mouse(_graph_edit, _graph_edit->get_local_mouse_position());
		gui_offset = mouse_position - clipboard_center;
	}

	_undo_redo->create_action("Paste nodes");

	// Do

	_undo_redo->add_do_method(
			_graph.ptr(), "paste_graph_with_pre_generated_ids", _clipboard.graph, pre_generated_ids, gui_offset
	);

	for (int i = 0; i < pre_generated_ids.size(); ++i) {
		const int id = pre_generated_ids[i];
		_undo_redo->add_do_method(this, "create_node_gui", id);
	}
	for (int i = 0; i < pre_generated_ids.size(); ++i) {
		const int id = pre_generated_ids[i];
		_undo_redo->add_do_method(this, "create_node_gui_input_connections", id);
	}

	// Undo

	for (int i = 0; i < pre_generated_ids.size(); ++i) {
		const int id = pre_generated_ids[i];
		_undo_redo->add_undo_method(_graph.ptr(), "remove_node", id);

		const StringName node_name = node_to_gui_name(id);
		_undo_redo->add_undo_method(this, "remove_node_gui", node_name);
	}

	_undo_redo->commit_action();
}

void VoxelGraphEditor::create_node_gui_input_connections(int node_id) {
	VOXEL_ASSERT_RETURN(_graph.is_valid());
	// 假设该节点在 GraphEdit 中没有设置连接。这相当特定于复制/粘贴。

	const uint32_t input_count = _graph->get_node_input_count(node_id);

	for (uint32_t i = 0; i < input_count; ++i) {
		const ProgramGraph::PortLocation dst{ uint32_t(node_id), i };
		ProgramGraph::PortLocation src;

		if (_graph->try_get_connection_to(dst, src)) {
			_graph_edit->connect_node(
					node_to_gui_name(src.node_id), src.port_index, node_to_gui_name(dst.node_id), dst.port_index
			);
		}
	}
}

void VoxelGraphEditor::_bind_methods() {
	using Self = VoxelGraphEditor;

	ClassDB::bind_method(D_METHOD("_check_nothing_selected"), &Self::_check_nothing_selected);

	ClassDB::bind_method(D_METHOD("create_node_gui", "node_id"), &Self::create_node_gui);
	ClassDB::bind_method(
			D_METHOD("create_node_gui_input_connections", "node_id"), &Self::create_node_gui_input_connections
	);
	ClassDB::bind_method(D_METHOD("remove_node_gui", "node_name"), &Self::remove_node_gui);
	ClassDB::bind_method(D_METHOD("set_node_position", "node_id", "offset"), &Self::set_node_position);
	ClassDB::bind_method(D_METHOD("set_node_size", "node_id", "size"), &Self::set_node_size);
	ClassDB::bind_method(D_METHOD("update_node_layout", "node_id"), &Self::update_node_layout);
	ClassDB::bind_method(D_METHOD("update_node_comment", "node_id"), &Self::update_node_comment);

	ADD_SIGNAL(MethodInfo(SIGNAL_NODE_SELECTED, PropertyInfo(Variant::INT, "node_id")));
	ADD_SIGNAL(MethodInfo(SIGNAL_NOTHING_SELECTED));
	ADD_SIGNAL(MethodInfo(SIGNAL_NODES_DELETED));
	ADD_SIGNAL(MethodInfo(SIGNAL_REGENERATE_REQUESTED));
	ADD_SIGNAL(MethodInfo(SIGNAL_POPOUT_REQUESTED));
}

} // namespace voxel
