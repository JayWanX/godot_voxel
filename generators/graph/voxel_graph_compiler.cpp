#include "voxel_graph_compiler.h"
#include "../../util/containers/container_funcs.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_unordered_set.h"
#include <core/variant/array.h>
#include "../../util/macros.h"
#include "../../util/profiling.h"
#include "../../util/string/expression_parser.h"
#include "../../util/string/format.h"
#include "node_type_db.h"
#include "voxel_graph_function.h"

#include <limits>

namespace voxel::pg {

namespace {

// 更新重映射，用于将一个节点替换为另一个节点，其中旧节点和新节点的输出数量相同。
void add_remap(GraphRemappingInfo &remaps, uint32_t old_node_id, uint32_t new_node_id, unsigned int output_count) {
	bool existing_remap = false;
	// 修改已有条目，以防旧节点是图先前处理的结果。
	for (PortRemap &remap : remaps.user_to_expanded_ports) {
		if (remap.expanded.node_id == old_node_id) {
			remap.expanded.node_id = new_node_id;
			existing_remap = true;
		}
	}
	for (ExpandedNodeRemap &remap : remaps.expanded_to_user_node_ids) {
		if (remap.expanded_node_id == old_node_id) {
			remap.expanded_node_id = new_node_id;
		}
	}
	if (!existing_remap) {
		// 旧节点不是图先前处理的结果。
		// 因此可以添加新条目。
		for (uint32_t output_index = 0; output_index < output_count; ++output_index) {
			PortRemap port_remap;
			port_remap.original = ProgramGraph::PortLocation{ old_node_id, output_index };
			port_remap.expanded = ProgramGraph::PortLocation{ new_node_id, output_index };
			remaps.user_to_expanded_ports.push_back(port_remap);

			remaps.expanded_to_user_node_ids.push_back(ExpandedNodeRemap{ new_node_id, old_node_id });
		}
	}
}

// 更新重映射，用于将一个节点替换为多个节点。原始节点的输出可以成为不同展开节点的输出。
// 如果某个输出无效或没有连接（因此不存在），则 PortLocation::node_id 为 NULL_ID。
void add_remap(
		GraphRemappingInfo &remaps,
		uint32_t old_node_id,
		Span<const uint32_t> new_node_ids,
		Span<const ProgramGraph::PortLocation> output_locations
) {
	VOXEL_ASSERT(old_node_id != ProgramGraph::NULL_ID);
	// 为输出端口添加重映射
	{
		bool found = false;
		for (PortRemap &pr : remaps.user_to_expanded_ports) {
			if (pr.expanded.node_id == old_node_id) {
				// 旧节点是先前展开的结果
				pr.expanded = output_locations[pr.expanded.port_index];
				found = true;
			}
		}
		if (!found) {
			for (unsigned int output_index = 0; output_index < output_locations.size(); ++output_index) {
				PortRemap pr;
				pr.original.node_id = old_node_id;
				pr.original.port_index = output_index;
				pr.expanded = output_locations[output_index];
				remaps.user_to_expanded_ports.push_back(pr);
			}
		}
	}
	// 为节点添加重映射
	{
		uint32_t original_node_id = old_node_id;
		for (const ExpandedNodeRemap &nr : remaps.expanded_to_user_node_ids) {
			if (nr.expanded_node_id == old_node_id) {
				// 旧节点本身是展开后的节点，回溯到原始节点
				original_node_id = nr.original_node_id;
				break;
			}
		}
		for (const uint32_t expanded_node_id : new_node_ids) {
			remaps.expanded_to_user_node_ids.push_back({ expanded_node_id, original_node_id });
		}
	}
}

inline void add_remap(
		GraphRemappingInfo &remaps,
		uint32_t old_node_id,
		Span<const uint32_t> new_node_ids,
		const ProgramGraph::PortLocation output_location
) {
	add_remap(remaps, old_node_id, new_node_ids, Span<const ProgramGraph::PortLocation>(&output_location, 1));
}

uint32_t get_original_node_id(const GraphRemappingInfo &remaps, uint32_t expanded_node_id) {
	for (const ExpandedNodeRemap &enr : remaps.expanded_to_user_node_ids) {
		if (enr.expanded_node_id == expanded_node_id) {
			return enr.original_node_id;
		}
	}
	return expanded_node_id;
}

struct ToConnect {
	std::string_view var_name;
	ProgramGraph::PortLocation dst;
};

uint32_t expand_node(
		ProgramGraph &graph,
		const ExpressionParser::Node &ep_node,
		const NodeTypeDB &db,
		StdVector<ToConnect> &to_connect,
		StdVector<uint32_t> &expanded_node_ids,
		Span<const ExpressionParser::Function> functions
);

bool expand_input(
		ProgramGraph &graph,
		const ExpressionParser::Node &arg,
		ProgramGraph::Node &pg_node,
		uint32_t pg_node_input_index,
		const NodeTypeDB &db,
		StdVector<ToConnect> &to_connect,
		StdVector<uint32_t> &expanded_node_ids,
		Span<const ExpressionParser::Function> functions
) {
	switch (arg.type) {
		case ExpressionParser::Node::NUMBER: {
			const ExpressionParser::NumberNode &arg_nn = reinterpret_cast<const ExpressionParser::NumberNode &>(arg);
			VOXEL_ASSERT(pg_node_input_index < pg_node.default_inputs.size());
			pg_node.default_inputs[pg_node_input_index] = arg_nn.value;
		} break;

		case ExpressionParser::Node::VARIABLE: {
			const ExpressionParser::VariableNode &arg_vn =
					reinterpret_cast<const ExpressionParser::VariableNode &>(arg);
			to_connect.push_back({ arg_vn.name, { pg_node.id, pg_node_input_index } });
		} break;

		case ExpressionParser::Node::OPERATOR:
		case ExpressionParser::Node::FUNCTION: {
			const uint32_t dependency_pg_node_id =
					expand_node(graph, arg, db, to_connect, expanded_node_ids, functions);
			VOXEL_ASSERT_RETURN_V(dependency_pg_node_id != ProgramGraph::NULL_ID, false);
			graph.connect({ dependency_pg_node_id, 0 }, { pg_node.id, pg_node_input_index });
		} break;

		default:
			return false;
	}
	return true;
}

ProgramGraph::Node &create_node(
		ProgramGraph &graph,
		const NodeTypeDB &db,
		VoxelGraphFunction::NodeTypeID node_type_id
) {
	// 这里不创建默认子资源，因为没有使用此类节点的场景。
	ProgramGraph::Node *node = create_node_internal(graph, node_type_id, Vector2(), ProgramGraph::NULL_ID, false);
	VOXEL_ASSERT(node != nullptr);
	return *node;
}

uint32_t expand_node(
		ProgramGraph &graph,
		const ExpressionParser::Node &ep_node,
		const NodeTypeDB &db,
		StdVector<ToConnect> &to_connect,
		StdVector<uint32_t> &expanded_node_ids,
		Span<const ExpressionParser::Function> functions
) {
	switch (ep_node.type) {
		case ExpressionParser::Node::NUMBER: {
			// 注意，此代码只应在整个表达式仅为一个数字时运行。
			// 常量节点输入不会创建常量节点，它们只是设置输入的默认值。
			ProgramGraph::Node &pg_node = create_node(graph, db, VoxelGraphFunction::NODE_CONSTANT);
			const ExpressionParser::NumberNode &nn = reinterpret_cast<const ExpressionParser::NumberNode &>(ep_node);
			VOXEL_ASSERT(pg_node.params.size() == 1);
			pg_node.params[0] = nn.value;
			expanded_node_ids.push_back(pg_node.id);
			return pg_node.id;
		}

		case ExpressionParser::Node::VARIABLE: {
			// 注意，此代码只应在整个表达式仅为一个变量时运行。
			// 变量节点输入不会每次都创建节点，它们会在后续阶段转换为连接。
			// 这里我们需要一个直通节点，所以使用 `var + 0`。反正这不是常见情况。
			ProgramGraph::Node &pg_node = create_node(graph, db, VoxelGraphFunction::NODE_ADD);
			const ExpressionParser::VariableNode &vn =
					reinterpret_cast<const ExpressionParser::VariableNode &>(ep_node);
			to_connect.push_back({ vn.name, { pg_node.id, 0 } });
			VOXEL_ASSERT(pg_node.default_inputs.size() == 2);
			pg_node.default_inputs[1] = 0;
			expanded_node_ids.push_back(pg_node.id);
			return pg_node.id;
		}

		case ExpressionParser::Node::OPERATOR: {
			const ExpressionParser::OperatorNode &on =
					reinterpret_cast<const ExpressionParser::OperatorNode &>(ep_node);

			VOXEL_ASSERT(on.n0 != nullptr);
			VOXEL_ASSERT(on.n1 != nullptr);

			VoxelGraphFunction::NodeTypeID node_type_id;
			switch (on.op) {
				case ExpressionParser::OperatorNode::ADD:
					node_type_id = VoxelGraphFunction::NODE_ADD;
					break;
				case ExpressionParser::OperatorNode::SUBTRACT:
					node_type_id = VoxelGraphFunction::NODE_SUBTRACT;
					break;
				case ExpressionParser::OperatorNode::MULTIPLY:
					node_type_id = VoxelGraphFunction::NODE_MULTIPLY;
					break;
				case ExpressionParser::OperatorNode::DIVIDE:
					node_type_id = VoxelGraphFunction::NODE_DIVIDE;
					break;
				case ExpressionParser::OperatorNode::POWER:
					if (on.n1->type == ExpressionParser::Node::NUMBER) {
						// 如果幂是常量，尝试使用优化节点
						const ExpressionParser::NumberNode &arg1 =
								static_cast<const ExpressionParser::NumberNode &>(*on.n1);

						const int pi = int(arg1.value);
						if (Math::is_equal_approx(arg1.value, pi) && pi >= 0) {
							// 常量正整数
							ProgramGraph::Node &pg_node = create_node(graph, db, VoxelGraphFunction::NODE_POWI);
							expanded_node_ids.push_back(pg_node.id);

							VOXEL_ASSERT(pg_node.params.size() == 1);
							pg_node.params[0] = pi;

							VOXEL_ASSERT_RETURN_V(
									expand_input(
											graph, *on.n0, pg_node, 0, db, to_connect, expanded_node_ids, functions
									),
									ProgramGraph::NULL_ID
							);

							return pg_node.id;
						}
					}
					// 回退到通用幂函数
					node_type_id = VoxelGraphFunction::NODE_POW;
					break;
				default:
					// 修复 Clang 上未初始化变量的警告，即使它本不该在 switch 之后继续执行
					node_type_id = VoxelGraphFunction::NODE_CONSTANT;
					VOXEL_CRASH();
					break;
			}

			ProgramGraph::Node &pg_node = create_node(graph, db, node_type_id);
			expanded_node_ids.push_back(pg_node.id);

			VOXEL_ASSERT_RETURN_V(
					expand_input(graph, *on.n0, pg_node, 0, db, to_connect, expanded_node_ids, functions),
					ProgramGraph::NULL_ID
			);

			VOXEL_ASSERT_RETURN_V(
					expand_input(graph, *on.n1, pg_node, 1, db, to_connect, expanded_node_ids, functions),
					ProgramGraph::NULL_ID
			);

			return pg_node.id;
		}

		case ExpressionParser::Node::FUNCTION: {
			const ExpressionParser::FunctionNode &fn =
					reinterpret_cast<const ExpressionParser::FunctionNode &>(ep_node);
			const ExpressionParser::Function *f = ExpressionParser::find_function_by_id(fn.function_id, functions);
			VOXEL_ASSERT(f != nullptr);
			const unsigned int arg_count = f->argument_count;

			ProgramGraph::Node &pg_node = create_node(graph, db, VoxelGraphFunction::NodeTypeID(fn.function_id));
			// TODO 优化：为每个函数设置快捷方式

			for (unsigned int arg_index = 0; arg_index < arg_count; ++arg_index) {
				const ExpressionParser::Node *arg = fn.args[arg_index].get();
				VOXEL_ASSERT(arg != nullptr);
				VOXEL_ASSERT_RETURN_V(
						expand_input(graph, *arg, pg_node, arg_index, db, to_connect, expanded_node_ids, functions),
						ProgramGraph::NULL_ID
				);
			}

			return pg_node.id;
		}

		default:
			return ProgramGraph::NULL_ID;
	}
}

CompilationResult expand_expression_node(
		ProgramGraph &graph,
		uint32_t original_node_id,
		ProgramGraph::PortLocation &expanded_output_port,
		StdVector<uint32_t> &expanded_nodes,
		const NodeTypeDB &type_db
) {
	VOXEL_PROFILE_SCOPE();
	const ProgramGraph::Node &original_node = graph.get_node(original_node_id);
	VOXEL_ASSERT(original_node.params.size() != 0);
	const String code = original_node.params[0];
	const CharString code_utf8 = code.utf8();

	Span<const ExpressionParser::Function> functions = type_db.get_expression_parser_functions();

	// 提取 AST，以便将其转换为图节点，
	// 并利用范围分析和缓冲区处理的所有特性
	ExpressionParser::Result parse_result = ExpressionParser::parse(code_utf8.get_data(), functions);

	if (parse_result.error.id != ExpressionParser::ERROR_NONE) {
		// 表达式出错
		const StdString error_message_utf8 = ExpressionParser::to_string(parse_result.error);
		CompilationResult result;
		result.success = false;
		result.node_id = original_node_id;
		result.message = String(error_message_utf8.c_str());
		return result;
	}

	if (parse_result.root == nullptr) {
		// 表达式为空
		CompilationResult result;
		result.success = false;
		result.node_id = original_node_id;
		result.message = "Expression is empty";
		return result;
	}

	StdVector<ToConnect> to_connect;

	// 从表达式的 AST 创建节点并将它们连接起来
	const uint32_t expanded_root_node_id =
			expand_node(graph, *parse_result.root, type_db, to_connect, expanded_nodes, functions);
	if (expanded_root_node_id == ProgramGraph::NULL_ID) {
		CompilationResult result;
		result.success = false;
		result.node_id = original_node_id;
		result.message = "Internal error";
		return result;
	}

	expanded_output_port = { expanded_root_node_id, 0 };

	// 添加从表达式外部到表达式入口节点的连接
	for (const ToConnect tc : to_connect) {
		unsigned int original_port_index;
		if (!original_node.find_input_port_by_name(tc.var_name, original_port_index)) {
			CompilationResult result;
			result.success = false;
			result.node_id = original_node_id;
			result.message = "Could not resolve expression variable from input ports";
			return result;
		}
		VOXEL_ASSERT(original_port_index < original_node.inputs.size());
		const ProgramGraph::Port &original_port = original_node.inputs[original_port_index];
		for (const ProgramGraph::PortLocation src : original_port.connections) {
			graph.connect(src, tc.dst);
		}
	}

	// 先复制，因为之后要移除原始节点
	VOXEL_ASSERT(original_node.outputs.size() != 0);
	const ProgramGraph::Port original_output_port_copy = original_node.outputs[0];

	// 移除原始表达式节点
	graph.remove_node(original_node_id);

	// 添加来自表达式最终节点的连接。
	// 必须在最后完成，因为同一输入（旧的和新的）不允许有两条连接。
	for (const ProgramGraph::PortLocation dst : original_output_port_copy.connections) {
		graph.connect(expanded_output_port, dst);
	}

	CompilationResult result;
	result.success = true;
	return result;
}

CompilationResult expand_expression_nodes(
		ProgramGraph &graph,
		const NodeTypeDB &type_db,
		GraphRemappingInfo *remap_info
) {
	VOXEL_PROFILE_SCOPE();
	const unsigned int initial_node_count = graph.get_nodes_count();

	// 首先收集表达式节点的 ID，因为展开可能使迭代器失效
	StdVector<uint32_t> expression_node_ids;
	graph.for_each_node([&expression_node_ids](ProgramGraph::Node &node) {
		if (node.type_id == VoxelGraphFunction::NODE_EXPRESSION) {
			expression_node_ids.push_back(node.id);
		}
	});

	StdVector<uint32_t> expanded_node_ids;

	for (const uint32_t node_id : expression_node_ids) {
		ProgramGraph::PortLocation expanded_output_port;
		expanded_node_ids.clear();
		const CompilationResult result =
				expand_expression_node(graph, node_id, expanded_output_port, expanded_node_ids, type_db);
		if (!result.success) {
			return result;
		}
		if (remap_info != nullptr) {
			add_remap(*remap_info, node_id, to_span(expanded_node_ids), expanded_output_port);
		}
	}

	// 展开表达式节点可能产生更多节点，而不会移除任何节点
	VOXEL_ASSERT_RETURN_V(graph.get_nodes_count() >= initial_node_count, CompilationResult::make_error("Internal error"));

	CompilationResult result;
	result.success = true;
	return result;
}

struct NodePair {
	uint32_t node1_id; // 在 node1 的分支上
	uint32_t node2_id; // 在 node2 的分支上

