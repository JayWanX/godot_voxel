#include "voxel_graph_editor_node.h"
#include "../../generators/graph/node_type_db.h"
#include "../../generators/graph/voxel_generator_graph.h"
#include <scene/gui/box_container.h>
#include <scene/gui/label.h>
#include "../../util/godot/classes/node.h"
#include <scene/resources/style_box.h>
#include <core/variant/array.h>
#include "../../util/godot/core/string_name.h"
#include <core/version.h>
#include <core/version.h>
#include <editor/themes/editor_scale.h>
#include <core/math/color.h>
#include "../../util/math/color.h"
#include "graph_editor_adapter.h"
#include "voxel_graph_editor_node_preview.h"

namespace voxel {

using namespace pg;

static const Color PORT_COLOR(0.4, 0.4, 1.0);

VoxelGraphEditorNode *VoxelGraphEditorNode::create(const VoxelGraphFunction &graph, uint32_t node_id) {
	VoxelGraphEditorNode *node_view = memnew(VoxelGraphEditorNode);
	node_view->set_position_offset(graph.get_node_gui_position(node_id) * EDSCALE);

	// 不要翻译标题，它显示的是节点的名称
	{
		Node *titlebar = node_view->get_titlebar_hbox();
		if (titlebar != nullptr) {
			set_node_auto_translate_mode(*titlebar, voxel::godot::AUTO_TRANSLATE_MODE_DISABLED);
		} else {
			VOXEL_PRINT_ERROR("Title bar is null?");
		}
	}

	node_view->update_title(graph, node_id);

	node_view->_node_id = node_id;

	const uint32_t node_type_id = graph.get_node_type_id(node_id);
	const bool is_comment = node_type_id == VoxelGraphFunction::NODE_COMMENT;
	const bool is_resizable = node_type_id == VoxelGraphFunction::NODE_EXPRESSION || is_comment;

	node_view->_is_relay = node_type_id == VoxelGraphFunction::NODE_RELAY;

	// 某些节点可以具有可变大小的标题和布局。节点可以自动变大，但不会缩小。
	// 因此目前我们让它们可调整大小，以便用户自行调整。
	if (is_resizable) {
		node_view->set_resizable(is_resizable);

		const Vector2 node_size = graph.get_node_gui_size(node_id) * EDSCALE;
		node_view->set_size(node_size);
	}
	// node_view.rect_size = Vector2(200, 100)

	node_view->_is_comment = is_comment;

	// TODO GraphEdit 在 Godot 4.2 中正在重构，注释已不可用

	node_view->update_layout(graph);

	if (node_type_id == VoxelGraphFunction::NODE_SDF_PREVIEW) {
		node_view->_preview = memnew(VoxelGraphEditorNodePreview);
		node_view->add_child(node_view->_preview);
	}

	return node_view;
}

void VoxelGraphEditorNode::update_layout(const VoxelGraphFunction &graph) {
	const uint32_t node_type_id = graph.get_node_type_id(_node_id);
	const NodeType &node_type = NodeTypeDB::get_singleton().get_type(node_type_id);
	// 如果节点是输出节点，我们人为地隐藏输出端口。
	// 这些节点出于实现原因拥有输出，某些输出可以像任何其他节点一样处理数据。
	const bool hide_outputs = node_type.category == CATEGORY_OUTPUT;

	struct Input {
		String name;
	};
	struct Output {
		String name;
	};
	StdVector<Input> inputs;
	StdVector<Output> outputs;
	{
		const unsigned int input_count = graph.get_node_input_count(_node_id);
		const unsigned int output_count = graph.get_node_output_count(_node_id);

		for (unsigned int i = 0; i < input_count; ++i) {
			Input input;
			graph.get_node_input_info(_node_id, i, &input.name, nullptr);
			inputs.push_back(input);
		}
		for (unsigned int i = 0; i < output_count; ++i) {
			Output output;
			output.name = graph.get_node_output_name(_node_id, i);
			outputs.push_back(output);
		}
	}

	const unsigned int row_count = math::max(inputs.size(), hide_outputs ? 0 : outputs.size());
	const Color hint_label_modulate(0.6, 0.6, 0.6);

	// const int middle_min_width = EDSCALE * 32.0;

	// 临时移除预览（如果有）
	if (_preview != nullptr) {
		remove_child(_preview);
	}

	if (_comment_label != nullptr) {
		_comment_label->queue_free();
		_comment_label = nullptr;
	}

	// 清除先前的输入和输出
	for (Node *row : _rows) {
		remove_child(row);
		row->queue_free();
	}
	_rows.clear();
	_output_labels.clear();

	clear_all_slots();

	_input_hints.clear();

	const bool is_relay = (node_type_id == VoxelGraphFunction::NODE_RELAY);
	// 无法移除帧样式，它会破坏与节点的交互...
	// if (is_relay) {
	// 	Ref<StyleBoxEmpty> sb;
	// 	sb.instantiate();
	// 	add_theme_style_override("frame", sb);
	// } else {
	// 	remove_theme_style_override("frame");
	// }

	// 添加输入和输出
	for (unsigned int slot_index = 0; slot_index < row_count; ++slot_index) {
		const bool has_left = slot_index < inputs.size();
		const bool has_right = (slot_index < outputs.size()) && !hide_outputs;

		HBoxContainer *property_control = memnew(HBoxContainer);
		property_control->set_custom_minimum_size(Vector2(0, 24 * EDSCALE));
		property_control->set_mouse_filter(Control::MOUSE_FILTER_PASS);

		if (has_left && !is_relay) {
			Label *label = memnew(Label);
			label->set_text(inputs[slot_index].name);
			property_control->add_child(label);

			Label *hint_label = memnew(Label);
			hint_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			hint_label->set_modulate(hint_label_modulate);
			// 需要通过过滤器（pass）才能让工具提示工作
			hint_label->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			// hint_label->set_clip_text(true);
			// hint_label->set_custom_minimum_size(Vector2(middle_min_width, 0));
			property_control->add_child(hint_label);
			VoxelGraphEditorNode::InputHint input_hint;
			input_hint.label = hint_label;
			_input_hints.push_back(input_hint);
		}

		if (has_right && !is_relay) {
			if (property_control->get_child_count() < 2) {
				Control *spacer = memnew(Control);
				spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				// spacer->set_custom_minimum_size(Vector2(middle_min_width, 0));
				property_control->add_child(spacer);
			}

			Label *label = memnew(Label);
			label->set_text(outputs[slot_index].name);
			// 需要通过过滤器（pass）才能让工具提示工作
			label->set_mouse_filter(Control::MOUSE_FILTER_PASS);
			property_control->add_child(label);

			_output_labels.push_back(label);
		}

		add_child(property_control);
		set_slot(slot_index, has_left, Variant::FLOAT, PORT_COLOR, has_right, Variant::FLOAT, PORT_COLOR);
		_rows.push_back(property_control);
	}

	// 重新添加预览（如果有）
	if (_preview != nullptr) {
		add_child(_preview);
	}

	if (_is_comment) {
		_comment_label = memnew(Label);
		add_child(_comment_label);
		_comment_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		_comment_label->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		update_comment_text(graph);
	}
}

void VoxelGraphEditorNode::update_comment_text(const VoxelGraphFunction &graph) {
	ERR_FAIL_COND(_comment_label == nullptr);
	const String text = graph.get_node_param(_node_id, 0);
	_comment_label->set_text(text);
}

void VoxelGraphEditorNode::update_title(const VoxelGraphFunction &graph) {
	update_title(graph, _node_id);
}

void VoxelGraphEditorNode::update_title(const VoxelGraphFunction &graph, uint32_t node_id) {
	const VoxelGraphFunction::NodeTypeID type_id = graph.get_node_type_id(node_id);
	if (type_id == VoxelGraphFunction::NODE_RELAY) {
		// 中继节点没有标题栏
		return;
	}
	const NodeType &type = NodeTypeDB::get_singleton().get_type(type_id);
	const String node_name = graph.get_node_name(node_id);

	if (type_id == VoxelGraphFunction::NODE_FUNCTION) {
		Ref<VoxelGraphFunction> func = graph.get_node_param(node_id, 0);
		ERR_FAIL_COND(func.is_null());
		String fname = func->get_path();
		fname = fname.get_file();
		if (voxel::godot::is_empty(node_name)) {
			set_title(fname);
		} else {
			set_title(String("{0} ({1})").format(varray(node_name, fname)));
		}

	} else if (voxel::godot::is_empty(node_name)) {
		set_title(type.name);

	} else if (type_id == VoxelGraphFunction::NODE_COMMENT) {
		set_title(String(node_name));

	} else {
		set_title(String("{0} ({1})").format(varray(node_name, type.name)));
	}
}

void VoxelGraphEditorNode::poll(const VoxelGraphFunction &graph) {
	poll_default_inputs(graph);
	poll_params(graph);
}

// 当输入未连接时，它会采用默认值。输入提示会显示这个值。
// 在节点被选中时它也会显示在检查器中，但一眼看到它们会更有帮助。
void VoxelGraphEditorNode::poll_default_inputs(const VoxelGraphFunction &graph) {
	ProgramGraph::PortLocation src_loc_unused;
	const String prefix = ": ";

	for (unsigned int input_index = 0; input_index < _input_hints.size(); ++input_index) {
		VoxelGraphEditorNode::InputHint &input_hint = _input_hints[input_index];
		const ProgramGraph::PortLocation loc{ _node_id, input_index };

		if (graph.try_get_connection_to(loc, src_loc_unused)) {
			// 存在入站连接，不显示默认值
			if (input_hint.last_value != Variant()) {
				input_hint.label->set_text("");
				input_hint.last_value = Variant();
			}

		} else {
			if (graph.get_node_default_inputs_autoconnect(loc.node_id)) {
				// const VoxelGraphFunction::NodeTypeID node_type_id = graph.get_node_type_id(loc.node_id);
				// const NodeType &node_type =
				// NodeTypeDB::get_singleton().get_type(node_type_id); const NodeType::Port &input_port =
				// node_type.inputs[input_index]; const VoxelGraphFunction::AutoConnect auto_connect =
				// input_port.auto_connect;
				VoxelGraphFunction::AutoConnect auto_connect;
				graph.get_node_input_info(loc.node_id, loc.port_index, nullptr, &auto_connect);
				if (auto_connect != VoxelGraphFunction::AUTO_CONNECT_NONE) {
					Variant value;
					switch (auto_connect) {
						case VoxelGraphFunction::AUTO_CONNECT_X:
							value = "Auto X";
							break;
						case VoxelGraphFunction::AUTO_CONNECT_Y:
							value = "Auto Y";
							break;
						case VoxelGraphFunction::AUTO_CONNECT_Z:
							value = "Auto Z";
							break;
						default:
							ERR_PRINT("Unhandled autoconnect");
							value = int(auto_connect);
							break;
					}
					if (input_hint.last_value != value) {
						input_hint.label->set_text(prefix + value.stringify());
						input_hint.last_value = value;
					}
					continue;
				}
			}
			// 既没有入站连接也没有自动连接，显示默认值
			const Variant current_value = graph.get_node_default_input(loc.node_id, loc.port_index);
			// 只在值变化时更新，以免刷爆编辑器重绘
			if (input_hint.last_value != current_value) {
				String s;
				String tooltip;
				if (current_value.get_type() == Variant::FLOAT) {
					// 转换为 float，因为即使在双精度构建中，图形底层实际使用的也是 float
					const float fv = current_value;
					// 四舍五入小数，否则像 `0.2` 这样的值会被格式化为
					// `0.19999999999709`，这会使节点变宽，非常烦人。
					String fs = String(Variant(fv));
					s = String::num(fv, 7);
					if (fs != s) {
						s += "*";
						tooltip = fs;
					}
				}

				input_hint.label->set_text(prefix + s);
				input_hint.label->set_tooltip_text(tooltip);
				input_hint.last_value = current_value;
			}
		}
	}
}

void VoxelGraphEditorNode::poll_params(const VoxelGraphFunction &graph) {
	if (graph.get_node_type_id(_node_id) == VoxelGraphFunction::NODE_EXPRESSION) {
		const String code = graph.get_node_param(_node_id, 0);
		set_title(code);
	}
}

void VoxelGraphEditorNode::update_range_analysis_tooltips(
		const GraphEditorAdapter &adapter,
		const pg::Runtime::State &state,
		const StdUnorderedMap<uint32_t, math::Interval> &actual_ranges
) {
	for (unsigned int port_index = 0; port_index < _output_labels.size(); ++port_index) {
		ProgramGraph::PortLocation loc;
		loc.node_id = get_generator_node_id();
		loc.port_index = port_index;
		uint32_t address;
		if (!adapter.try_get_output_port_address(loc, address)) {
			continue;
		}
		const math::Interval range = state.get_range(address);
		String text = String("Min: {0}\nMax: {1}").format(varray(range.min, range.max));

		const auto actual_range_it = actual_ranges.find(address);
		if (actual_range_it != actual_ranges.end()) {
			const math::Interval actual_range = actual_range_it->second;

			text += String("\nActual Min: {0}").format(varray(actual_range.min));
			const float min_err = range.min - actual_range.min;
			if (min_err > 0.f) {
				text += String(" | ERROR: {0}").format(varray(min_err));
			}

			text += String("\nActual Max: {0}").format(varray(actual_range.max));
			const float max_err = actual_range.max - range.max;
			if (max_err > 0.f) {
				text += String(" | ERROR: {0}").format(varray(max_err));
			}
		}

		Control *label = _output_labels[port_index];
		label->set_tooltip_text(text);
	}
}

void VoxelGraphEditorNode::clear_range_analysis_tooltips() {
	for (unsigned int i = 0; i < _output_labels.size(); ++i) {
		Control *oc = _output_labels[i];
		oc->set_tooltip_text("");
	}
}

void VoxelGraphEditorNode::set_profiling_ratio_visible(bool p_visible) {
	if (_profiling_ratio_enabled == p_visible) {
		return;
	}
	_profiling_ratio_enabled = p_visible;
	queue_redraw();
}

void VoxelGraphEditorNode::set_profiling_ratio(float ratio) {
	if (_profiling_ratio == ratio) {
		return;
	}
	_profiling_ratio = ratio;
	queue_redraw();
}

void VoxelGraphEditorNode::_notification(int p_what) {
	using namespace voxel::godot;

	if (p_what == NOTIFICATION_DRAW) {
		if (_is_relay) {
			// 绘制线条以表示数据被直接中继
			// TODO 线宽和抗锯齿应该来自 GraphEdit
			const float width = Math::floor(2.f * get_theme_default_base_scale());
			// 不能直接使用输入和输出的位置……Godot 会预先缩放它们，导致它们无法
			// 用于绘制，因为节点本身已经缩放
			const Vector2 input_pos = get_graph_node_input_port_position(*this, 0);
			const Vector2 output_pos = get_graph_node_output_port_position(*this, 0);
			draw_line(input_pos, output_pos, get_graph_node_input_port_color(*this, 0), width, true);
		}
		if (_profiling_ratio_enabled) {
			const float bgh = EDSCALE * 4.f;
			const Vector2 control_size = get_size();
			const float bgw = control_size.x;
			const Color bg_color(0.1, 0.1, 0.1);
			const Color fg_color = math::lerp(Color(0.8, 0.8, 0.0), Color(1.0, 0.2, 0.0), _profiling_ratio);
			draw_rect(Rect2(0, control_size.y - bgh, bgw, bgh), bg_color);
			draw_rect(Rect2(0, control_size.y - bgh, bgw * _profiling_ratio, bgh), fg_color);
		}
	}
}

void VoxelGraphEditorNode::_bind_methods() {}

} // namespace voxel
