#include "voxel_graph_editor_plugin.h"
#include "../../constants/voxel_string_names.h"
#include "../../generators/graph/voxel_generator_graph.h"
#include "../../terrain/voxel_node.h"
#include "../../util/containers/container_funcs.h"
#include "../../util/godot/classes/button.h"
#include <core/version.h>
#include <editor/editor_interface.h>
#include <editor/editor_data.h>
#include "../../util/godot/classes/node.h"
#include "../../util/godot/classes/object.h"
#include "../../util/godot/classes/resource_saver.h"
#include "../../util/godot/core/string.h"
#include <core/version.h>
#include <editor/themes/editor_scale.h>
#include "../../util/string/format.h"
#include "editor_property_text_change_on_submit.h"
#include "voxel_graph_editor.h"
#include "voxel_graph_editor_inspector_plugin.h"
#include "voxel_graph_editor_io_dialog.h"
#include "voxel_graph_editor_window.h"
#include "voxel_graph_function_inspector_plugin.h"
#include <editor/plugins/editor_plugin.h>
#include <editor/editor_data.h>
#include <editor/themes/editor_scale.h>
#include <core/version.h>

#include <editor/editor_node.h>
#include <editor/gui/editor_bottom_panel.h>
#include <core/object/callable_mp.h>


namespace voxel {

using namespace pg;
using namespace voxel::godot;

VoxelGraphEditorPlugin::VoxelGraphEditorPlugin() {}

// 插件构造期间 EditorNode 尚未就绪，因此将工作推迟到 `init()`。
void VoxelGraphEditorPlugin::init() {
	// EditorInterface *ed = get_editor_interface();
	_graph_editor = memnew(VoxelGraphEditor);
	_graph_editor->set_custom_minimum_size(Size2(0, 300) * EDSCALE);
	_graph_editor->connect(
			VoxelGraphEditor::SIGNAL_NODE_SELECTED,
			callable_mp(this, &VoxelGraphEditorPlugin::_on_graph_editor_node_selected)
	);
	_graph_editor->connect(
			VoxelGraphEditor::SIGNAL_NOTHING_SELECTED,
			callable_mp(this, &VoxelGraphEditorPlugin::_on_graph_editor_nothing_selected)
	);
	_graph_editor->connect(
			VoxelGraphEditor::SIGNAL_NODES_DELETED,
			callable_mp(this, &VoxelGraphEditorPlugin::_on_graph_editor_nodes_deleted)
	);
	_graph_editor->connect(
			VoxelGraphEditor::SIGNAL_REGENERATE_REQUESTED,
			callable_mp(this, &VoxelGraphEditorPlugin::_on_graph_editor_regenerate_requested)
	);
	_graph_editor->connect(
			VoxelGraphEditor::SIGNAL_POPOUT_REQUESTED,
			callable_mp(this, &VoxelGraphEditorPlugin::_on_graph_editor_popout_requested)
	);
	_bottom_panel_button = EditorNode::get_bottom_panel()->add_item(VOXEL_TTR("Voxel Graph"), _graph_editor);
	_bottom_panel_button->hide();

	// TODO 将此移到 `_enter_tree`，并在 `_exit_tree` 中移除？
	Ref<VoxelGraphEditorInspectorPlugin> vge_inspector_plugin;
	vge_inspector_plugin.instantiate();
	add_inspector_plugin(vge_inspector_plugin);

	Ref<VoxelGraphFunctionInspectorPlugin> vgf_inspector_plugin;
	vgf_inspector_plugin.instantiate();
	vgf_inspector_plugin->set_listener(this);
	add_inspector_plugin(vgf_inspector_plugin);
}

bool VoxelGraphEditorPlugin::_voxel_handles(const Object *p_object) const {
	if (p_object == nullptr) {
		return false;
	}
	// 目前我们必须处理这两种资源，因为我们让生成器表现得像一个图形，
	// 尽管图形实际上是它的内部属性（“主函数”）。这可以为用户省去一些点击。
	const VoxelGeneratorGraph *generator_ptr = Object::cast_to<VoxelGeneratorGraph>(p_object);
	if (generator_ptr != nullptr) {
		return true;
	}
	const VoxelGraphFunction *graph_ptr = Object::cast_to<VoxelGraphFunction>(p_object);
	if (graph_ptr != nullptr) {
		return true;
	}
	// 过去我们也会处理 `VoxelGraphNodeInspectorWrapper`，用于检查图形的节点，但
	// Godot 实际上不允许一个插件同时处理多个资源。因此我们改用了
	// 不同的方式。
	// 参见 https://github.com/godotengine/godot/issues/73650
	return false;
}

void VoxelGraphEditorPlugin::_voxel_edit(Object *p_object) {
	// 用于检查图形节点时的变通方法...
	if (p_object == nullptr && _ignore_edit_null) {
		VOXEL_PRINT_VERBOSE(format("{}: ignored edit(null)", VOXEL_CLASS_NAME_C(VoxelGraphEditorPlugin)));
		return;
	}

	Ref<VoxelGeneratorGraph> generator;
	Ref<VoxelGraphFunction> graph;
	{
		VoxelGeneratorGraph *generator_ptr = Object::cast_to<VoxelGeneratorGraph>(p_object);
		if (generator_ptr != nullptr) {
			generator.reference_ptr(generator_ptr);
		}
	}
	{
		VoxelGraphFunction *graph_ptr = Object::cast_to<VoxelGraphFunction>(p_object);
		if (graph_ptr != nullptr) {
			graph.reference_ptr(graph_ptr);
		}
	}
	{
		VoxelGraphNodeInspectorWrapper *wrapper_ptr = Object::cast_to<VoxelGraphNodeInspectorWrapper>(p_object);
		if (wrapper_ptr != nullptr) {
			generator = wrapper_ptr->get_generator();
			graph = wrapper_ptr->get_graph();
		}
	}

	_graph_editor->set_undo_redo(get_undo_redo()); // UndoRedo 在构造函数中不可用

	if (generator.is_valid()) {
		const VoxelStringNames &sn = VoxelStringNames::get_singleton();
		Callable callable = callable_mp(this, &VoxelGraphEditorPlugin::_on_generator_changed);
		if (!generator->is_connected(sn.changed, callable)) {
			generator->connect(sn.changed, callable);
		}
	}

	_graph_editor->set_generator(generator);
	if (generator.is_null()) {
		_graph_editor->set_graph(graph);
	}

	{
		VoxelNode *voxel_node = nullptr;
		if (generator.is_valid()) {
			Array selected_nodes = get_editor_interface()->get_selection()->get_selected_nodes();
			for (int i = 0; i < selected_nodes.size(); ++i) {
				Node *node = Object::cast_to<Node>(selected_nodes[i]);
				ERR_FAIL_COND(node == nullptr);
				VoxelNode *vn = Object::cast_to<VoxelNode>(node);
				if (vn != nullptr && vn->get_generator() == generator) {
					voxel_node = vn;
					break;
				}
			}
		}
		_voxel_node.set(voxel_node);
		// TODO 有时 Godot 不再给我那个节点，它会变成 null，尽管它仍然在
		//      场景树中被选中。但哈哈才怪，因为我以某种方式选中了一个资源，导致它跑偏了？？
		//      这会毫无理由地导致场景内的预览辅助线（例如范围分析）消失，
		//      在调试本就难以调查的 bug 时快把我逼疯了。
		//      如果你选中一个图形节点，然后点击图形的背景（即编辑图形资源），就会发生这种情况。
		//      要获得非 null 值的唯一方法是在场景树中手动再次选中地形节点。
		//      所以我用这种方式绕过它……永远不把它设为 null。这实在太糟糕了。
		//      由于我们使用的是 ObjectWeakRef，它不应该有指针安全问题。
		if (voxel_node != nullptr) {
			_graph_editor->set_voxel_node(voxel_node);
		}
	}

	if (_graph_editor_window != nullptr) {
		update_graph_editor_window_title();
	}
}

void VoxelGraphEditorPlugin::_voxel_make_visible(bool visible) {
	// 用于检查图形节点时的变通方法...
	if (_ignore_make_visible) {
		VOXEL_PRINT_VERBOSE(format("{}: ignored make_visible({})", VOXEL_CLASS_NAME_C(VoxelGraphEditorPlugin), visible));
		return;
	}

	if (_graph_editor_window != nullptr) {
		return;
	}

	if (visible) {
		_bottom_panel_button->show();
		make_bottom_panel_item_visible(_graph_editor);

	} else {
		_voxel_node.set(nullptr);
		_graph_editor->set_voxel_node(nullptr);

		const bool pinned = _graph_editor_window != nullptr || _graph_editor->is_pinned_hint();
		if (!pinned) {
			_bottom_panel_button->hide();

			// TODO 处理 `_on_graph_editor_node_selected` 中发生的胡闹的糟糕 hack
			if (!_deferred_visibility_scheduled) {
				_deferred_visibility_scheduled = true;
				call_deferred("_hide_deferred");
			}
		}
	}
}

void VoxelGraphEditorPlugin::_hide_deferred() {
	_deferred_visibility_scheduled = false;
	if (_bottom_panel_button->is_visible()) {
		// 实际上仍然可见？那就不要隐藏
		return;
	}
	// 关键是当插件的 UI 关闭时（真正的关闭，而不是关闭后又同时重新打开！），
	// 它应该清理自己的 UI，以免浪费内存（因为它引用了很多东西）。
	_voxel_edit(nullptr);

	if (_graph_editor->is_visible_in_tree()) {
		hide_bottom_panel();
	}
}

void VoxelGraphEditorPlugin::_on_graph_editor_node_selected(uint32_t node_id) {
	// 节点不是 Godot 对象，所以我们必须创建一个代理。
	// 每次都必须创建新的包装器，因为它需要在给定时间内指向同一个节点。
	Ref<VoxelGraphNodeInspectorWrapper> wrapper;
	wrapper.instantiate();
	wrapper->setup(node_id, _graph_editor);
	// 绕过新行为：即使编辑的对象也由本插件处理，甚至即使 `inspector_only` 为 `true`，
	// Godot 也会在编辑其他对象时先调用 `edit(nullptr)`。`edit(nullptr)` 会导致 UI
	// 在 GraphNode 正在发出 `selected` 信号时被清理，从而销毁该 GraphNode。
	_ignore_edit_null = true;
	_ignore_make_visible = true;
	// 注意：这既不明示也没有文档说明，但由于 EditorHistory::_add_object，引用会保持存活。
	// 指定 `inspector_only=true`，因为这是其他插件在编辑“子对象”时的做法
	get_editor_interface()->inspect_object(*wrapper, String(), true);
	_ignore_edit_null = false;
	_ignore_make_visible = false;
	_node_wrappers.push_back(wrapper);
	// TODO 这里的情况太荒谬了……
	// 尽管我们传入了 `inspector_only=true`，`inspect_object()` 仍然会走到 Godot 出于某种原因
	// 在所有插件上调用 `make_visible(false)` 的地方……并且还会在我们的插件上调用 `edit(null)`。
	// 我不明白那个参数的意义是什么……所以我们只能忽略这些调用，好在目前
	// 这样是可行的，因为它们没有使用 `call_deferred`。
	// https://github.com/godotengine/godot/issues/40166
}

void VoxelGraphEditorPlugin::inspect_graph_or_generator(const VoxelGraphEditor &graph_editor) {
	Ref<VoxelGeneratorGraph> generator = graph_editor.get_generator();
	if (generator.is_valid()) {
		_ignore_edit_null = true;
		get_editor_interface()->inspect_object(*generator);
		_ignore_edit_null = false;
		return;
	}
	Ref<VoxelGraphFunction> graph = graph_editor.get_graph();
	if (graph.is_valid()) {
		_ignore_edit_null = true;
		get_editor_interface()->inspect_object(*graph);
		_ignore_edit_null = false;
		return;
	}
}

void VoxelGraphEditorPlugin::_on_graph_editor_nothing_selected() {
	// 不幸的是，检查器被设计得像单例一样，所以当我们选中节点来编辑其属性时，
	// 就无法再访问图形资源本身（例如保存它）。我想把
	// 属性嵌入到节点自身内部，但这需要更多工作（而且浪费空间），而且 Godot 不暴露
	// EditorInspector，否则就可以在图形编辑器中拥有一个次级检查器。所以目前我让
	// 取消选中图形中的所有节点（比如点击背景）时选中图形。
	inspect_graph_or_generator(*_graph_editor);
}

void VoxelGraphEditorPlugin::_on_graph_editor_nodes_deleted() {
	// 删除节点时，被选中的节点可能在其中，但检查器包装器仍会指向它。
	// 清理它，并改为检查图形本身。
	inspect_graph_or_generator(*_graph_editor);
}

template <typename F>
void for_each_node(Node *parent, F action) {
	action(parent);
	for (int i = 0; i < parent->get_child_count(); ++i) {
		for_each_node(parent->get_child(i), action);
	}
}

void VoxelGraphEditorPlugin::_on_graph_editor_regenerate_requested() {
	// 我们可能在没有加载地形的情况下独立编辑图形
	VoxelNode *terrain_node = _voxel_node.get();
	if (terrain_node != nullptr) {
		// 重新生成选中的地形。
		terrain_node->restart_stream();

	} else {
		// 该节点未被选中，但它可能还在场景树中
		Node *root = get_editor_interface()->get_edited_scene_root();

		if (root != nullptr) {
			Ref<VoxelGeneratorGraph> generator = _graph_editor->get_generator();
			ERR_FAIL_COND(generator.is_null());

			for_each_node(root, [&generator](Node *node) {
				VoxelNode *vnode = Object::cast_to<VoxelNode>(node);
				if (vnode != nullptr && vnode->get_generator() == generator) {
					vnode->restart_stream();
				}
			});
		}
	}
}

void VoxelGraphEditorPlugin::_on_graph_editor_popout_requested() {
	undock_graph_editor();
}

void VoxelGraphEditorPlugin::_on_graph_editor_window_close_requested() {
	dock_graph_editor();
}

void VoxelGraphEditorPlugin::_on_generator_changed() {
	// 这是为了绕过一个事实：如果在自定义编辑器中通过 UndoRedoManager 编辑某个内嵌在资源 A 里的内置资源 B，
	// Godot 不会保存资源 A。
	//
	// 以我的经验，Godot 检测资源是否改变的主要方式就是让它
	// 出现在 UndoRedo 操作中。但如果资源是内置的（嵌入在另一个资源中），它不会随 Ctrl+S 保存，
	// 除非包含它的资源被保存。遗憾的是，到目前为止与我讨论过的 Godot 开发者似乎都没有意识到
	// 在自定义编辑器中这种情况应该如何处理。仿佛这是一个边缘情况，在引擎中要么被某种方式
	// 幸运地绕开了，要么用临时的按情况 hack 处理。
	// 有 `Object::set_edited` 和 `Resource::owners`，它们被用在编辑器的各个地方，但当然
	// 不会暴露给脚本或扩展，而且我也不知道它们如何处理嵌套……
	// 也许我还没有遇到那个懂行的开发者……
	//
	// 如果被编辑的图形嵌套得更深，这个变通方法甚至可能还不够。极端一点，
	// 我们可能不得不递归地追溯包含它的资源，直到找到有文件路径的那一个，并告诉 Godot
	// 它需要被标记为保存，但这做起来极其繁琐，因为资源不像节点那样有“父级”
	// 属性……
	//
	// 参见 https://github.com/godotengine/godot-proposals/discussions/7168
	Ref<VoxelGeneratorGraph> generator = _graph_editor->get_generator();
	if (generator.is_valid()) {
		set_object_edited(**generator);
	}
}

void VoxelGraphEditorPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		init();

	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		for (Ref<VoxelGraphNodeInspectorWrapper> &w : _node_wrappers) {
			ERR_CONTINUE(w.is_null());
			w->detach_from_graph_editor();
		}
	}
}