	inline bool operator==(const NodePair &other) const {
		return node1_id == other.node1_id && node2_id == other.node2_id;
	}

	inline bool operator!=(const NodePair &other) const {
		return node1_id != other.node1_id || node2_id != other.node2_id;
	}
};

bool is_node_equivalent(
		const ProgramGraph &graph,
		const ProgramGraph::Node &node1,
		const ProgramGraph::Node &node2,
		StdVector<NodePair> &equivalences
) {
	if (node1.id == node2.id) {
		// 它们是同一个节点（处理祖先节点时可能发生）。
		return true;
	}
	if (contains(equivalences, NodePair{ node1.id, node2.id })) {
		// 我们已找到该等价关系。
		// 当节点有多个输入连接到相同的等价祖先时，可能发生这种情况。
		// 在评估输入时，我们可能会对同一祖先进行多次递归。
		return true;
	}
	if (node1.type_id != node2.type_id) {
		// 类型不同
		return false;
	}
	if (node1.type_id == VoxelGraphFunction::NODE_CUSTOM_INPUT) {
		if (node1.name != node2.name) {
			// 自定义输入不同
			return false;
		}
	}
	// 注意，某些节点可以有动态输入，因此我们不检查节点类型规格，而是检查节点实例
	if (node1.inputs.size() != node2.inputs.size()) {
		// 输入数量不同
		return false;
	}
	VOXEL_ASSERT_RETURN_V(node1.params.size() == node2.params.size(), false);
	for (unsigned int param_index = 0; param_index < node1.params.size(); ++param_index) {
		const Variant v1 = node1.params[param_index];
		const Variant v2 = node2.params[param_index];
		// 暂时不检查对象。即两个等价的 Noise 实例不会被判定为等价。
		if (v1 != v2) {
			// 参数不同
			return false;
		}
	}
	for (unsigned int input_index = 0; input_index < node1.inputs.size(); ++input_index) {
		const ProgramGraph::Port &node1_input = node1.inputs[input_index];
		const ProgramGraph::Port &node2_input = node2.inputs[input_index];
		if (node1_input.connections.size() != node2_input.connections.size()) {
			return false;
		}
		VOXEL_ASSERT_RETURN_V_MSG(
				node1_input.connections.size() <= 1, false, "Multiple input connections isn't supported"
		);
		// TODO 某些节点如 `*` 和 `+` 的输入是无序的，我们需要处理这种情况
		if (node1_input.connections.size() == 0) {
			// 继续这里的过度谨慎，但这是因为 Godot 没有定义 `_DEBUG`（而且我无法在不导致链接失败的情况下
			// 在我的模块中定义它），所以标准库的边界检查形同虚设
			VOXEL_ASSERT(node1.default_inputs.size() == node1.inputs.size());
			VOXEL_ASSERT(node2.default_inputs.size() == node2.inputs.size());
			// 没有祖先节点，检查默认输入（忽略自动连接，它必须在之前已应用）
			const Variant v1 = node1.default_inputs[input_index];
			const Variant v2 = node2.default_inputs[input_index];
			if (v1 != v2) {
				// 默认输入不同
				return false;
			}
		} else {
			const ProgramGraph::PortLocation &node1_src = node1_input.connections[0];
			const ProgramGraph::PortLocation &node2_src = node2_input.connections[0];
			if (node1_src.port_index != node2_src.port_index) {
				// 祖先节点输出不同
				return false;
			}
			const ProgramGraph::Node &ancestor1 = graph.get_node(node1_src.node_id);
			const ProgramGraph::Node &ancestor2 = graph.get_node(node2_src.node_id);
			if (!is_node_equivalent(graph, ancestor1, ancestor2, equivalences)) {
				// 祖先节点不同
				equivalences.clear();
				return false;
			}
		}
	}
	NodePair equivalence{ node1.id, node2.id };
#ifdef DEBUG_ENABLED
	for (const NodePair &p : equivalences) {
		// 我们已经检查过这一点，如果这里仍然出现重复，说明有问题
		VOXEL_ASSERT_RETURN_V(p != equivalence, true);
	}
#endif
	equivalences.push_back(equivalence);
	return true;
}

// 移除节点 2 并将其输出连接转移到节点 1。两个节点必须具有相同的类型。
void merge_node(ProgramGraph &graph, uint32_t node1_id, uint32_t node2_id, GraphRemappingInfo *remap_info) {
	const ProgramGraph::Node &node1 = graph.get_node(node1_id);
	const ProgramGraph::Node &node2 = graph.get_node(node2_id);
	VOXEL_ASSERT_RETURN(node1.type_id == node2.type_id);
	VOXEL_ASSERT_RETURN(node1.outputs.size() == node2.outputs.size());
	// 移除 2，保留 1
	for (unsigned int output_index = 0; output_index < node2.outputs.size(); ++output_index) {
		// 移除输出连接，在等价节点上重新创建它们。
		// 复制输出连接，因为在迭代过程中它们会被修改。
		StdVector<ProgramGraph::PortLocation> dsts = node2.outputs[output_index].connections;
		for (ProgramGraph::PortLocation dst : dsts) {
			graph.disconnect(ProgramGraph::PortLocation{ node2_id, output_index }, dst);
			graph.connect(ProgramGraph::PortLocation{ node1_id, output_index }, dst);
		}
	}
	if (remap_info != nullptr) {
		add_remap(*remap_info, node2_id, node1_id, node1.outputs.size());
	}
	graph.remove_node(node2_id);
}

// 找到具有等价参数和等价祖先的节点，以便它们可以合并为单个分支。
// 这最好在展开表达式和宏之后进行。
// 自动化此过程可以创建使用相似操作的宏，而不会因重复而损失性能。
// 例如，SphereHeightNoise 宏会想要归一化 (X,Y,Z)。其他分支可能也想这样做，
// 因此我们应该共享该操作，但对于自包含的分支来说这比较困难。所以更简单的方式是
// 将此委托给自动化过程。
void merge_equivalences(ProgramGraph &graph, GraphRemappingInfo *remap_info) {
	VOXEL_PROFILE_SCOPE();
	StdVector<uint32_t> node_ids;
	graph.get_node_ids(node_ids);

	// 在此声明以避免过多地重新分配内存
	StdVector<NodePair> equivalences;

	// 对每对唯一节点
	for (unsigned int i = 0; i < node_ids.size(); ++i) {
		// 列表中的节点可能在此过程中被移除，因此我们测试它们是否仍然存在
		const uint32_t node1_id = node_ids[i];
		const ProgramGraph::Node *node1 = graph.try_get_node(node1_id);
		if (node1 == nullptr) {
			continue;
		}
		for (unsigned int j = i + 1; j < node_ids.size(); ++j) {
			const uint32_t node2_id = node_ids[j];
			const ProgramGraph::Node *node2 = graph.try_get_node(node2_id);
			if (node2 == nullptr) {
				// 可能已被合并
				continue;
			}
			equivalences.clear();
			// 该对是否等价？
			if (is_node_equivalent(graph, *node1, *node2, equivalences)) {
				// 合并节点以共享它们的输出
				for (NodePair equivalence : equivalences) {
					merge_node(graph, equivalence.node1_id, equivalence.node2_id, remap_info);
				}
				if (graph.try_get_node(node1_id) == nullptr) {
					// 节点 1 已被合并，停止寻找与它的等价关系
					break;
				}
			}
		}
	}
}

// 对于每个启用了自动连接的节点，如果它们有支持自动连接但未连接的端口，则将其连接到
// 默认输入节点。如果图中不存在此类节点，但它存在于图的输入定义中，则
// 创建该节点。
void apply_auto_connects(
		ProgramGraph &graph,
		Span<const VoxelGraphFunction::Port> input_defs,
		const NodeTypeDB &type_db
) {
	// 先复制 ID，因为可能要创建新节点
	StdVector<uint32_t> node_ids;
	graph.get_node_ids(node_ids);

	for (const uint32_t node_id : node_ids) {
		const ProgramGraph::Node &node = graph.get_node(node_id);

		if (node.autoconnect_default_inputs == false) {
			// 将改用显式常量
			continue;
		}

		for (unsigned int input_index = 0; input_index < node.inputs.size(); ++input_index) {
			const ProgramGraph::Port &input_port = node.inputs[input_index];
			if (input_port.connections.size() > 0) {
				// 已连接
				continue;
			}

			// const NodeType &type = type_db.get_type(node.type_id);
			// VOXEL_ASSERT(node.inputs.size() == type.inputs.size());
			// const VoxelGraphFunction::AutoConnect auto_connect = type.inputs[input_index].auto_connect;
			const VoxelGraphFunction::AutoConnect auto_connect =
					VoxelGraphFunction::AutoConnect(input_port.autoconnect_hint);
			VoxelGraphFunction::NodeTypeID src_type;
			if (!VoxelGraphFunction::try_get_node_type_id_from_auto_connect(auto_connect, src_type)) {
				// 没有提示或提示无效
				continue;
			}

			bool found_in_input_defs = false;
			for (const VoxelGraphFunction::Port &input_def : input_defs) {
				if (input_def.type == src_type) {
					found_in_input_defs = true;
					break;
				}
			}
			if (!found_in_input_defs) {
				// 不是声明的输入
				VOXEL_PRINT_VERBOSE(
						"Not applying auto-connect because the corresponding node type isn't present in input "
						"definitions of the function."
				);
				continue;
			}

			// 图输入节点的实例都是等价的，因此我们可以任选一个
			uint32_t src_node_id = graph.find_node_by_type(src_type);
			if (src_node_id == ProgramGraph::NULL_ID) {
				// 未找到，则创建它
				const ProgramGraph::Node *src_node =
						create_node_internal(graph, src_type, Vector2(), graph.generate_node_id(), false);
				VOXEL_ASSERT_CONTINUE(src_node != nullptr);
				src_node_id = src_node->id;
			}
			graph.connect(
					ProgramGraph::PortLocation{ src_node_id, 0 }, ProgramGraph::PortLocation{ node_id, input_index }
			);
		}
	}
}

void try_simplify_clamp_node(
		ProgramGraph &graph,
		const ProgramGraph::Node &node,
		const NodeTypeDB &type_db,
		GraphRemappingInfo *remap_info
) {
	VOXEL_ASSERT(node.inputs.size() == 3);

	const uint32_t clamp_x_input_id = 0;
	const uint32_t clamp_output_id = 0;
	const uint32_t clamp_min_input_id = 1;
	const uint32_t clamp_max_input_id = 2;

	const uint32_t clampc_x_input_id = 0;
	const uint32_t clampc_output_id = 0;
	const uint32_t clampc_min_param_id = 0;
	const uint32_t clampc_max_param_id = 1;

	if (node.inputs[clamp_min_input_id].connections.size() == 0 &&
		node.inputs[clamp_max_input_id].connections.size() == 0) {
		// 可以替换为具有常量边界的 clamp 版本

		VOXEL_ASSERT(node.default_inputs.size() == node.inputs.size());

		const float minv = node.default_inputs[clamp_min_input_id];
		const float maxv = node.default_inputs[clamp_max_input_id];

		// 创建新节点
		ProgramGraph::Node &clampc_node = create_node(graph, type_db, VoxelGraphFunction::NODE_CLAMP_C);

		// 为新节点分配参数
		clampc_node.params[clampc_min_param_id] = minv;
		clampc_node.params[clampc_max_param_id] = maxv;

		// 连接新节点的输入
		const ProgramGraph::Port &clamp_input = node.inputs[clamp_x_input_id];
		for (const ProgramGraph::PortLocation &src : clamp_input.connections) {
			graph.connect(src, ProgramGraph::PortLocation{ clampc_node.id, clampc_x_input_id });
		}

		// 制作副本，因为我们需要先断开这些连接
		const StdVector<ProgramGraph::PortLocation> clamp_output_connections =
				node.outputs[clamp_output_id].connections;
		for (const ProgramGraph::PortLocation &dst : clamp_output_connections) {
			graph.disconnect(ProgramGraph::PortLocation{ node.id, clamp_output_id }, dst);
		}

		// 连接新节点的输出
		for (const ProgramGraph::PortLocation &dst : clamp_output_connections) {
			graph.connect(ProgramGraph::PortLocation{ clampc_node.id, clampc_output_id }, dst);
		}

		// 更新重映射以用于调试追踪
		if (remap_info != nullptr) {
			add_remap(*remap_info, node.id, clampc_node.id, node.outputs.size());
		}

		// 移除旧节点
		graph.remove_node(node.id);
		// 从此时起，`node` 已失效。
	}
}

void replace_simplifiable_nodes(ProgramGraph &graph, const NodeTypeDB &type_db, GraphRemappingInfo *remap_info) {
	StdVector<uint32_t> node_ids;
	// TODO 优化：只收集我们感兴趣的节点 ID？
	graph.get_node_ids(node_ids);

	for (const uint32_t &node_id : node_ids) {
		const ProgramGraph::Node &node = graph.get_node(node_id);

		if (node.type_id == VoxelGraphFunction::NODE_CLAMP) {
			try_simplify_clamp_node(graph, node, type_db, remap_info);
		}
	}
}

// 如果传入的节点与某个端口对应，则将其添加到与该端口对应的节点列表中。
bool try_add_io_node(
		Span<const VoxelGraphFunction::Port> ports,
		const ProgramGraph::Node &node,
		Span<StdVector<uint32_t>> node_ids_per_port
) {
	VOXEL_ASSERT(ports.size() == node_ids_per_port.size());

	for (unsigned int port_index = 0; port_index < ports.size(); ++port_index) {
		const VoxelGraphFunction::Port &port = ports[port_index];

		if (is_node_matching_port(node, port)) {
			node_ids_per_port[port_index].push_back(node.id);
			return true;
		}
	}

	return false;
}

void get_input_and_output_node_ids(
		const ProgramGraph &graph,
		Span<const VoxelGraphFunction::Port> input_defs,
		Span<const VoxelGraphFunction::Port> output_defs,
		StdVector<StdVector<uint32_t>> &input_node_ids,
		StdVector<StdVector<uint32_t>> &output_node_ids
) {
	input_node_ids.resize(input_defs.size());
	for (StdVector<uint32_t> &node_ids : input_node_ids) {
		node_ids.clear();
	}
	output_node_ids.resize(output_defs.size());
	for (StdVector<uint32_t> &node_ids : output_node_ids) {
		node_ids.clear();
	}
	graph.for_each_node_const(
			[&input_node_ids, &output_node_ids, &input_defs, &output_defs](const ProgramGraph::Node &node) {
				if (try_add_io_node(input_defs, node, to_span(input_node_ids))) {
					return;
				}
				if (try_add_io_node(output_defs, node, to_span(output_node_ids))) {
					return;
				}
			}
	);
}

void get_input_node_ids(
		const ProgramGraph &graph,
		Span<const VoxelGraphFunction::Port> input_defs,
		StdVector<StdVector<uint32_t>> &input_node_ids
) {
	input_node_ids.resize(input_defs.size());
	for (StdVector<uint32_t> &node_ids : input_node_ids) {
		node_ids.clear();
	}
	graph.for_each_node_const([&input_node_ids, &input_defs](const ProgramGraph::Node &node) {
		try_add_io_node(input_defs, node, to_span(input_node_ids));
	});
}

// 用一个函数的内部内容原地替换该函数节点，并建立与周围环境的等价连接。
CompilationResult expand_function(
		ProgramGraph &graph,
		uint32_t node_id,
		const NodeTypeDB &type_db,
		GraphRemappingInfo *remap_info
) {
	VOXEL_PROFILE_SCOPE();
	const ProgramGraph::Node &fnode = graph.get_node(node_id);
	VOXEL_ASSERT(fnode.type_id == VoxelGraphFunction::NODE_FUNCTION);
	VOXEL_ASSERT(fnode.params.size() >= 1);
	Ref<VoxelGraphFunction> function = fnode.params[0];

	if (function.is_null()) {
		return CompilationResult::make_error("Function resource is invalid.", node_id);
	}

	// 检查输入输出是否是最新的
	Span<const VoxelGraphFunction::Port> func_inputs = function->get_input_definitions();
	Span<const VoxelGraphFunction::Port> func_outputs = function->get_output_definitions();
	if (func_inputs.size() != fnode.inputs.size()) {
		return CompilationResult::make_error("Function inputs are not up to date.", node_id);
	}
	if (func_outputs.size() != fnode.outputs.size()) {
		return CompilationResult::make_error("Function outputs are not up to date.", node_id);
	}

	// 复制原始图，以便我们可以对函数进行一些本地预处理
	ProgramGraph fgraph;
	{
		const ProgramGraph &fgraph_original = function->get_graph();
		fgraph.copy_from(fgraph_original, false);
		apply_auto_connects(fgraph, func_inputs, type_db);
	}

	StdUnorderedMap<uint32_t, uint32_t> fn_to_expanded_node_ids;
	StdVector<uint32_t> nested_func_node_ids;

	// 复制节点。I/O 节点暂时替换为中继节点，它们将在后续阶段被简化掉。
	fgraph.for_each_node_const(
			[&graph, &type_db, &fn_to_expanded_node_ids, &nested_func_node_ids](const ProgramGraph::Node &src_node) {
				const NodeType &node_type = type_db.get_type(src_node.type_id);

				// 所有节点都会有一个解包后的等价节点
				const ProgramGraph::Node *expanded_node;
				if (node_type.category == pg::CATEGORY_INPUT || node_type.category == pg::CATEGORY_OUTPUT) {
					expanded_node = create_node_internal(
							graph, VoxelGraphFunction::NODE_RELAY, Vector2(), graph.generate_node_id(), false
					);
				} else {
					expanded_node = duplicate_node(graph, src_node, false);
				}

				fn_to_expanded_node_ids[src_node.id] = expanded_node->id;

				if (expanded_node->type_id == VoxelGraphFunction::NODE_FUNCTION) {
					nested_func_node_ids.push_back(expanded_node->id);
				}
			}
	);

	// 复制内部连接
	for (auto it = fn_to_expanded_node_ids.begin(); it != fn_to_expanded_node_ids.end(); ++it) {
		const uint32_t fn_node_id = it->first;
		const uint32_t ex_node_id = it->second;

		const ProgramGraph::Node &fn_node = fgraph.get_node(fn_node_id);

		for (unsigned int i = 0; i < fn_node.outputs.size(); ++i) {
			const ProgramGraph::Port &fn_port = fn_node.outputs[i];

			for (const ProgramGraph::PortLocation fn_dst : fn_port.connections) {
				auto it2 = fn_to_expanded_node_ids.find(fn_dst.node_id);
				if (it2 == fn_to_expanded_node_ids.end()) {
					continue;
				}
				graph.connect(
						ProgramGraph::PortLocation{ ex_node_id, i },
						ProgramGraph::PortLocation{ it2->second, fn_dst.port_index }
				);
			}
		}
	}

	// 获取与每个输入和输出对应的节点
	StdVector<StdVector<uint32_t>> inputs_node_ids;
	StdVector<StdVector<uint32_t>> outputs_node_ids;
	get_input_and_output_node_ids(fgraph, func_inputs, func_outputs, inputs_node_ids, outputs_node_ids);

	// 断开函数节点的输出连接，因为我们要替换它们。
	// （目标端口在同一时间不允许有两条连接）
	StdVector<ProgramGraph::Port> fnode_outputs = fnode.outputs;
	for (unsigned int output_index = 0; output_index < fnode_outputs.size(); ++output_index) {
		const ProgramGraph::Port &port = fnode_outputs[output_index];
		ProgramGraph::PortLocation src{ fnode.id, output_index };
		for (const ProgramGraph::PortLocation &dst : port.connections) {
			graph.disconnect(src, dst);
		}
	}

	// 将指示在检查面向用户的图中的输出时应查找哪些端口
	StdVector<ProgramGraph::PortLocation> output_locations;
	if (remap_info != nullptr) {
		output_locations.resize(fnode_outputs.size(), ProgramGraph::PortLocation{ ProgramGraph::NULL_ID, 0 });
	}

	// 查找函数的输入与解包后节点之间的映射
	StdVector<StdVector<ProgramGraph::PortLocation>> inputs_to_destinations;
	inputs_to_destinations.resize(func_inputs.size());

	for (unsigned int input_index = 0; input_index < fnode.inputs.size(); ++input_index) {
		StdVector<ProgramGraph::PortLocation> &in_destinations = inputs_to_destinations[input_index];

		VOXEL_ASSERT(input_index < inputs_node_ids.size());
		const StdVector<uint32_t> &inner_input_node_ids = inputs_node_ids[input_index];

		// 对于与此输入对应的每个内部节点
		for (const uint32_t inner_input_node_id : inner_input_node_ids) {
			auto it = fn_to_expanded_node_ids.find(inner_input_node_id);
			// 我们为函数中的每个节点都创建了一个节点，因此必须存在匹配
			VOXEL_ASSERT(it != fn_to_expanded_node_ids.end());
			in_destinations.push_back(ProgramGraph::PortLocation{ it->second, 0 });
		}
	}

	// 创建来自连接到函数输入的节点的连接。
	for (unsigned int input_index = 0; input_index < inputs_to_destinations.size(); ++input_index) {
		const StdVector<ProgramGraph::PortLocation> &destinations = inputs_to_destinations[input_index];
		const ProgramGraph::Port &port = fnode.inputs[input_index];
		if (port.connections.size() == 0) {
			// 分配默认输入值
			if (port.autoconnect_hint == VoxelGraphFunction::AUTO_CONNECT_NONE || !fnode.autoconnect_default_inputs) {
				VOXEL_ASSERT(input_index < fnode.default_inputs.size());
				const float defval = fnode.default_inputs[input_index];
				for (const ProgramGraph::PortLocation &dst : destinations) {
					ProgramGraph::Node &dst_node = graph.get_node(dst.node_id);
					VOXEL_ASSERT(dst.port_index < dst_node.default_inputs.size());
					dst_node.default_inputs[dst.port_index] = defval;
				}
			}
		} else {
			// 创建连接
			VOXEL_ASSERT_MSG(port.connections.size() == 1, "Input nodes are expected to have only 1 input");
			const ProgramGraph::PortLocation src = port.connections[0];
			for (const ProgramGraph::PortLocation &dst : destinations) {
				graph.connect(src, dst);
			}
		}
	}

	// 创建来自函数输出的连接
	for (unsigned int output_index = 0; output_index < fnode_outputs.size(); ++output_index) {
		const ProgramGraph::Port &port = fnode_outputs[output_index];
		if (port.connections.size() == 0) {
			// 该输出在函数外部没有连接
			continue;
		}
		VOXEL_ASSERT(output_index < outputs_node_ids.size());
		const StdVector<uint32_t> &output_node_ids = outputs_node_ids[output_index];
		if (output_node_ids.size() == 0) {
			// 该输出实际上没有绑定到任何节点。
			VOXEL_PRINT_VERBOSE("Function output isn't bound to an output node");
			continue;
		}
		// 输出节点只能出现一次
		VOXEL_ASSERT(output_node_ids.size() == 1);
		const ProgramGraph::Node &inner_fnode = fgraph.get_node(output_node_ids[0]);
		VOXEL_ASSERT(inner_fnode.inputs.size() == 1);
		const ProgramGraph::Port &foi = inner_fnode.inputs[0];

		if (foi.connections.size() == 0) {
			// 该输出在函数内部没有连接
			if (foi.autoconnect_hint == VoxelGraphFunction::AUTO_CONNECT_NONE ||
				!inner_fnode.autoconnect_default_inputs) {
				// 分配默认值
				for (const ProgramGraph::PortLocation dst : port.connections) {
					ProgramGraph::Node &dst_node = graph.get_node(dst.node_id);
					// TODO 不确定如何处理对其值进行处理的输出！
					dst_node.default_inputs[dst.port_index] = inner_fnode.default_inputs[0];
				}
			}

		} else {
			// 该输出在函数内部已连接
			VOXEL_ASSERT(foi.connections.size() == 1);
			const ProgramGraph::PortLocation fsrc = foi.connections[0];
			auto it = fn_to_expanded_node_ids.find(fsrc.node_id);
			// 我们为函数中的每个节点都创建了一个节点，因此必须存在匹配
			VOXEL_ASSERT(it != fn_to_expanded_node_ids.end());
			for (const ProgramGraph::PortLocation dst : port.connections) {
				graph.connect(ProgramGraph::PortLocation{ it->second, fsrc.port_index }, dst);
			}

			if (remap_info != nullptr) {
				VOXEL_ASSERT(output_index < output_locations.size());
				output_locations[output_index] = ProgramGraph::PortLocation{ it->second, fsrc.port_index };
			}
		}
	}

	if (remap_info != nullptr) {
		StdVector<uint32_t> expanded_node_ids;
		for (auto it = fn_to_expanded_node_ids.begin(); it != fn_to_expanded_node_ids.end(); ++it) {
			const uint32_t ex_node_id = it->second;
			const ProgramGraph::Node &node = graph.get_node(ex_node_id);
			// 不要为函数节点添加重映射，因为它们反正会被展开
			if (node.type_id == VoxelGraphFunction::NODE_FUNCTION) {
				continue;
			}
			expanded_node_ids.push_back(ex_node_id);
		}

		add_remap(*remap_info, node_id, to_span(expanded_node_ids), to_span(output_locations));
		// TODO 如果函数是直通的，它将不会出现在 `ExecutionMap::debug_nodes` 中。
		// 在 `A --- Func --- B` 这样的图中，Func 会消失，只留下 `A --- B`。因此在最终图中
		// 没有与面向用户图中的函数对应的节点。
		// 从技术上讲，我们可以认为 A 与 Func 等价，但 A 已经出现在调试执行
		// 映射中。我们需要让 A 出现在 `debug_nodes` 列表中，并在
		// `expanded_node_id_to_user_node_id` 中有一个 (A => Func) 对，但如果 A 已经在面向用户的图中，
		// 就保留它在列表中，而不是用重映射替换它。但这样做意味着 `debug_nodes` 的索引不再与执行映射的
		// 索引匹配，这对性能分析来说有点问题。
		// 我目前没有修复这个问题，感觉不值得，这是一个退化情况的边缘案例。
	}

	// 移除函数节点
	graph.remove_node(fnode.id);
	// 从此时起，`fnode` 已失效。

	// 展开嵌套函数
	for (const uint32_t nested_node_id : nested_func_node_ids) {
		expand_function(graph, nested_node_id, type_db, remap_info);
	}

	CompilationResult result;
	result.success = true;
	return result;
}

CompilationResult expand_functions(ProgramGraph &graph, const NodeTypeDB &type_db, GraphRemappingInfo *remap_info) {
	StdVector<uint32_t> func_node_ids;

	graph.for_each_node_const([&func_node_ids](const ProgramGraph::Node &node) {
		if (node.type_id == VoxelGraphFunction::NODE_FUNCTION) {
			func_node_ids.push_back(node.id);
		}
	});

	for (const uint32_t node_id : func_node_ids) {
		const CompilationResult result = expand_function(graph, node_id, type_db, remap_info);
		if (!result.success) {
			return result;
		}
	}

	CompilationResult result;
	result.success = true;
	return result;
}

void remove_relay(ProgramGraph &graph, const uint32_t node_id, GraphRemappingInfo *remap_info) {
	const ProgramGraph::Node &node = graph.get_node(node_id);
	VOXEL_ASSERT(node.inputs.size() == 1);
	VOXEL_ASSERT(node.outputs.size() == 1);

	const ProgramGraph::Port &node_input = node.inputs[0];
	if (node_input.connections.size() == 0) {
		// 直接移除节点，
		// 但首先需要传播默认输入。这用于函数展开。
		if (node.autoconnect_default_inputs == false) {
			VOXEL_ASSERT(node.default_inputs.size() > 0);
			const float defval = node.default_inputs[0];
			for (const ProgramGraph::Port &out : node.outputs) {
				for (const ProgramGraph::PortLocation dst : out.connections) {
					ProgramGraph::Node &dst_node = graph.get_node(dst.node_id);
					dst_node.default_inputs[dst.port_index] = defval;
				}
			}
		}
		graph.remove_node(node_id);
		return;
	}

	VOXEL_ASSERT(node_input.connections.size() == 1);
	const ProgramGraph::PortLocation src = node_input.connections[0];

	const StdVector<ProgramGraph::Port> node_outputs = node.outputs;

	for (uint32_t output_index = 0; output_index < node_outputs.size(); ++output_index) {
		const ProgramGraph::Port &port = node_outputs[output_index];
		for (const ProgramGraph::PortLocation dst : port.connections) {
			const ProgramGraph::PortLocation old_src{ node_id, output_index };
			graph.disconnect(old_src, dst);
			graph.connect(src, dst);
		}
	}

	if (remap_info != nullptr) {
		for (unsigned int i = 0; i < remap_info->expanded_to_user_node_ids.size(); ++i) {
			const ExpandedNodeRemap &r = remap_info->expanded_to_user_node_ids[i];
			if (r.expanded_node_id == node_id) {
				remap_info->expanded_to_user_node_ids[i] = remap_info->expanded_to_user_node_ids.back();
				remap_info->expanded_to_user_node_ids.pop_back();
				break;
			}
		}
		for (unsigned int i = 0; i < remap_info->user_to_expanded_ports.size(); ++i) {
			PortRemap &pr = remap_info->user_to_expanded_ports[i];
			if (pr.expanded.node_id == node_id) {
				pr.expanded = src;
			}
		}
	}

	graph.remove_node(node_id);
}

void remove_relays(ProgramGraph &graph, GraphRemappingInfo *remap_info) {
	StdVector<uint32_t> node_ids;
	graph.for_each_node([&node_ids](const ProgramGraph::Node &node) {
		if (node.type_id == VoxelGraphFunction::NODE_RELAY) {
			node_ids.push_back(node.id);
		}
	});
	for (const uint32_t node_id : node_ids) {
		remove_relay(graph, node_id, remap_info);
	}
}

void combine_inputs(
		ProgramGraph &graph,
		uint32_t node_id,
		uint32_t node_id_to_combine,
		GraphRemappingInfo *remap_info
) {
	const ProgramGraph::Node &node = graph.get_node(node_id_to_combine);
	VOXEL_ASSERT(node.inputs.size() == 0);
	VOXEL_ASSERT(node.outputs.size() == 1);
	merge_node(graph, node_id, node_id_to_combine, remap_info);
}

// 输入节点为了便利可以出现多次，但编译时应只出现一次。
CompilationResult combine_inputs(
		ProgramGraph &graph,
		Span<const VoxelGraphFunction::Port> input_defs,
		const NodeTypeDB &type_db,
		GraphRemappingInfo *remap_info,
		StdVector<uint32_t> *out_input_node_ids
) {
	StdVector<StdVector<uint32_t>> node_ids_per_port;
	get_input_node_ids(graph, input_defs, node_ids_per_port);

	for (unsigned int input_index = 0; input_index < input_defs.size(); ++input_index) {
		const StdVector<uint32_t> &node_ids = node_ids_per_port[input_index];
		if (node_ids.size() == 0) {
			const VoxelGraphFunction::Port &input_def = input_defs[input_index];
			const NodeType &type = type_db.get_type(input_def.type);
			CompilationResult result;
			result.success = false;
			result.message = String("The graph requires at least one input node matching '{0}' ({1}). Add the missing "
									"node or remove "
									"the input in I/O settings.")
									 .format(varray(input_def.name, type.name));
			return result;
		}
		const uint32_t node_id = node_ids[0];
		for (unsigned int i = 1; i < node_ids.size(); ++i) {
			combine_inputs(graph, node_id, node_ids[i], remap_info);
		}
	}

	if (out_input_node_ids != nullptr) {
		out_input_node_ids->resize(input_defs.size());
		for (unsigned int input_index = 0; input_index < input_defs.size(); ++input_index) {
			const StdVector<uint32_t> &node_ids = node_ids_per_port[input_index];
			VOXEL_ASSERT(node_ids.size() > 0);
			(*out_input_node_ids)[input_index] = node_ids[0];
		}
	}

	return CompilationResult::make_success();
}

CompilationResult compile_params(
		const NodeType &node_type,
		const uint32_t node_id,
		StdVector<uint16_t> &program,
		StdVector<Runtime::HeapResource> &heap_resources,
		Span<const Variant> params_source
) {
	// 为参数大小预留空间，默认为无参数，因此大小为 0
	const uint32_t params_size_index = program.size();
	program.push_back(0);

	if (node_type.compile_func != nullptr) {
		pg::CompileContext ctx(program, heap_resources, params_source);
		node_type.compile_func(ctx);

		if (ctx.has_error()) {
			CompilationResult result;
			result.success = false;
			result.message = ctx.get_error_message();
			result.node_id = node_id;
			return result;
		}

		const size_t params_size = ctx.get_params_size_in_words();
		VOXEL_ASSERT(params_size <= std::numeric_limits<uint16_t>::max());
		VOXEL_ASSERT(params_size_index < program.size());
		program[params_size_index] = params_size;
	}

	return CompilationResult::make_success();
}

CompilationResult evaluate_single_node(
		const ProgramGraph::Node &node,
		const NodeType &node_type,
		StdVector<float> &output_values
) {
	output_values.clear();

	if (node.type_id == VoxelGraphFunction::NODE_CONSTANT) {
		VOXEL_ASSERT(node.params.size() >= 1);
		output_values.push_back(node.params[0]);
		return CompilationResult::make_success();
	}

	VOXEL_ASSERT_RETURN_V(
			node_type.process_buffer_func != nullptr,
			CompilationResult::make_error("Graph node has no buffer processing function. Bug?", node.id)
	);

	StdVector<float> values;
	StdVector<Runtime::Buffer> buffers;
	StdVector<uint16_t> input_indices;
	StdVector<uint16_t> output_indices;
	StdVector<uint8_t> params;
	StdVector<uint16_t> program;
	StdVector<Runtime::HeapResource> heap_resources;

	for (unsigned int i = 0; i < node.inputs.size(); ++i) {
		values.push_back(node.default_inputs[i]);
	}
	for (unsigned int i = 0; i < node.inputs.size(); ++i) {
		Runtime::Buffer buffer;
		buffer.data = &values[i];
		buffer.is_constant = true;
		buffer.constant_value = values[i];
		buffer.size = 1;

		buffers.push_back(buffer);
		input_indices.push_back(i);
	}

	values.resize(values.size() + node.outputs.size());
	for (unsigned int i = 0; i < node.outputs.size(); ++i) {
		const unsigned int value_index = node.inputs.size() + i;

		Runtime::Buffer buffer;
		buffer.data = &values[value_index];
		buffer.is_constant = false;
		buffer.size = 1;
		buffers.push_back(buffer);

		output_indices.push_back(value_index);
	}

	{
		pg::CompilationResult res = compile_params(node_type, node.id, program, heap_resources, to_span(node.params));
		if (!res.success) {
			return res;
		}
	}

	{
		Span<const uint16_t> program_s = to_span_const(program);
		unsigned int pc = 0;
		Span<const uint8_t> params_s = Runtime::read_params(program_s, pc);

		pg::Runtime::ProcessBufferContext ctx(
				to_span(input_indices), to_span(output_indices), params_s, to_span(buffers), false
		);
		node_type.process_buffer_func(ctx);
	}

	for (Runtime::HeapResource &hr : heap_resources) {
		hr.free();
	}

	output_values.clear();
	for (const uint16_t oi : output_indices) {
		output_values.push_back(values[oi]);
	}

	return CompilationResult::make_success();
}

bool has_ancestor(const ProgramGraph::Node &node) {
	for (const ProgramGraph::Port &input : node.inputs) {
		if (input.connections.size() != 0) {
			return true;
		}
	}
	return false;
}

// 移除常量节点和分支，将它们作为默认值留在所连接的输入上。
// 必须在应用自动连接之后使用。
CompilationResult reduce_constants(ProgramGraph &graph, const NodeTypeDB &type_db) {
	StdVector<uint32_t> src_node_ids;
	StdVector<float> output_values;

	// 测试是否有任何节点是常量，移除它们，然后重试，直到全部移除
	// TODO 这可能可以进行优化
	bool keep_going = true;
	while (keep_going) {
		keep_going = false;

		src_node_ids.clear();
		graph.get_node_ids(src_node_ids);

		for (const uint32_t node_id : src_node_ids) {
			const ProgramGraph::Node &node = graph.get_node(node_id);

			if (node.outputs.size() == 0) {
				continue;
			}

			const NodeType &node_type = type_db.get_type(node.type_id);
			if (node_type.category == pg::CATEGORY_OUTPUT) {
				continue;
			}
			if (node_type.category == pg::CATEGORY_INPUT) {
				continue;
			}

			if (has_ancestor(node)) {
				continue;
			}

			const CompilationResult eval_result = evaluate_single_node(node, node_type, output_values);
			if (!eval_result.success) {
				return eval_result;
			}

			VOXEL_ASSERT_CONTINUE(output_values.size() == node.outputs.size());

			for (unsigned int output_index = 0; output_index < output_values.size(); ++output_index) {
				const ProgramGraph::Port &output = node.outputs[output_index];
				const float value = output_values[output_index];

				for (const ProgramGraph::PortLocation &dst_loc : output.connections) {
					ProgramGraph::Node &dst_node = graph.get_node(dst_loc.node_id);
					dst_node.default_inputs[dst_loc.port_index] = value;
				}
			}

			graph.remove_node(node_id);

			keep_going = true;
		}
	}

	return CompilationResult::make_success();
}

} // namespace

CompilationResult expand_graph(
		const ProgramGraph &graph,
		ProgramGraph &expanded_graph,
		Span<const VoxelGraphFunction::Port> input_defs,
		StdVector<uint32_t> *input_node_ids,
		const NodeTypeDB &type_db,
		GraphRemappingInfo *remap_info,
		const bool enable_constant_reduction
) {
	VOXEL_PROFILE_SCOPE();
	// 首先制作一份我们要修改的图的副本
	expanded_graph.copy_from(graph, false);

	apply_auto_connects(expanded_graph, input_defs, type_db);

	CompilationResult func_expand_result = expand_functions(expanded_graph, type_db, remap_info);
	if (!func_expand_result.success) {
		return func_expand_result;
	}

	remove_relays(expanded_graph, remap_info);

	const CompilationResult expr_expand_result = expand_expression_nodes(expanded_graph, type_db, remap_info);
	if (!expr_expand_result.success) {
		return expr_expand_result;
	}

	if (enable_constant_reduction) {
		const CompilationResult reduction_result = reduce_constants(expanded_graph, type_db);
		if (!reduction_result.success) {
			return reduction_result;
		}
	}

	merge_equivalences(expanded_graph, remap_info);
	replace_simplifiable_nodes(expanded_graph, type_db, remap_info);
	const CompilationResult input_combining_result =
			combine_inputs(expanded_graph, input_defs, type_db, remap_info, input_node_ids);
	if (!input_combining_result.success) {
		return input_combining_result;
	}

	return expr_expand_result;
}

CompilationResult Runtime::compile(const VoxelGraphFunction &function, bool debug) {
	VOXEL_PROFILE_SCOPE();

	const NodeTypeDB &type_db = NodeTypeDB::get_singleton();

	GraphRemappingInfo remap_info;
	ProgramGraph expanded_graph;
	StdVector<uint32_t> input_node_ids;
	Span<const VoxelGraphFunction::Port> input_defs = function.get_input_definitions();
	CompilationResult expand_result = expand_graph(
			function.get_graph(), expanded_graph, input_defs, &input_node_ids, type_db, &remap_info, !debug
	);
	if (!expand_result.success) {
		expand_result.node_id = get_original_node_id(remap_info, expand_result.node_id);
		return expand_result;
	}

	CompilationResult result = compile_preprocessed_graph(
			_program, expanded_graph, input_defs.size(), to_span(input_node_ids), debug, type_db
	);
	if (!result.success) {
		clear();
	}

	for (PortRemap r : remap_info.user_to_expanded_ports) {
		if (r.expanded.node_id != ProgramGraph::NULL_ID) {
			_program.user_port_to_expanded_port.insert({ r.original, r.expanded });
		}
	}
	for (ExpandedNodeRemap r : remap_info.expanded_to_user_node_ids) {
		_program.expanded_node_id_to_user_node_id.insert({ r.expanded_node_id, r.original_node_id });
	}
	// 将执行映射中的调试节点重映射为面向用户的节点
	for (uint32_t &debug_node_id : _program.default_execution_map.debug_nodes) {
		auto it = _program.expanded_node_id_to_user_node_id.find(debug_node_id);
		if (it != _program.expanded_node_id_to_user_node_id.end()) {
			debug_node_id = it->second;
		}
	}

	// debug_print_operations();

	result.expanded_nodes_count = expanded_graph.get_nodes_count();
	return result;
}

namespace {

// 优化图中仅依赖于标记为"外部组"的输入的部分，
// 以便在生成数据块时将它们移到外层循环中，减少运行次数。
// 将它们全部移到开头。
// `order` 是先前计算出的每个节点的执行顺序。
uint32_t move_outer_group_operations_up(StdVector<uint32_t> &order, const ProgramGraph &graph) {
	VOXEL_PROFILE_SCOPE();
	StdVector<uint32_t> immediate_deps;
	StdUnorderedSet<uint32_t> outer_group_node_ids;
	StdVector<uint32_t> order_outer_group;
	StdVector<uint32_t> order_inner_group;

	for (const uint32_t node_id : order) {
		const ProgramGraph::Node &node = graph.get_node(node_id);

		// 我们将 X 和 Z 视为外部组的起点。
		bool is_outer_group =
				node.type_id == VoxelGraphFunction::NODE_INPUT_X || node.type_id == VoxelGraphFunction::NODE_INPUT_Z;

		if (!is_outer_group) {
			immediate_deps.clear();
			graph.find_immediate_dependencies(node_id, immediate_deps);

			// 假定为外部组，除非某个依赖项不在外部组中
			is_outer_group = true;
			for (const uint32_t dep_node_id : immediate_deps) {
				if (outer_group_node_ids.find(dep_node_id) == outer_group_node_ids.end()) {
					is_outer_group = false;
					break;
				}
			}
		}

		if (is_outer_group) {
			order_outer_group.push_back(node_id);
			outer_group_node_ids.insert(node_id);
		} else {
			order_inner_group.push_back(node_id);
		}
	}

	const uint32_t inner_group_start_index = order_outer_group.size();

	size_t i = 0;
	for (const uint32_t node_id : order_outer_group) {
		order[i++] = node_id;
	}
	for (const uint32_t node_id : order_inner_group) {
		order[i++] = node_id;
	}
	VOXEL_ASSERT(i == order.size());

	return inner_group_start_index;
}

void compute_node_execution_order(
		StdVector<uint32_t> &order,
		const ProgramGraph &graph,
		bool debug,
		const NodeTypeDB &type_db
) {
	StdVector<uint32_t> terminal_nodes;

	// 不使用通用的 `get_terminal_nodes` 函数，因为我们的终端节点确实有输出
	graph.for_each_node_const([&terminal_nodes, &type_db](const ProgramGraph::Node &node) {
		const NodeType &type = type_db.get_type(node.type_id);
		if (type.category == pg::CATEGORY_OUTPUT) {
			terminal_nodes.push_back(node.id);
		}
	});

	if (!debug) {
		// 排除调试节点
		unordered_remove_if(terminal_nodes, [&graph, &type_db](uint32_t node_id) {
			const ProgramGraph::Node &node = graph.get_node(node_id);
			const NodeType &type = type_db.get_type(node.type_id);
			return type.debug_only;
		});
	}

	graph.find_dependencies(to_span(terminal_nodes), order);
}

} // namespace

CompilationResult Runtime::compile_preprocessed_graph(
		Program &program,
		const ProgramGraph &graph,
		const unsigned int input_count,
		Span<const uint32_t> input_node_ids,
		const bool debug,
		const NodeTypeDB &type_db
) {
	VOXEL_PROFILE_SCOPE();
	program.clear();

	program.inputs.resize(input_count);

	StdVector<uint32_t> order;
	compute_node_execution_order(order, graph, debug, type_db);

	const uint32_t inner_group_start_index = move_outer_group_operations_up(order, graph);

	struct MemoryHelper {
		StdVector<BufferSpec> &buffer_specs;
		unsigned int next_address = 0;

		uint16_t add_binding() {
			const unsigned int a = next_address;
			++next_address;
			BufferSpec bs;
			bs.address = a;
			bs.is_binding = true;
			bs.is_constant = false;
			bs.users_count = 0;
			buffer_specs.push_back(bs);
			return a;
		}

		uint16_t add_var() {
			const unsigned int a = next_address;
			++next_address;
			BufferSpec bs;
			bs.address = a;
			bs.is_binding = false;
			bs.is_constant = false;
			bs.is_pinned = false;
			bs.users_count = 0;
			buffer_specs.push_back(bs);
			return a;
		}

		uint16_t add_constant(float v, bool buffer_required) {
			const unsigned int a = next_address;
			++next_address;
			BufferSpec bs;
			bs.address = a;
			bs.constant_value = v;
			bs.is_binding = false;
			bs.is_constant = true;
			bs.is_pinned = buffer_required;
			bs.users_count = 0;
			buffer_specs.push_back(bs);
			return a;
		}
	};

	MemoryHelper mem{ program.buffer_specs };

	StdVector<uint16_t> &operations = program.operations;
	StdUnorderedMap<uint32_t, uint32_t> node_id_to_dependency_graph;
	StdVector<uint16_t> input_buffer_indices;

	// 分配输入槽
	// 注意，即使输入未连接到任何东西，它仍然会获得其绑定空间（但它不会出现在 `order` 中）。
	// 这意味着查询仍会包含该输入，因为源图就是这样定义的，但运行时
	// 不会使用它。
	for (unsigned int input_index = 0; input_index < input_node_ids.size(); ++input_index) {
		const uint32_t node_id = input_node_ids[input_index];

#ifdef DEBUG_ENABLED
		const ProgramGraph::Node &node = graph.get_node(node_id);
		VOXEL_ASSERT(node.inputs.size() == 0);
		VOXEL_ASSERT(node.outputs.size() == 1);
#endif

		InputInfo &input = program.inputs[input_index];
		input.buffer_address = mem.add_binding();
		program.output_port_addresses[ProgramGraph::PortLocation{ node_id, 0 }] = input.buffer_address;

		const unsigned int dg_node_index = program.dependency_graph.nodes.size();
		program.dependency_graph.nodes.push_back(DependencyGraph::Node());
		DependencyGraph::Node &dg_node = program.dependency_graph.nodes.back();
		dg_node.is_input = true;
		dg_node.op_address = 0;
		dg_node.first_dependency = 0;
		dg_node.end_dependency = 0;
		dg_node.debug_node_id = node_id;
		node_id_to_dependency_graph.insert(std::make_pair(node_id, dg_node_index));
	}

	// 按顺序遍历每个节点，并将它们转换为程序指令
	for (size_t order_index = 0; order_index < order.size(); ++order_index) {
		const uint32_t node_id = order[order_index];
		const ProgramGraph::Node &node = graph.get_node(node_id);
		const NodeType &type = type_db.get_type(node.type_id);

		VOXEL_ASSERT(node.inputs.size() == type.inputs.size());
		VOXEL_ASSERT(node.outputs.size() == type.outputs.size());

		if (order_index == inner_group_start_index) {
			program.inner_group_start_op_index = operations.size();
		}

		const unsigned int dg_node_index = program.dependency_graph.nodes.size();
		program.dependency_graph.nodes.push_back(DependencyGraph::Node());
		DependencyGraph::Node &dg_node = program.dependency_graph.nodes.back();
		dg_node.is_input = false;
		dg_node.op_address = operations.size();
		dg_node.first_dependency = program.dependency_graph.dependencies.size();
		dg_node.end_dependency = dg_node.first_dependency;
		dg_node.debug_node_id = node_id;
		node_id_to_dependency_graph.insert(std::make_pair(node_id, dg_node_index));

		// 我们仍然硬编码一些节点。也许有一天我们也可以将它们抽象化。
		switch (node.type_id) {
			// TODO 移除常量节点，在任何使用它们的地方用默认输入替换？
			case VoxelGraphFunction::NODE_CONSTANT: {
				VOXEL_ASSERT(type.outputs.size() == 1);
				VOXEL_ASSERT(type.params.size() == 1);
				const uint16_t a = mem.add_constant(node.params[0].operator float(), true);
				program.output_port_addresses[ProgramGraph::PortLocation{ node_id, 0 }] = a;
				// 从技术上讲不是输入或输出，但无论如何它都是一个依赖项，所以将其视为输入
				dg_node.is_input = true;
				continue;
			}

			case VoxelGraphFunction::NODE_INPUT_X:
			case VoxelGraphFunction::NODE_INPUT_Y:
			case VoxelGraphFunction::NODE_INPUT_Z:
			case VoxelGraphFunction::NODE_INPUT_SDF:
			case VoxelGraphFunction::NODE_CUSTOM_INPUT: {
				if (!contains(input_node_ids, node_id)) {
					CompilationResult result;
					result.success = false;
					result.message =
							VOXEL_TTR("Used input node isn't registered. Remove it, or add it to function inputs.");
					result.node_id = node_id;
					return result;
				}
				// 在之前已处理
				continue;
			}

			case VoxelGraphFunction::NODE_SDF_PREVIEW: {
				if (!debug) {
					VOXEL_PRINT_WARNING(
							"Found preview node when compiling graph in non-debug mode. That node should not "
							"have been present. Bug?"
					);
				}
				auto it = program.output_port_addresses.find(ProgramGraph::PortLocation{ node_id, 0 });
				if (it != program.output_port_addresses.end()) {
					const uint16_t a = it->second;
					VOXEL_ASSERT(a < program.buffer_specs.size());
					BufferSpec &src_buffer_spec = program.buffer_specs[a];
					// 添加一个假用户，我们想看到它们的结果。
					// 固定也可以，但它会分配更多缓冲区。
					// src_buffer_spec.is_pinned = true;
					++src_buffer_spec.users_count;
				}
				continue;
			}
		}

		// 添加实际操作

		VOXEL_ASSERT(node.type_id <= std::numeric_limits<uint16_t>::max());

		if (order_index == inner_group_start_index) {
			program.default_execution_map.inner_group_start_index = program.default_execution_map.operations.size();
		}
		program.default_execution_map.operations.push_back(
				ExecutionMap::OperationInfo{ uint16_t(operations.size()), 0 }
		);
		if (debug) {
			// 如果节点是展开后的节点，稍后将进行重映射
			program.default_execution_map.debug_nodes.push_back(node_id);
		}

		operations.push_back(node.type_id);

		// 输入和输出使用约定，以便我们为它们编写通用代码。
		// 参数更具体，并且可能受对齐影响，因此最好手工处理

		// 添加输入
		for (size_t j = 0; j < type.inputs.size(); ++j) {
			const NodeType::Port &port = type.inputs[j];
			uint16_t a;

			if (node.inputs[j].connections.size() == 0) {
				// 没有输入，使用默认值
				VOXEL_ASSERT(j < node.default_inputs.size());
				float defval = node.default_inputs[j];
				a = mem.add_constant(defval, port.require_input_buffer_when_constant);

			} else {
				const ProgramGraph::PortLocation src_port = node.inputs[j].connections[0];
				auto address_it = program.output_port_addresses.find(src_port);
				// 之前的节点端口必须已经注册
				VOXEL_ASSERT(address_it != program.output_port_addresses.end());
				a = address_it->second;

				// 注册依赖项
				auto it = node_id_to_dependency_graph.find(src_port.node_id);
				VOXEL_ASSERT(it != node_id_to_dependency_graph.end());
				VOXEL_ASSERT(it->second < program.dependency_graph.nodes.size());
				program.dependency_graph.dependencies.push_back(it->second);
				++dg_node.end_dependency;
			}

			operations.push_back(a);

			VOXEL_ASSERT(a < program.buffer_specs.size());
			BufferSpec &bs = program.buffer_specs[a];
			++bs.users_count;

			input_buffer_indices.push_back(a);
		}

		// 添加输出
		for (size_t j = 0; j < type.outputs.size(); ++j) {
			// 注意，输出节点的输出可以被固定，但由于它们被赋予一个假用户，其生命周期
			// 是不确定的，永远不会被内存分配重用。这比固定更好，因为
			// 缓冲区在到达输出之前可以被多次重用，而固定的缓冲区始终
			// 在其位置上唯一。
			const uint16_t a = mem.add_var();

			// 这将被下一个节点使用
			const ProgramGraph::PortLocation op{ node_id, static_cast<uint32_t>(j) };
			program.output_port_addresses[op] = a;

			operations.push_back(a);
		}

		// 获取参数，使用时复制资源并持有对它们的引用
		StdVector<Variant> params_copy;
		params_copy.reserve(node.params.size());
		for (size_t i = 0; i < node.params.size(); ++i) {
			Variant v = node.params[i];

			if (v.get_type() == Variant::OBJECT) {
				Ref<Resource> res = v;

				if (res.is_null()) {
					// duplicate() 仅在 Resource 中可用，
					// 因此我们必须限制为 Resource 而不是 Reference 或 Object
					CompilationResult result;
					result.success = false;
					result.message = VOXEL_TTR("A parameter is an object but does not inherit Resource");
					result.node_id = node_id;
					return result;
				}

				res = res->duplicate();

				program.ref_resources.push_back(res);
				v = res;
			}

			params_copy.push_back(v);
		}

		{
			const CompilationResult res =
					compile_params(type, node_id, operations, program.heap_resources, to_span(params_copy));
			if (!res.success) {
				return res;
			}
		}

		if (type.category == pg::CATEGORY_OUTPUT) {
			VOXEL_ASSERT(node.outputs.size() == 1);
			VOXEL_ASSERT(node.outputs[0].connections.size() == 0);

			if (program.outputs_count == program.outputs.size()) {
				CompilationResult result;
				result.success = false;
				result.message = VOXEL_TTR("Maximum number of outputs has been reached");
				result.node_id = node_id;
				return result;
			}

			{
				auto address_it = program.output_port_addresses.find(ProgramGraph::PortLocation{ node_id, 0 });
				// 之前的节点端口必须已经注册
				VOXEL_ASSERT(address_it != program.output_port_addresses.end());
				OutputInfo &output_info = program.outputs[program.outputs_count];
				output_info.buffer_address = address_it->second;
				output_info.dependency_graph_node_index = dg_node_index;
				output_info.node_id = node_id;
				++program.outputs_count;
			}

			// 为输出端口添加假用户，以便它们能通过优化中的本地用户检查
			for (unsigned int j = 0; j < type.outputs.size(); ++j) {
				const ProgramGraph::PortLocation loc{ node_id, j };
				auto address_it = program.output_port_addresses.find(loc);
				VOXEL_ASSERT(address_it != program.output_port_addresses.end());
				BufferSpec &bs = program.buffer_specs[address_it->second];
				// 不期望该端口已有用户
				VOXEL_ASSERT_RETURN_V(bs.users_count == 0, CompilationResult());
				++bs.users_count;
			}
		}

#ifdef VOXEL_DEBUG_GRAPH_PROG_SENTINEL
		// 在每个操作之后附加一个特殊值
		append(operations, VOXEL_DEBUG_GRAPH_PROG_SENTINEL);
#endif
	}

	program.buffer_count = mem.next_address;

	// 固定外部组中被子组操作读取的缓冲区。
	// 来自外部组的缓冲区数据，如果被子组读取，则必须固定，
	// 因为它在多次执行之间会被重用。
	{
		Span<BufferSpec> buffer_specs = to_span(program.buffer_specs);

		// 对于子组的每个节点
		for (unsigned int order_index = inner_group_start_index; order_index < order.size(); ++order_index) {
			const uint32_t node_id = order[order_index];
			const ProgramGraph::Node &node = graph.get_node(node_id);
			// const NodeType &type = type_db.get_type(node.type_id);

			for (const ProgramGraph::Port &input : node.inputs) {
				if (input.connections.size() == 0) {
					continue;
				}
				const ProgramGraph::PortLocation src_port = input.connections[0];

				// 查找源节点是否属于 XZ 组
				bool found = false;
				for (unsigned int i = 0; i < inner_group_start_index; ++i) {
					if (order[i] == src_port.node_id) {
						found = true;
						break;
					}
				}
				if (!found) {
					continue;
				}

				auto address_it = program.output_port_addresses.find(src_port);
				// 之前的节点端口必须已经注册
				VOXEL_ASSERT(address_it != program.output_port_addresses.end());
				BufferSpec &src_buffer_spec = buffer_specs[address_it->second];
				src_buffer_spec.is_pinned = true;
			}
		}
	}

	// 分配缓冲区数据
	{
		struct DataHelper {
			StdVector<uint16_t> free_indices;
			struct Data {
				uint16_t usages;
				bool pinned;
			};
			StdVector<Data> datas;

			uint16_t allocate(uint16_t users, bool pinned) {
				VOXEL_ASSERT(users > 0);
				// 注意，固定缓冲区必须有唯一的数据，因此我们不能为它们重用之前的缓冲区
				if (free_indices.size() == 0 || pinned) {
					const uint16_t i = datas.size();
					datas.push_back(Data{ users, pinned });
					return i;
				} else {
					const uint16_t i = free_indices[free_indices.size() - 1];
					free_indices.pop_back();
					VOXEL_ASSERT(i < datas.size());
					Data &d = datas[i];
					// 不能重用固定的缓冲区
					VOXEL_ASSERT(!d.pinned);
					d.usages = users;
					return i;
				}
			}

			void unref(uint16_t i) {
				VOXEL_ASSERT(i < datas.size());
				Data &d = datas[i];
				VOXEL_ASSERT(!d.pinned);
				VOXEL_ASSERT(d.usages > 0);
				--d.usages;
				if (d.usages == 0) {
					free_indices.push_back(i);
				}
			}
		};

		DataHelper data_helper;

		if (debug) {
			// 在调试模式下，不进行缓冲区数据重用优化
			for (BufferSpec &buffer_spec : program.buffer_specs) {
				if (!buffer_spec.is_binding) {
					// 在 DataHelper 中将所有用途硬编码为 1，我们完全不跟踪它们。
					buffer_spec.data_index = data_helper.allocate(1, true);
					buffer_spec.has_data = true;
				}
			}

		} else {
			Span<BufferSpec> buffer_specs = to_span(program.buffer_specs);

			// 分配唯一缓冲区（主要用于需要缓冲区的编译期常量）
			for (BufferSpec &buffer_spec : program.buffer_specs) {
				if (!buffer_spec.is_binding && buffer_spec.is_pinned) {
					buffer_spec.data_index = data_helper.allocate(1, true);
					buffer_spec.has_data = true;
				}
			}

			// 分配可重用的缓冲区。
			// 按执行顺序遍历每个节点，在需要时使用池化逻辑分配缓冲区，
			// 以便在运行生成器之前预先计算出总共实际需要哪些缓冲区。
			// 这比给每个缓冲区唯一数据占用更少的内存。
			for (unsigned int order_index = 0; order_index < order.size(); ++order_index) {
				const uint32_t node_id = order[order_index];
				const ProgramGraph::Node &node = graph.get_node(node_id);
				const NodeType &type = type_db.get_type(node.type_id);

				uint16_t throwaway_data_index = 0;
				bool has_throwaway_data = false;

				// 分配数据以存储输出。
				// 注意，我们不为输入分配数据。分配它们的唯一方法是固定它们。
				for (unsigned int output_index = 0; output_index < type.outputs.size(); ++output_index) {
					const ProgramGraph::PortLocation dst_port{ node_id, output_index };
					auto address_it = program.output_port_addresses.find(dst_port);
					VOXEL_ASSERT(address_it != program.output_port_addresses.end());
					BufferSpec &buffer_spec = buffer_specs[address_it->second];

					if (buffer_spec.is_binding || buffer_spec.is_pinned) {
						continue;
					}
					if (buffer_spec.users_count > 0) {
						buffer_spec.data_index = data_helper.allocate(buffer_spec.users_count, false);
					} else {
						// 该节点将被运行，但有一个未使用的输出。我们必须分配一个一次性缓冲区。
						// 如果同一节点上有更多未使用的输出，我们应该能够使用同一个缓冲区，但不能
						// 与正在使用的缓冲区相同。
						if (!has_throwaway_data) {
							has_throwaway_data = true;
							throwaway_data_index = data_helper.allocate(1, false);
						}
						buffer_spec.data_index = throwaway_data_index;
					}
					buffer_spec.has_data = true;
				}

				if (has_throwaway_data) {
					// 一旦该节点运行完，就让此缓冲区可再次使用
					data_helper.unref(throwaway_data_index);
				}

				// 释放对输入数据的引用，以便它们可以被后续操作重用
				for (const ProgramGraph::Port &input : node.inputs) {
					if (input.connections.size() == 0) {
						continue;
					}
					const ProgramGraph::PortLocation src_port = input.connections[0];
					auto address_it = program.output_port_addresses.find(src_port);
					VOXEL_ASSERT(address_it != program.output_port_addresses.end());
					const BufferSpec &buffer_spec = buffer_specs[address_it->second];

					// 绑定是用户提供的。
					// 固定缓冲区永远不会被重用。
					if (buffer_spec.is_binding || buffer_spec.is_pinned) {
						continue;
					}

					data_helper.unref(buffer_spec.data_index);
				}
			}
		}

		program.buffer_data_count = data_helper.datas.size();
	}

	VOXEL_PRINT_VERBOSE(
			format("Compiled voxel graph. Program size: {}b, ports: {}, buffers: {}",
				   program.operations.size() * sizeof(uint16_t),
				   program.buffer_count,
				   program.buffer_data_count)
	);

	CompilationResult result;
	result.success = true;
	return result;
}

} // namespace voxel::pg
