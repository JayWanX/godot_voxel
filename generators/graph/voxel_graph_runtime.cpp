#include "voxel_graph_runtime.h"
#include "../../util/containers/container_funcs.h"
#include "../../util/godot/core/string.h"
#include "../../util/io/log.h"
#include "../../util/macros.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"
#ifdef TOOLS_ENABLED
#include "../../util/profiling_clock.h"
#endif
#include "../../util/io/std_string_text_writer.h"
#include "node_type_db.h"
#include "voxel_generator_graph.h"

#include <unordered_set>

// #ifdef DEBUG_ENABLED
// #define VOXEL_DEBUG_GRAPH_PROG_SENTINEL uint16_t(12345) // 48, 57 (base 10)
// #endif

namespace voxel::pg {

Runtime::Runtime() {
	clear();
}

Runtime::~Runtime() {
	clear();
}

void Runtime::clear() {
	_program.clear();
}

namespace {

Span<const uint16_t> get_outputs_from_op_address(Span<const uint16_t> operations, uint16_t op_address) {
	const uint16_t opid = operations[op_address];
	const NodeType &node_type = NodeTypeDB::get_singleton().get_type(opid);

	const uint32_t inputs_count = node_type.inputs.size();
	const uint32_t outputs_count = node_type.outputs.size();

	// +1 是为 `opid` 预留的
	return operations.sub(op_address + 1 + inputs_count, outputs_count);
}

} // namespace

bool Runtime::is_operation_constant(const State &state, uint16_t op_address) const {
	Span<const uint16_t> outputs = get_outputs_from_op_address(to_span_const(_program.operations), op_address);

	for (unsigned int i = 0; i < outputs.size(); ++i) {
		const uint16_t output_address = outputs[i];
		const Buffer &buffer = state.get_buffer(output_address);
		if (!(buffer.is_constant //
			  || state.get_range(output_address).is_single_value() //
			  || buffer.local_users_count == 0)) {
			// 至少有一个输出在当前区域无法被预测
			return false;
		}
	}

	return true;
}

void Runtime::generate_optimized_execution_map(const State &state, ExecutionMap &execution_map, bool debug) const {
	FixedArray<unsigned int, MAX_OUTPUTS> all_outputs;
	for (unsigned int i = 0; i < _program.outputs_count; ++i) {
		all_outputs[i] = i;
	}
	generate_optimized_execution_map(state, execution_map, to_span_const(all_outputs, _program.outputs_count), debug);
}

const Runtime::ExecutionMap &Runtime::get_default_execution_map() const {
	return _program.default_execution_map;
}

// 生成要执行的操作地址列表，
// 跳过那些被上次范围分析判定为常量的操作。
// 如果一个非常量操作仅对常量操作有贡献，它也会被跳过。
// 这样可以在运行时进行局部优化，而不必依赖显式的条件分支。
// 这对生物群系很有用，其中某些分支在未用于最终混合时会变为常量。
void Runtime::generate_optimized_execution_map(
		const State &state,
		ExecutionMap &execution_map,
		Span<const unsigned int> required_outputs,
		bool debug
) const {
	VOXEL_PROFILE_SCOPE();

	// 必须已经计算出范围分析结果
	VOXEL_ASSERT_RETURN(state.ranges.size() != 0);

	const Program &program = _program;
	const DependencyGraph &graph = program.dependency_graph;

	execution_map.clear();

	// if (program.default_execution_map.size() == 0) {
	// 	// 不能再减少更多
	// 	return;
	// }

	// 此函数会运行很多次，所以最好复用同一个向量
	static thread_local StdVector<uint16_t> to_process;
	to_process.clear();

	for (unsigned int i = 0; i < required_outputs.size(); ++i) {
		const unsigned int output_index = required_outputs[i];
		const unsigned int dg_index = program.outputs[output_index].dependency_graph_node_index;
		to_process.push_back(dg_index);
	}

	enum ProcessResult { NOT_PROCESSED, SKIPPABLE, REQUIRED };

	static thread_local StdVector<ProcessResult> results;
	results.clear();
	results.resize(graph.nodes.size(), NOT_PROCESSED);

	while (to_process.size() != 0) {
		const uint32_t node_index = to_process.back();
		const unsigned int to_process_previous_size = to_process.size();

		// 需要检查，因为 Godot 从不以 `_DEBUG` 编译...
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(node_index < graph.nodes.size());
#endif
		const DependencyGraph::Node &node = graph.nodes[node_index];

		// 忽略输入，因为它们在操作列表中不存在
		if (!node.is_input && is_operation_constant(state, node.op_address)) {
			// 暂时跳过此操作。
			// 如果没有其他依赖到达它，它将在结果中被有效地跳过。
			to_process.pop_back();
			results[node_index] = SKIPPABLE;
			continue;
		}

		for (uint32_t i = node.first_dependency; i < node.end_dependency; ++i) {
			const uint32_t dep_node_index = graph.dependencies[i];
			if (results[dep_node_index] != NOT_PROCESSED) {
				// 已处理
				continue;
			}
			to_process.push_back(dep_node_index);
		}

		if (to_process_previous_size == to_process.size()) {
			to_process.pop_back();
			results[node_index] = REQUIRED;
		}
	}

	if (debug) {
		StdVector<uint32_t> &debug_nodes = execution_map.debug_nodes;
		VOXEL_ASSERT(debug_nodes.size() == 0);

		for (unsigned int node_index = 0; node_index < graph.nodes.size(); ++node_index) {
			const ProcessResult res = results[node_index];
			const DependencyGraph::Node &node = graph.nodes[node_index];

			if (res == REQUIRED) {
				uint32_t debug_node_id = node.debug_node_id;
				auto it = _program.expanded_node_id_to_user_node_id.find(debug_node_id);

				if (it != _program.expanded_node_id_to_user_node_id.end()) {
					debug_node_id = it->second;
					// if (std::find(debug_nodes.begin(), debug_nodes.end(), debug_node_id) != debug_nodes.end()) {
					// 	// 忽略重复项。某些节点可能已被展开为多个节点。
					// 	continue;
					// }
				}

				debug_nodes.push_back(debug_node_id);
			}
		}
	}

	Span<const uint16_t> operations(program.operations.data(), 0, program.operations.size());
	bool inner_group_start_not_assigned = true;

	static thread_local StdVector<ExecutionMap::ConstantFill> tls_constant_fills;
	tls_constant_fills.clear();

	// 现在我们必须在缓冲区中填充可能找到的局部常量。
	// 我们主要遍历节点，因为我们必须相对于外层循环优化保持一定的顺序。
	for (unsigned int node_index = 0; node_index < graph.nodes.size(); ++node_index) {
		const ProcessResult res = results[node_index];
		const DependencyGraph::Node &node = graph.nodes[node_index];

		if (node.is_input) {
			continue;
		}

		switch (res) {
			case NOT_PROCESSED:
				continue;

			case SKIPPABLE: {
				const Span<const uint16_t> outputs = get_outputs_from_op_address(operations, node.op_address);

				for (unsigned int output_index = 0; output_index < outputs.size(); ++output_index) {
					const uint16_t output_address = outputs[output_index];
					const Buffer &buffer = state.get_buffer(output_address);

					if (buffer.is_constant) {
						// 已在准备阶段赋值
						continue;
					}

					VOXEL_ASSERT(!buffer.is_binding);

					// 该节点被视为可跳过，意味着其输出要么是局部常量，要么未被使用。
					// 未使用的缓冲区可以保持不变，但局部常量必须被填充。
					if (buffer.local_users_count > 0) {
						const math::Interval range = state.ranges[output_address];
						// 如果此区间不是单值，那么该节点本不应被跳过
						VOXEL_ASSERT(range.is_single_value());
						const float v = range.min;
						// 当我们在多个节点中复用缓冲区数据时，若在此处填充常量数据，
						// 该优化无法可靠工作。数据指针可能被在可跳过节点之前
						// 复用它的另一个操作写入，这会覆盖我们预期的值。
						// 为避免此问题，将填充延迟到读取该缓冲区的第一个节点运行之前。
						// 这样做的原因是为了避免为每种常量参数与缓冲区的组合重写操作。
						VOXEL_ASSERT(buffer.data != nullptr);
						tls_constant_fills.push_back(ExecutionMap::ConstantFill{ buffer.data, v });
					}
				}
			} break;

			case REQUIRED:
				if (inner_group_start_not_assigned && node.op_address >= program.inner_group_start_op_index) {
					// 只要图中的节点列表遵循与 `compile()` 中相同的重排序优化顺序
					// （所有不依赖 Y 的节点排在前面），这就是正确的
					execution_map.inner_group_start_index = execution_map.operations.size();
					inner_group_start_not_assigned = false;
				}

				execution_map.operations.push_back(
						ExecutionMap::OperationInfo{ node.op_address, uint16_t(tls_constant_fills.size()) }
				);

				// TODO 只做实际被使用的常量填充
				// 以下方法并非最优。如果图的 50% 被跳过，且剩余节点不使用
				// 被跳过部分的常量输出，它们最终仍会被填充...

				// 使之前被跳过的节点的局部常量输出被填充，就好像它们已经运行过一样。
				// 它们不一定会被当前节点使用，但根据缓冲区生命周期规则，
				// 它们应保持有效，直到所有使用者都已读取它们。
				for (const ExecutionMap::ConstantFill &cf : tls_constant_fills) {
					execution_map.constant_fills.push_back(cf);
				}
				tls_constant_fills.clear();

				break;

			default:
				CRASH_NOW();
				break;
		}
	}
}

void Runtime::generate_single(State &state, Span<const float> inputs, const ExecutionMap *execution_map) const {
	FixedArray<Span<const float>, MAX_INPUTS> input_bindings;
	VOXEL_ASSERT_RETURN_MSG(inputs.size() < input_bindings.size(), "Too many inputs, not supported");
	for (unsigned int i = 0; i < inputs.size(); ++i) {
		input_bindings[i] = Span<const float>(&inputs[i], 1);
	}
	generate_set(state, to_span(input_bindings, inputs.size()), false, execution_map);
}

void Runtime::prepare_state(State &state, unsigned int buffer_size, bool with_profiling) const {
	// 分配内存

	const unsigned int old_buffer_data_count = state.buffer_datas.size();
	if (state.buffer_datas.size() < _program.buffer_data_count) {
		// 创建更多缓冲区数据。
		state.buffer_datas.resize(_program.buffer_data_count);
		for (unsigned int i = old_buffer_data_count; i < state.buffer_datas.size(); ++i) {
			BufferData &bd = state.buffer_datas[i];
			VOXEL_ASSERT(bd.data == nullptr);
			// 这些是新条目，我们总是进行分配。
			bd.data = reinterpret_cast<float *>(VOXEL_ALLOC(buffer_size * sizeof(float)));
			bd.capacity = buffer_size;
		}
	}

	if (state.buffer_size < buffer_size) {
		// 扩大现有的缓冲区数据。
		for (unsigned int i = 0; i < old_buffer_data_count; ++i) {
			BufferData &bd = state.buffer_datas[i];
			VOXEL_ASSERT(bd.data != nullptr);
			if (bd.capacity < buffer_size) {
				// 这些是已有条目，我们总是重新分配。
				bd.data = reinterpret_cast<float *>(VOXEL_REALLOC(bd.data, buffer_size * sizeof(float)));
				bd.capacity = buffer_size;
			}
		}
		// TODO 不确定是否值得在状态层面保留容量。缓冲区数据可能具有不同的容量，具体取决于
		// 之前准备了哪些图。
		state.buffer_capacity = buffer_size;
	}

	// 初始化缓冲区

#if DEBUG_ENABLED
	for (const Buffer &buffer : state.buffers) {
		if (buffer.is_binding) {
			// 忘记解绑了？
			VOXEL_ASSERT(buffer.data == nullptr);
		}
	}
#endif

	state.buffers.resize(_program.buffer_count);
	state.ranges.resize(_program.buffer_count);
	// 注意：这必须在我们调整向量大小之后进行。
	// 这样做主要是因为 Godot 编译时不带标准库的边界检查...
	Span<Buffer> buffers = to_span(state.buffers);
	Span<BufferData> buffer_datas = to_span(state.buffer_datas);
	Span<math::Interval> ranges = to_span(state.ranges);

	state.buffer_size = buffer_size;

	for (const BufferSpec &buffer_spec : _program.buffer_specs) {
		Buffer &buffer = buffers[buffer_spec.address];

		if (buffer_spec.has_data) {
			VOXEL_ASSERT(!buffer_spec.is_binding);
			BufferData &bd = buffer_datas[buffer_spec.data_index];
			VOXEL_ASSERT(bd.capacity >= buffer_size);
			buffer.data = bd.data;
		} else {
			VOXEL_ASSERT(buffer_spec.is_binding || buffer_spec.is_constant);
			buffer.data = nullptr;
		}

		buffer.is_binding = buffer_spec.is_binding;
		buffer.is_constant = buffer_spec.is_constant;
		buffer.size = buffer_size;
		buffer.buffer_data_index = buffer_spec.data_index;

		// 总是重置常量，因为我们不知道是否会像之前一样运行同一个程序...
		if (buffer_spec.is_constant) {
			buffer.constant_value = buffer_spec.constant_value;
			// 如果已确定使用此端口的节点不需要缓冲区，则数据可能为 null。
			if (buffer.data != nullptr) {
				for (unsigned int i = 0; i < buffer_size; ++i) {
					buffer.data[i] = buffer_spec.constant_value;
				}
			}
			ranges[buffer_spec.address] = math::Interval::from_single_value(buffer_spec.constant_value);
		}
	}

	// 在缓冲区中放入哨兵值，以检测未初始化的缓冲区
	// #ifdef DEBUG_ENABLED
	// 	for (unsigned int i = 0; i < state.buffers.size(); ++i) {
	// 		Buffer &buffer = state.buffers[i];
	// 		if (!buffer.is_constant && !buffer.is_binding) {
	// 			VOXEL_ASSERT(buffer.data != nullptr);
	// 			for (unsigned int j = 0; j < buffer.size; ++j) {
	// 				buffer.data[j] = -969696.f;
	// 			}
	// 		}
	// 	}
	// #endif

	/*if (use_range_analysis) {
		// TODO 要真正做到物有所值，我们可能需要一个运行时图遍历过程，
		// 在其中构建值得执行的节点执行映射 🔨

		const float ra_min = _memory[i];
		const float ra_max = _memory[i + _memory.size() / 2];

		buffer.is_constant = (ra_min == ra_max);
		if (buffer.is_constant) {
			buffer.constant_value = ra_min;
		}
	}*/

	state.debug_profiler_times.clear();
	if (with_profiling) {
		// 给出最大大小
		state.debug_profiler_times.resize(_program.dependency_graph.nodes.size());
	}
}

void Runtime::generate_set(
		State &state,
		Span<const Span<const float>> p_inputs,
		bool skip_outer_group,
		const ExecutionMap *p_execution_map
) const {
	// 我不喜欢在头文件中放置私有辅助函数。
	struct L {
		static inline void bind_input_buffer(Span<Buffer> buffers, int a, Span<const float> d) {
			Buffer &buffer = buffers[a];
			VOXEL_ASSERT(buffer.is_binding);
			// TODO 这很不幸，但以当前设计，我们无法在编译时保证 const 性。
			// 输入绝不应被写入。
			buffer.data = const_cast<float *>(d.data());
			buffer.size = d.size();
		}

		static inline void unbind_buffer(Span<Buffer> buffers, int a) {
			Buffer &buffer = buffers[a];
			VOXEL_ASSERT(buffer.is_binding);
			buffer.data = nullptr;
		}
	};

	VOXEL_PROFILE_SCOPE();

	VOXEL_ASSERT_RETURN(p_inputs.size() == _program.inputs.size());

#ifdef DEBUG_ENABLED
	// 每个数组必须具有相同的大小
	for (unsigned int i = 1; i < p_inputs.size(); ++i) {
		VOXEL_ASSERT(p_inputs[0].size() == p_inputs[i].size());
	}
#endif

#ifdef TOOLS_ENABLED
	VOXEL_ASSERT_RETURN(state.buffers.size() >= _program.buffer_count);
	VOXEL_ASSERT_RETURN(state.buffers.size() != 0);
	const unsigned int buffer_size = p_inputs.size() > 0 ? p_inputs[0].size() : state.buffer_size;
	VOXEL_ASSERT_RETURN(state.buffer_size >= buffer_size);
	VOXEL_ASSERT_RETURN(state.buffers[0].size >= buffer_size);
#ifdef DEBUG_ENABLED
	for (size_t i = 0; i < state.buffers.size(); ++i) {
		const Buffer &b = state.buffers[i];
		VOXEL_ASSERT(b.size >= buffer_size);
		VOXEL_ASSERT(b.size <= state.buffer_capacity);
		VOXEL_ASSERT(b.size == state.buffer_size);
		if (b.data != nullptr && !b.is_binding) {
			VOXEL_ASSERT(b.buffer_data_index < state.buffer_datas.size());
			const BufferData &bd = state.buffer_datas[b.buffer_data_index];
			VOXEL_ASSERT(b.size <= bd.capacity);
		}
	}
#endif
#endif

	Span<Buffer> buffers = to_span(state.buffers);

	// 绑定输入
	for (unsigned int i = 0; i < p_inputs.size(); ++i) {
		L::bind_input_buffer(buffers, _program.inputs[i].buffer_address, p_inputs[i]);
	}

	const Span<const uint16_t> operations(_program.operations.data(), 0, _program.operations.size());

	const ExecutionMap &execution_map = p_execution_map != nullptr ? *p_execution_map : _program.default_execution_map;
	Span<const ExecutionMap::OperationInfo> operation_infos = to_span(execution_map.operations);
	const Span<const ExecutionMap::ConstantFill> constant_fills = to_span(execution_map.constant_fills);

	if (skip_outer_group && operation_infos.size() > 0) {
		const unsigned int offset = execution_map.inner_group_start_index;
		operation_infos = operation_infos.sub(offset);
	}

#ifdef TOOLS_ENABLED
	ProfilingClock profiling_clock;
	const bool profile = state.debug_profiler_times.size() > 0;
#endif

	unsigned int constant_fill_index = 0;

	for (unsigned int execution_map_index = 0; execution_map_index < operation_infos.size(); ++execution_map_index) {
		const ExecutionMap::OperationInfo op_info = operation_infos[execution_map_index];

		for (unsigned int i = 0; i < op_info.constant_fill_count; ++i) {
			const ExecutionMap::ConstantFill &cf = constant_fills[constant_fill_index];
			VOXEL_ASSERT(cf.data != nullptr);
			for (unsigned int j = 0; j < state.buffer_size; ++j) {
				cf.data[j] = cf.value;
			}
			++constant_fill_index;
		}

		unsigned int pc = op_info.address;

		const uint16_t opid = operations[pc++];
		const NodeType &node_type = NodeTypeDB::get_singleton().get_type(opid);

		const uint32_t inputs_count = node_type.inputs.size();
		const uint32_t outputs_count = node_type.outputs.size();

		const Span<const uint16_t> op_inputs = operations.sub(pc, inputs_count);
		pc += inputs_count;
		const Span<const uint16_t> op_outputs = operations.sub(pc, outputs_count);
		pc += outputs_count;

		Span<const uint8_t> op_params = read_params(operations, pc);

		// TODO 如果发生此错误，缓冲区将保持绑定状态！
		VOXEL_ASSERT_RETURN(node_type.process_buffer_func != nullptr);
		ProcessBufferContext ctx(op_inputs, op_outputs, op_params, buffers, p_execution_map != nullptr);
		node_type.process_buffer_func(ctx);

#ifdef TOOLS_ENABLED
		if (profile) {
			const uint32_t elapsed_microseconds = profiling_clock.get_elapsed_microseconds();
			state.add_execution_time(execution_map_index, elapsed_microseconds);
			profiling_clock.restart();
		}
#endif
	}

	// 解绑缓冲区
	for (unsigned int i = 0; i < p_inputs.size(); ++i) {
		L::unbind_buffer(buffers, _program.inputs[i].buffer_address);
	}
}

void Runtime::analyze_range(State &state, Span<const math::Interval> p_inputs) const {
	VOXEL_PROFILE_SCOPE();

#ifdef TOOLS_ENABLED
	ERR_FAIL_COND(state.ranges.size() != _program.buffer_count);
#endif

	VOXEL_ASSERT_RETURN(p_inputs.size() == _program.inputs.size());

	Span<math::Interval> ranges = to_span(state.ranges);
	Span<Buffer> buffers = to_span(state.buffers);

	// 重置用户计数，因为它们在分析期间可能会减少
	for (auto it = _program.buffer_specs.cbegin(); it != _program.buffer_specs.cend(); ++it) {
		const BufferSpec &bs = *it;
		Buffer &b = buffers[bs.address];
		b.local_users_count = bs.users_count;
	}

	for (unsigned int i = 0; i < p_inputs.size(); ++i) {
		const unsigned int bi = _program.inputs[i].buffer_address;
		ranges[bi] = p_inputs[i];
	}

	const Span<const uint16_t> operations(_program.operations.data(), 0, _program.operations.size());

	// 这里的操作必须全部被分析，因为我们将其作为粗粒度阶段处理。
	// 只有细粒度阶段最终可能会跳过某些操作。
	uint32_t pc = 0;
	while (pc < operations.size()) {
		const uint16_t opid = operations[pc++];
		const NodeType &node_type = NodeTypeDB::get_singleton().get_type(opid);

		const uint32_t inputs_count = node_type.inputs.size();
		const uint32_t outputs_count = node_type.outputs.size();

		const Span<const uint16_t> op_inputs = operations.sub(pc, inputs_count);
		pc += inputs_count;
		const Span<const uint16_t> op_outputs = operations.sub(pc, outputs_count);
		pc += outputs_count;

		Span<const uint8_t> op_params = read_params(operations, pc);

		VOXEL_ASSERT_RETURN(node_type.range_analysis_func != nullptr);
		RangeAnalysisContext ctx(op_inputs, op_outputs, op_params, ranges, buffers);
		node_type.range_analysis_func(ctx);

#ifdef VOXEL_DEBUG_GRAPH_PROG_SENTINEL
		// 如果失败，则程序格式不正确
		VOXEL_ASSERT(read<uint16_t>(_program, pc) == VOXEL_DEBUG_GRAPH_PROG_SENTINEL);
#endif
	}
}

#ifdef DEBUG_ENABLED

void Runtime::debug_print_operations() {
	const Span<const uint16_t> operations(_program.operations.data(), 0, _program.operations.size());

	StdStringTextWriter ss;
	unsigned int op_index = 0;
	uint32_t pc = 0;
	while (pc < operations.size()) {
		const uint16_t opid = operations[pc++];
		const NodeType &node_type = NodeTypeDB::get_singleton().get_type(opid);

		const uint32_t inputs_count = node_type.inputs.size();
		const uint32_t outputs_count = node_type.outputs.size();

		const Span<const uint16_t> inputs = operations.sub(pc, inputs_count);
		pc += inputs_count;
		const Span<const uint16_t> outputs = operations.sub(pc, outputs_count);
		pc += outputs_count;

		Span<const uint8_t> params = read_params(operations, pc);

		ss << "[";
		ss << op_index;
		ss << "] op: ";
		ss << node_type.name;
		ss << " in(";
		for (size_t i = 0; i < inputs.size(); ++i) {
			if (i > 0) {
				ss << ", ";
			}
			ss << int(inputs[i]);
		}
		ss << ") out(";
		for (size_t i = 0; i < outputs.size(); ++i) {
			if (i > 0) {
				ss << ", ";
			}
			ss << int(outputs[i]);
		}
		ss << ") ";
		ss << "params(";
		ss << params.size();
		ss << "b)\n";

		++op_index;
	}

	print_line(ss.get_written());
}

#endif

bool Runtime::try_get_output_port_address(ProgramGraph::PortLocation port, uint16_t &out_address) const {
	auto port_it = _program.user_port_to_expanded_port.find(port);
	if (port_it != _program.user_port_to_expanded_port.end()) {
		port = port_it->second;
	}
	auto aptr_it = _program.output_port_addresses.find(port);
	if (aptr_it == _program.output_port_addresses.end()) {
		// 此端口未参与编译结果
		return false;
	}
	out_address = aptr_it->second;
	return true;
}

} // namespace voxel::pg