void VoxelGraphEditorPlugin::undock_graph_editor() {
	ERR_FAIL_COND(_graph_editor_window != nullptr);
	VOXEL_PRINT_VERBOSE("Undock voxel graph editor");

	EditorNode::get_bottom_panel()->remove_item(_graph_editor);
	_bottom_panel_button = nullptr;

	_graph_editor->set_popout_button_enabled(false);
	_graph_editor->set_anchors_preset(Control::PRESET_FULL_RECT);
	// 我不知道是什么隐藏了它，但我需要让它再次可见
	_graph_editor->show();

	_graph_editor_window = memnew(VoxelGraphEditorWindow);
	update_graph_editor_window_title();
	_graph_editor_window->add_child(_graph_editor);
	_graph_editor_window->connect(
			"close_requested", callable_mp(this, &VoxelGraphEditorPlugin::_on_graph_editor_window_close_requested)
	);

	Node *base_control = get_editor_interface()->get_base_control();
	base_control->add_child(_graph_editor_window);

	_graph_editor_window->popup_centered_ratio(0.6);
}

void VoxelGraphEditorPlugin::dock_graph_editor() {
	ERR_FAIL_COND(_graph_editor_window == nullptr);
	VOXEL_PRINT_VERBOSE("Dock voxel graph editor");

	_graph_editor->get_parent()->remove_child(_graph_editor);
	_graph_editor_window->queue_free();
	_graph_editor_window = nullptr;

	_graph_editor->set_popout_button_enabled(true);

	_bottom_panel_button = EditorNode::get_bottom_panel()->add_item(VOXEL_TTR("Voxel Graph"), _graph_editor);

	_bottom_panel_button->show();
	make_bottom_panel_item_visible(_graph_editor);
}

