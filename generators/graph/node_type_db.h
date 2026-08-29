#ifndef VOXEL_GRAPH_NODE_TYPE_DB_H
#define VOXEL_GRAPH_NODE_TYPE_DB_H

#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/core/string.h" // 供 String 哈希使用
#include "../../util/string/expression_parser.h"
#include "../../util/string/std_string.h"
#include "voxel_graph_compiler.h"
#include "voxel_graph_function.h"
#include "voxel_graph_shader_generator.h"

namespace voxel::pg {

enum Category {
	CATEGORY_INPUT = 0,
	CATEGORY_OUTPUT,
	CATEGORY_MATH,
	CATEGORY_CONVERT,
	CATEGORY_GENERATE,
	CATEGORY_SDF,
	CATEGORY_DEBUG,
	CATEGORY_FUNCTIONS,
	CATEGORY_RELAY,
	CATEGORY_CONSTANT,
	CATEGORY_COUNT
};

const char *get_category_name(Category category);

struct NodeType {
	// TODO 将输入和输出端口类型分开？某些成员值没有意义。
	struct Port {
		String name;
		// 仅与输入相关。
		float default_value;
		// 当输入端口未连接且未显式指定固定值时，将自动建立的连接。仅与输入相关。
		VoxelGraphFunction::AutoConnect auto_connect = VoxelGraphFunction::AUTO_CONNECT_NONE;
		// 如果为 true，即使值被确定为常量，也会提供缓冲区作为输入。
		// 如果为 false，则不提供缓冲区，值将改为在 Buffer::constant_value 中可用。
		// 该选项的存在是为了避免在输入缓冲区中实现常量值与变量值的所有可能组合。
		bool require_input_buffer_when_constant = true;
		// PortType port_type;

		Port(String p_name,
			 float p_default_value = 0.f,
			 VoxelGraphFunction::AutoConnect ac = VoxelGraphFunction::AUTO_CONNECT_NONE,
			 bool p_require_input_buffer_when_constant = true) :
				name(p_name),
				default_value(p_default_value),
				auto_connect(ac),
				require_input_buffer_when_constant(p_require_input_buffer_when_constant) {}
	};

	typedef Variant (*DefaultValueFactory)();

	struct Param {
		String name;
		Variant default_value;
		DefaultValueFactory default_value_func;
		Variant::Type type;
		String class_name;
		uint32_t index = -1;
		bool has_range = false;
		bool multiline = false;
		bool hidden = false;
		Variant min_value;
		Variant max_value;
		StdVector<StdString> enum_items;

		Param(String p_name, Variant::Type p_type, Variant p_default_value = Variant()) :
				name(p_name), default_value(p_default_value), type(p_type) {}

		Param(String p_name, String p_class_name, DefaultValueFactory dvf) :
				name(p_name), default_value_func(dvf), type(Variant::OBJECT), class_name(p_class_name) {}
	};

	String name;
	// 仅调试用的节点在非调试编译时被忽略。
	bool debug_only = false;
	// 伪节点在编译期间被替换为一个或多个真实节点，它们本身没有逻辑
	bool is_pseudo_node = false;
	Category category;
	StdVector<Port> inputs;
	StdVector<Port> outputs;
	StdVector<Param> params;
	StdUnorderedMap<String, uint32_t> param_name_to_index;
	StdUnorderedMap<String, uint32_t> input_name_to_index;
	CompileFunc compile_func = nullptr;
	Runtime::ProcessBufferFunc process_buffer_func = nullptr;
	Runtime::RangeAnalysisFunc range_analysis_func = nullptr;
	// 如果有的话，用于表达式节点中对应函数的名称
	const char *expression_func_name = nullptr;
	// Expression 节点可以调用其它节点的逻辑，但它需要一个特定的实现
	ExpressionParser::FunctionCallback expression_func = nullptr;
	ShaderGenFunc shader_gen_func = nullptr;

	inline bool has_autoconnect_inputs() const {
		for (const Port &port : inputs) {
			if (port.auto_connect != VoxelGraphFunction::AUTO_CONNECT_NONE) {
				return true;
			}
		}
		return false;
	}
};

class NodeTypeDB {
public:
	NodeTypeDB();

	static const NodeTypeDB &get_singleton();
	static void create_singleton();
	static void destroy_singleton();

	int get_type_count() const {
		return _types.size();
	}
	bool is_valid_type_id(int type_id) const {
		return type_id >= 0 && type_id < static_cast<int>(_types.size());
	}
	const NodeType &get_type(uint32_t id) const {
		return _types[id];
	}
	Dictionary get_type_info_dict(uint32_t id) const;
	bool try_get_type_id_from_name(const String &name, VoxelGraphFunction::NodeTypeID &out_type_id) const;
	bool try_get_param_index_from_name(uint32_t type_id, const String &name, uint32_t &out_param_index) const;
	bool try_get_input_index_from_name(uint32_t type_id, const String &name, uint32_t &out_input_index) const;
	bool try_get_output_index_from_name(const uint32_t type_id, const String &name, uint32_t &out_output_index) const;

	Span<const ExpressionParser::Function> get_expression_parser_functions() const {
		return to_span(_expression_functions);
	}

private:
	FixedArray<NodeType, VoxelGraphFunction::NODE_TYPE_COUNT> _types;
	StdUnorderedMap<String, VoxelGraphFunction::NodeTypeID> _type_name_to_id;
	StdVector<ExpressionParser::Function> _expression_functions;
};

VoxelGraphFunction::Port make_port_from_io_node(const ProgramGraph::Node &node, const NodeType &type);
bool is_node_matching_port(const ProgramGraph::Node &node, const VoxelGraphFunction::Port &port);

} // namespace voxel::pg

#endif // VOXEL_GRAPH_NODE_TYPE_DB_H
