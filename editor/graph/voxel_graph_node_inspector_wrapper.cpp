#include "voxel_graph_node_inspector_wrapper.h"
#include "../../constants/voxel_string_names.h"
#include "../../generators/graph/node_type_db.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/core/array.h"
#include "../../util/io/log.h"
#include "../../util/string/std_string.h"
#include "voxel_graph_editor.h"

#include <algorithm>

namespace voxel {

using namespace pg;
using namespace voxel::godot;

namespace {
const char *AUTOCONNECT_PROPERTY_NAME = "autoconnect_default_inputs";
}

void VoxelGraphNodeInspectorWrapper::setup(uint32_t p_node_id, VoxelGraphEditor *ed) {
	VOXEL_ASSERT(ed != nullptr);
	_graph = ed->get_graph();
	_generator = ed->get_generator();
	_node_id = p_node_id;
	_graph_editor = ed;
}

void VoxelGraphNodeInspectorWrapper::detach_from_graph_editor() {
	_graph = Ref<VoxelGraphFunction>();
	_generator = Ref<VoxelGeneratorGraph>();
	_graph_editor = nullptr;
}

void VoxelGraphNodeInspectorWrapper::_get_property_list(List<PropertyInfo> *p_list) const {
	Ref<VoxelGraphFunction> graph = get_graph();
	ERR_FAIL_COND(graph.is_null());

	if (!graph->has_node(_node_id)) {
		// 也许被用户删除了？
#ifdef DEBUG_ENABLED
		VOXEL_PRINT_VERBOSE("VoxelGeneratorGraph node was not found, from the graph inspector");
#endif
		return;
	}

	p_list->push_back(PropertyInfo(Variant::STRING_NAME, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR));

	// Params
	{
		const uint32_t node_type_id = graph->get_node_type_id(_node_id);
		const NodeType &node_type = NodeTypeDB::get_singleton().get_type(node_type_id);

		p_list->push_back(PropertyInfo(Variant::NIL, "Params", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CATEGORY));

		for (const NodeType::Param &param : node_type.params) {
			if (param.hidden) {
				continue;
			}
			PropertyInfo pi;
			pi.name = param.name;
			pi.type = param.type;
			pi.class_name = param.class_name;

			if (!param.class_name.is_empty()) {
				pi.hint = PROPERTY_HINT_RESOURCE_TYPE;
				pi.hint_string = pi.class_name;

			} else if (param.has_range) {
				pi.hint = PROPERTY_HINT_RANGE;
				pi.hint_string = String("{0},{1}").format(varray(param.min_value, param.max_value));

			} else if (pi.type == Variant::STRING) {
				if (param.multiline) {
					pi.hint = PROPERTY_HINT_MULTILINE_TEXT;
				}

			} else if (param.enum_items.size() > 0) {
				StdString hint_string;
				for (unsigned int item_index = 0; item_index < param.enum_items.size(); ++item_index) {
					if (item_index > 0) {
						hint_string += ",";
					}
					hint_string += param.enum_items[item_index];
				}
				pi.hint_string = to_godot(hint_string);
				pi.hint = PROPERTY_HINT_ENUM;
			}

			pi.usage = PROPERTY_USAGE_EDITOR;
			p_list->push_back(pi);
		}
	}

	// Inputs

	p_list->push_back(PropertyInfo(Variant::NIL, "Input Defaults", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_CATEGORY));

	const bool autoconnect_enabled = graph->get_node_default_inputs_autoconnect(_node_id);
	const unsigned int input_count = graph->get_node_input_count(_node_id);
	bool has_autoconnect_inputs = false;

	for (unsigned int i = 0; i < input_count; ++i) {
		String port_name;
		VoxelGraphFunction::AutoConnect autoconnect_hint;
		graph->get_node_input_info(_node_id, i, &port_name, &autoconnect_hint);

		if (autoconnect_hint != VoxelGraphFunction::AUTO_CONNECT_NONE) {
			has_autoconnect_inputs = true;
		}

		PropertyInfo pi;
		pi.name = port_name;
		// 目前所有 I/O 都是 float。
		pi.type = Variant::FLOAT;
		if (autoconnect_enabled && autoconnect_hint != VoxelGraphFunction::AUTO_CONNECT_NONE) {
			// 这个默认值不会使用，因为编译时端口会自动连接
			pi.usage |= PROPERTY_USAGE_READ_ONLY;
		}
		p_list->push_back(pi);
	}

	// 自动连接

	if (has_autoconnect_inputs) {
		p_list->push_back(PropertyInfo(Variant::BOOL, AUTOCONNECT_PROPERTY_NAME));
	}
}

namespace {

// 根据表达式代码中使用的变量名自动更新输入列表。
// 与 VisualScript（需要用户手动操作）不同，提交包含
// 表达式代码的文本字段也会改变节点的动态输入并重新连接现有连接，所有这些作为一个
// UndoRedo 操作。
void update_expression_inputs(
		VoxelGraphFunction &graph,
		uint32_t node_id,
		String code,
		EditorUndoRedoManager &ur,
		VoxelGraphEditor &graph_editor
) {
	//
	const CharString code_utf8 = code.utf8();
	StdVector<std::string_view> new_input_names;
	if (!VoxelGraphFunction::get_expression_variables(code_utf8.get_data(), new_input_names)) {
		// 出错，操作将不会包含节点输入的变化
		return;
	}
	StdVector<StdString> old_input_names;
	graph.get_expression_node_inputs(node_id, old_input_names);

	struct Connection {
		ProgramGraph::PortLocation src;
		uint32_t dst_port_index;
	};
	// 找出我们将要断开的连接
	StdVector<Connection> to_disconnect;
	for (uint32_t port_index = 0; port_index < old_input_names.size(); ++port_index) {
		ProgramGraph::PortLocation src;
		if (graph.try_get_connection_to({ node_id, port_index }, src)) {
			to_disconnect.push_back({ { src.node_id, src.port_index }, port_index });
		}
	}
	// 找出我们将要重连的连接
	StdVector<Connection> to_reconnect;
	for (uint32_t port_index = 0; port_index < old_input_names.size(); ++port_index) {
		const std::string_view old_name = old_input_names[port_index];
		auto new_input_name_it = std::find(new_input_names.begin(), new_input_names.end(), old_name);
		if (new_input_name_it != new_input_names.end()) {
			ProgramGraph::PortLocation src;
			if (graph.try_get_connection_to({ node_id, port_index }, src)) {
				const uint32_t dst_port_index = new_input_name_it - new_input_names.begin();
				to_reconnect.push_back({ src, dst_port_index });
			}
		}
	}

	// Do

	for (size_t i = 0; i < to_disconnect.size(); ++i) {
		const Connection con = to_disconnect[i];
		ur.add_do_method(&graph, "remove_connection", con.src.node_id, con.src.port_index, node_id, con.dst_port_index);
	}

	ur.add_do_method(&graph, "set_expression_node_inputs", node_id, to_godot(new_input_names));

	for (size_t i = 0; i < to_reconnect.size(); ++i) {
		const Connection con = to_reconnect[i];
		ur.add_do_method(&graph, "add_connection", con.src.node_id, con.src.port_index, node_id, con.dst_port_index);
	}

	// Undo

	for (size_t i = 0; i < to_reconnect.size(); ++i) {
		const Connection con = to_reconnect[i];
		ur.add_undo_method(
				&graph, "remove_connection", con.src.node_id, con.src.port_index, node_id, con.dst_port_index
		);
	}

	ur.add_undo_method(&graph, "set_expression_node_inputs", node_id, to_godot(old_input_names));

	for (size_t i = 0; i < to_disconnect.size(); ++i) {
		const Connection con = to_disconnect[i];
		ur.add_undo_method(&graph, "add_connection", con.src.node_id, con.src.port_index, node_id, con.dst_port_index);
	}

	ur.add_do_method(&graph_editor, "update_node_layout", node_id);
	ur.add_undo_method(&graph_editor, "update_node_layout", node_id);
}

} // namespace

bool VoxelGraphNodeInspectorWrapper::_set(const StringName &p_name, const Variant &p_value) {
	Ref<VoxelGraphFunction> graph = get_graph();
	ERR_FAIL_COND_V(graph.is_null(), false);
	ERR_FAIL_COND_V(_graph_editor == nullptr, false);
	// 我们不能在对象中保存对 UndoRedo 的引用，因为我们的对象可能被 UndoRedo 引用，那
	// 会造成循环引用。因此我们通过对编辑器的弱引用来访问它。
	EditorUndoRedoManager *undo_redo = _graph_editor->get_undo_redo();
	ERR_FAIL_COND_V(undo_redo == nullptr, false);
	EditorUndoRedoManager &ur = *undo_redo;

	const String name = p_name;

	// 特殊情况，因为 `name` 既不是参数也不是输出
	if (name == "name") {
		String previous_name = graph->get_node_name(_node_id);
		ur.create_action("Set VoxelGeneratorGraph node name");
		ur.add_do_method(graph.ptr(), "set_node_name", _node_id, p_value);
		ur.add_undo_method(graph.ptr(), "set_node_name", _node_id, previous_name);
		// ur->add_do_method(this, "notify_property_list_changed");
		// ur->add_undo_method(this, "notify_property_list_changed");
		ur.commit_action();
		return true;
	}

	if (name == AUTOCONNECT_PROPERTY_NAME) {
		const bool prev_autoconnect = graph->get_node_default_inputs_autoconnect(_node_id);
		ur.create_action(String("Set ") + AUTOCONNECT_PROPERTY_NAME);
		ur.add_do_method(graph.ptr(), "set_node_default_inputs_autoconnect", _node_id, p_value);
		ur.add_undo_method(graph.ptr(), "set_node_default_inputs_autoconnect", _node_id, prev_autoconnect);
		// 更新检查器中被禁用的默认输入值
		ur.add_do_method(this, "notify_property_list_changed");
		ur.add_undo_method(this, "notify_property_list_changed");
		ur.commit_action();
		return true;
	}

	uint32_t index;

	if (graph->get_node_param_index_by_name(_node_id, p_name, index)) {
		Variant previous_value = graph->get_node_param(_node_id, index);
		ur.create_action("Set VoxelGeneratorGraph node parameter");
		ur.add_do_method(graph.ptr(), "set_node_param", _node_id, index, p_value);
		ur.add_undo_method(graph.ptr(), "set_node_param", _node_id, index, previous_value);

		const VoxelGraphFunction::NodeTypeID node_type_id = _graph->get_node_type_id(_node_id);

		if (node_type_id == VoxelGraphFunction::NODE_EXPRESSION) {
			update_expression_inputs(**graph, _node_id, p_value, ur, *_graph_editor);
			// TODO 添加变量后无法设置默认输入！
			// 它需要调用 `notify_property_list_changed`，但这会导致检查器中的 LineEdit
			// 重置光标位置，使字符串参数编辑成为一场噩梦。唯一的变通方法是取消选中
			// 再重新选中节点...
		} else if (node_type_id == VoxelGraphFunction::NODE_COMMENT) {
			ur.add_do_method(_graph_editor, "update_node_comment", _node_id);
			ur.add_undo_method(_graph_editor, "update_node_comment", _node_id);
		} else {
			ur.add_do_method(this, "notify_property_list_changed");
			ur.add_undo_method(this, "notify_property_list_changed");
		}

		ur.commit_action();

	} else if (graph->get_node_input_index_by_name(_node_id, p_name, index)) {
		Variant previous_value = graph->get_node_default_input(_node_id, index);
		ur.create_action("Set VoxelGeneratorGraph node default input");
		ur.add_do_method(graph.ptr(), "set_node_default_input", _node_id, index, p_value);
		ur.add_undo_method(graph.ptr(), "set_node_default_input", _node_id, index, previous_value);
		ur.add_do_method(this, "notify_property_list_changed");
		ur.add_undo_method(this, "notify_property_list_changed");
		ur.commit_action();

	} else {
		ERR_PRINT(String("Invalid param name {0}").format(varray(p_name)));
		return false;
	}

	return true;
}

bool VoxelGraphNodeInspectorWrapper::_get(const StringName &p_name, Variant &r_ret) const {
	Ref<VoxelGraphFunction> graph = get_graph();
	ERR_FAIL_COND_V(graph.is_null(), false);

	const String name = p_name;

	if (name == "name") {
		r_ret = graph->get_node_name(_node_id);
		return true;
	}

	if (name == AUTOCONNECT_PROPERTY_NAME) {
		r_ret = graph->get_node_default_inputs_autoconnect(_node_id);
		return true;
	}

	unsigned int index;
	if (graph->get_node_param_index_by_name(_node_id, p_name, index)) {
		r_ret = graph->get_node_param(_node_id, index);
		return true;
	}

	if (graph->get_node_input_index_by_name(_node_id, p_name, index)) {
		r_ret = graph->get_node_default_input(_node_id, index);
		return true;
	}

	// 不能那样做错误检查，Godot 有时会在编辑器中为未知属性刷屏（比如 `script`），
	// 而且毫无理由（比如每帧都刷，即使检查器中不可见）
	// ERR_PRINT(String("Invalid param name {0}").format(varray(p_name)));

	return false;
}

// 这是 `EditorInspector::_edit_set` 中使用的一个没有文档说明的 hack，让我们能够自己实现 UndoRedo。
// 如果我们不这样做，检查器的 UndoRedo 就会使用包装器，这样就不会把真正的资源标记为
// 已修改。
bool VoxelGraphNodeInspectorWrapper::_dont_undo_redo() const {
	return true;
}

void VoxelGraphNodeInspectorWrapper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_dont_undo_redo"), &VoxelGraphNodeInspectorWrapper::_dont_undo_redo);
}

} // namespace voxel