void VoxelGraphEditorPlugin::update_graph_editor_window_title() {
	ERR_FAIL_COND(_graph_editor_window == nullptr);

	String res_path;
	String type_name;

	if (_graph_editor->get_generator().is_valid()) {
		res_path = _graph_editor->get_generator()->get_path();
		type_name = VoxelGeneratorGraph::get_class_static();

	} else if (_graph_editor->get_graph().is_valid()) {
		res_path = _graph_editor->get_graph()->get_path();
		type_name = VoxelGraphFunction::get_class_static();
	}

	String title;
	if (!res_path.is_empty()) {
		title = res_path;
		title += " - ";
		title += type_name;
	} else {
		title = "VoxelGraphEditor (no graph opened)";
	}

	_graph_editor_window->set_title(title);
}

void VoxelGraphEditorPlugin::edit_ios(Ref<VoxelGraphFunction> graph) {
	if (_io_dialog == nullptr) {
		_io_dialog = memnew(VoxelGraphEditorIODialog);
		_io_dialog->set_undo_redo(get_undo_redo());
		Control *base_control = get_editor_interface()->get_base_control();
		base_control->add_child(_io_dialog);
	}

	_io_dialog->set_graph(graph);
	_io_dialog->popup_centered();
}

void VoxelGraphEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_hide_deferred"), &VoxelGraphEditorPlugin::_hide_deferred);
}

} // namespace voxel
