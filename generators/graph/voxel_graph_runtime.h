#ifndef VOXEL_GRAPH_RUNTIME_H
#define VOXEL_GRAPH_RUNTIME_H

#include "../../util/containers/fixed_array.h"
#include "../../util/containers/span.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/ref_counted.h"
#include "../../util/math/interval.h"
#include "../../util/math/vector3f.h"
#include <core/math/vector3i.h>
#include "program_graph.h"

namespace voxel::pg {

class VoxelGraphFunction;
class NodeTypeDB;

struct CompilationResult {
	bool success = false;
	int node_id = -1;
	int expanded_nodes_count = 0; // 用于测试和调试
	String message;

	static CompilationResult make_success() {
		CompilationResult res;
		res.success = true;
		return res;
	}

	static CompilationResult make_error(const char *p_message, int p_node_id = -1) {
		CompilationResult res;
		res.success = false;
		res.node_id = p_node_id;
		res.message = p_message;
		return res;
	}
};

// 用于执行体素图生成器的 CPU 虚拟机。
// 这是一个更通用的类，实现了 3D 表达式处理系统的核心。
// 一些专门处理体素数据的逻辑被移到其他类中。
class Runtime {
public:
	static const unsigned int MAX_INPUTS = 8;
	static const unsigned int MAX_OUTPUTS = 24;

	struct BufferData {
		// 拥有这些数据。
		float *data = nullptr;
		unsigned int capacity = 0;
	};

	// 包含节点输出的值
	struct Buffer {
		// 访问与此端口关联的缓冲区数据。必须至少包含 `size` 个值。
		// 该数据不归端口所有。多个端口可以使用同一个缓冲区。
		// TODO 考虑在调试模式下将其包装起来。这是我为数不多的没有这样做的情况之一。
		// 我曾经花了一个小时调试内存损坏，其根源是访问此数据时发生了越界。
		float *data = nullptr;
		// 此大小不是已分配的数量，而是低于容量的可用数量。
		// 所有缓冲区具有相同的可用数量，size 仅是为了方便。
		unsigned int size;
		// unsigned int capacity;
		// 缓冲区的常量值（如果是编译期常量）
		float constant_value;
		// 该缓冲区是否持有编译期常量
		bool is_constant;
		// 该缓冲区是否为用户输入/输出
		bool is_binding = false;
		// 有多少操作将该缓冲区作为输入使用。
		// 该值仅在使用优化的执行映射时相关。
		uint16_t local_users_count;
		// BufferData 池中数据的索引。主要用于调试。
		uint16_t buffer_data_index;
	};

	// 包含针对给定查询要执行的操作地址列表。
	// 如果未进行本地优化，则对于任何位置列表，它都可以保持不变。
	// 如果使用本地优化，它可能在每次查询前被重新计算。
	struct ExecutionMap {
		struct OperationInfo {
			uint16_t address = 0;
			// 在此操作之前要执行多少次常量填充。
			uint16_t constant_fill_count = 0;
		};

		StdVector<OperationInfo> operations;

		// 存储引用面向用户的图的节点 ID。
		// 每个索引对应操作索引。
		// 同一节点可能出现两次，因为有时面向用户的节点会编译为多个节点。
		// 它还可能包含一些用户图中未显式存在的节点（如自动输入）。
		StdVector<uint32_t> debug_nodes;

		// `operations` 列表中此索引之前的每个操作将仅依赖于标记为"外部组"的输入。
		// 从此索引开始的操作将不再仅依赖于外部组。
		unsigned int inner_group_start_index = 0;

		struct ConstantFill {
			float *data = nullptr;
			float value = 0;
		};

		// 使用本地优化时要填充常量的缓冲区列表。
		// 必须使用前进的光标读取此列表，光标按 `OperationInfo` 中指定的数量前进。
		// 注意，最好将其用于动态优化。编译期常量应使用固定的
		// 缓冲区，或者更好的是单个值。
		StdVector<ConstantFill> constant_fills;

		void clear() {
			operations.clear();
			debug_nodes.clear();
			inner_group_start_index = 0;
			constant_fills.clear();
		}
	};

	// 包含程序运行时将修改的数据。
	// 同一状态可以与多个程序重用，但在此之前应该准备好。
	class State {
	public:
		~State() {
			clear();
		}

		inline const Buffer &get_buffer(uint16_t address) const {
			// TODO 只是为了方便，因为 STL 边界检查在 Godot 3 中不工作
			CRASH_COND(address >= buffers.size());
			return buffers[address];
		}

		inline const math::Interval get_range(uint16_t address) const {
			// TODO 只是为了方便，因为 STL 边界检查在 Godot 3 中不工作
			CRASH_COND(address >= buffers.size());
			return ranges[address];
		}

		inline const math::Interval &get_range_const_ref(uint16_t address) const {
			// TODO 只是为了方便，因为 STL 边界检查在 Godot 3 中不工作
			VOXEL_ASSERT(address < buffers.size());
			return ranges[address];
		}

		inline uint32_t get_buffer_size() const {
			return buffer_size;
		}

		void clear() {
			buffer_size = 0;
			// buffer_capacity = 0;
			for (BufferData &bd : buffer_datas) {
				VOXEL_ASSERT(bd.data != nullptr);
				memfree(bd.data);
			}
			buffer_datas.clear();
			buffers.clear();
			ranges.clear();
			debug_profiler_times.clear();
		}

		inline void add_execution_time(const uint32_t execution_map_index, const uint32_t time) {
#if DEBUG_ENABLED
			CRASH_COND(execution_map_index >= debug_profiler_times.size());
#endif
			debug_profiler_times[execution_map_index] += time;
		}

		inline uint32_t get_execution_time(uint32_t execution_map_index) const {
#if DEBUG_ENABLED
			CRASH_COND(execution_map_index >= debug_profiler_times.size());
#endif
			return debug_profiler_times[execution_map_index];
		}

	private:
		friend class Runtime; // TODO 为什么需要 friend？此类是嵌套在内部的

		StdVector<math::Interval> ranges;
		StdVector<Buffer> buffers;
		StdVector<BufferData> buffer_datas;
		// [execution_map_index] => 微秒
		StdVector<uint32_t> debug_profiler_times;

		unsigned int buffer_size = 0;
		unsigned int buffer_capacity = 0;
	};

	struct InputInfo {
		// 注意：以下缓冲区由用户分配。
		// 它们被临时映射到 `State` 内部的同一个缓冲区数组中，
		// 因此我们不需要特定代码来处理它们。这需要知道它们被保留在哪个索引处。
		// 它们必须全部被赋值，程序才能正确运行。
		unsigned int buffer_address = 0;
	};

	// 关于图终端节点的信息
	struct OutputInfo {
		unsigned int buffer_address;
		unsigned int dependency_graph_node_index;
		unsigned int node_id;
	};

	Runtime();
	~Runtime();

	void clear();
	CompilationResult compile(const VoxelGraphFunction &function, bool debug);

	// 在使用带生成函数的状态之前调用此方法。
	// 只需调用一次，直到你想使用不同的图、缓冲区大小或缓冲区数量。
	// 如果这些都不变，你可以继续重用。
	void prepare_state(State &state, unsigned int buffer_size, bool with_profiling) const;

	// 用于仅有一个值的集合生成的便捷方法
	// TODO 评估 pg::Runtime 中是否需要双精度
	void generate_single(State &state, Span<const float> inputs, const ExecutionMap *execution_map) const;

	void generate_set(
			State &state,
			Span<const Span<const float>> p_inputs,
			bool skip_outer_group,
			const ExecutionMap *p_execution_map
	) const;

#ifdef DEBUG_ENABLED
	void debug_print_operations();
#endif

	// bool has_input(unsigned int node_type) const;

	inline unsigned int get_input_count() const {
		return _program.inputs.size();
	}

	inline unsigned int get_output_count() const {
		return _program.outputs_count;
	}

	inline const OutputInfo &get_output_info(unsigned int i) const {
		return _program.outputs[i];
	}

	// 分析输入的一个特定区域，以找出我们可以预期的输出范围。
	// 它可以借助执行映射来加速对 `generate_set` 的调用，
	// 以便在不影响结果的操作上可以跳过优化。
	void analyze_range(State &state, Span<const math::Interval> p_inputs) const;

	// 如果打算在该区域实际生成一组值或单个值，则在 `analyze_range` 之后调用此方法。
	// 这允许使用执行映射优化，直到你选择另一个区域。
	// （即使用此方法时，查询分析区域之外的值可能是无效的）
	void generate_optimized_execution_map(
			const State &state,
			ExecutionMap &execution_map,
			Span<const unsigned int> required_outputs,
			bool debug
	) const;

	// 用于要求所有输出的便捷函数
	void generate_optimized_execution_map(const State &state, ExecutionMap &execution_map, bool debug) const;

	const ExecutionMap &get_default_execution_map() const;

	// 获取特定输出端口的缓冲区地址
	bool try_get_output_port_address(ProgramGraph::PortLocation port, uint16_t &out_address) const;

	uint64_t get_program_hash() const;

	static inline Span<const uint8_t> read_params(Span<const uint16_t> operations, unsigned int &pc) {
		const uint16_t params_size_in_words = operations[pc];
		++pc;
		Span<const uint8_t> params;
		if (params_size_in_words > 0) {
			const size_t params_offset_in_words = operations[pc];
			// 定位到参数开始的对齐位置
			pc += params_offset_in_words;
			params = operations.sub(pc, params_size_in_words).reinterpret_cast_to<const uint8_t>();
			pc += params_size_in_words;
		}
		return params;
	}

	struct HeapResource {
		void *ptr;
		void (*deleter)(void *p);

		void free() {
			VOXEL_ASSERT(deleter != nullptr);
			VOXEL_ASSERT(ptr != nullptr);
			deleter(ptr);
			ptr = nullptr;
		}
	};

	class _ProcessContext {
	public:
		inline _ProcessContext(
				const Span<const uint16_t> inputs,
				const Span<const uint16_t> outputs,
				const Span<const uint8_t> params
		) :
				_inputs(inputs), _outputs(outputs), _params(params) {}

		template <typename T>
		inline const T &get_params() const {
#ifdef DEBUG_ENABLED
			CRASH_COND(sizeof(T) > _params.size());
#endif
			return *reinterpret_cast<const T *>(_params.data());
		}

		inline uint32_t get_input_address(uint32_t i) const {
			return _inputs[i];
		}

	protected:
		inline uint32_t get_output_address(uint32_t i) const {
			return _outputs[i];
		}

	private:
		const Span<const uint16_t> _inputs;
		const Span<const uint16_t> _outputs;
		const Span<const uint8_t> _params;
	};

	// 执行阶段节点实现可使用的函数
	class ProcessBufferContext : public _ProcessContext {
	public:
		inline ProcessBufferContext(
				const Span<const uint16_t> inputs,
				const Span<const uint16_t> outputs,
				const Span<const uint8_t> params,
				Span<Buffer> buffers,
				bool using_execution_map
		) :
				_ProcessContext(inputs, outputs, params),
				_buffers(buffers),
				_using_execution_map(using_execution_map) {}

		inline const Buffer &get_input(uint32_t i) const {
			const uint32_t address = get_input_address(i);
#ifdef DEBUG_ENABLED
			// 使用优化的执行映射时，
			// 如果在范围分析期间缓冲区被标记为没有用户，那么它确实不应被使用，
			// 因为它不会被填充相关数据。如果它仍被使用，
			// 那么结果可能与范围分析预测的完全不同。
			const Buffer &b = _buffers[address];
			ERR_FAIL_COND_V_MSG(
					_using_execution_map && !b.is_binding && b.local_users_count == 0,
					b,
					"buffer marked as 'ignored' is still being used"
			);
#endif
			return _buffers[address];
		}

		inline Buffer &get_output(uint32_t i) {
			const uint32_t address = get_output_address(i);
			return _buffers[address];
		}

		// 使用不同的签名以强制编码者确认该条件
		inline const Buffer &try_get_input(uint32_t i, bool &ignored) {
			const uint32_t address = get_input_address(i);
			const Buffer &b = _buffers[address];
			ignored = _using_execution_map && !b.is_binding && b.local_users_count == 0;
			return b;
		}

	private:
		Span<Buffer> _buffers;
		bool _using_execution_map;
	};

	// 范围分析阶段节点实现可使用的函数
	class RangeAnalysisContext : public _ProcessContext {
	public:
		inline RangeAnalysisContext(
				const Span<const uint16_t> inputs,
				const Span<const uint16_t> outputs,
				const Span<const uint8_t> params,
				Span<math::Interval> ranges,
				Span<Buffer> buffers
		) :
				_ProcessContext(inputs, outputs, params), _ranges(ranges), _buffers(buffers) {}

		inline const math::Interval get_input(uint32_t i) const {
			const uint32_t address = get_input_address(i);
			return _ranges[address];
		}

		inline void set_output(uint32_t i, const math::Interval r) {
			const uint32_t address = get_output_address(i);
			_ranges[address] = r;
		}

		inline void ignore_input(uint32_t i) {
			const uint32_t address = get_input_address(i);
			Buffer &b = _buffers[address];
			--b.local_users_count;
		}

	private:
		Span<math::Interval> _ranges;
		Span<Buffer> _buffers;
	};

	typedef void (*ProcessBufferFunc)(ProcessBufferContext &);
	typedef void (*RangeAnalysisFunc)(RangeAnalysisContext &);

private:
	struct Program;

	static CompilationResult compile_preprocessed_graph(
			Program &program,
			const ProgramGraph &graph,
			const unsigned int input_count,
			Span<const uint32_t> input_node_ids,
			const bool debug,
			const NodeTypeDB &type_db
	);

	bool is_operation_constant(const State &state, uint16_t op_address) const;

	struct BufferSpec {
		// 缓冲区应存储的索引
		uint16_t address = 0;
		// 缓冲区数据应存储的索引。
		// 除非固定，否则多个缓冲区可以相同。
		uint16_t data_index = 0;
		// 有多少节点将该缓冲区作为输入使用
		uint16_t users_count = 0;
		// 编译期常量的值（如果有）
		float constant_value = 0;
		// 缓冲区在编译时是否为常量
		bool is_constant = false;
		// 缓冲区是否为用户输入/输出
		bool is_binding = false;
		// 仅当 `data_index` 实际引用某些内容时才为 `true`。
		// 没有数据的缓冲区是绑定或不需要缓冲区的常量。
		bool has_data = false;
		// 如果为 true，端口将被分配唯一的缓冲区数据。
		// 如果为 false，端口可能与其他端口共享相同的缓冲区数据。
		// TODO 重命名为 `has_unique_data`？
		bool is_pinned = false;
	};

	// 用于运行时优化的预处理只读图。
	struct DependencyGraph {
		struct Node {
			uint16_t first_dependency;
			uint16_t end_dependency;
			uint16_t op_address;
			bool is_input;
			// 展开后的 ProgramGraph 中的节点 ID（非用户提供，因此可能需要重映射）
			uint32_t debug_node_id;
		};

		// 指向 `nodes` 数组的索引
		StdVector<uint16_t> dependencies;
		// 与默认执行映射中顺序相同的节点（但索引可能不匹配）
		StdVector<Node> nodes;

		inline void clear() {
			dependencies.clear();
			nodes.clear();
		}
	};

	// 编译后的程序数据。
	// 编译后保持常量和只读。
	struct Program {
		// 序列化的操作和参数，至少以 uint16 对齐。
		// 它们以如下系列出现：
		//
		// - uint16 opid
		// - uint16 inputs[0..*]
		// - uint16 outputs[0..*]
		// - uint16 parameters_size
		// - uint16 parameters_offset // 从这里前进多少才能到达 `parameters` 的开头
		// - <可选填充>
		// - T parameters，其中 T 可以是任何结构体
		// - <可选填充以保持与 uint16 对齐>
		//
		// 它们应该按运行顺序排列，尽管这不是绝对必需的。
		// 按顺序排列更好，因为内存访问将更可预测。
		StdVector<uint16_t> operations;

		// 描述操作之间的依赖关系。它在编译时生成。
		// 它用于在某个操作可以预测为常量时执行动态优化。
		DependencyGraph dependency_graph;

		// `operations` 内的索引列表，描述默认情况下它们应该按什么顺序运行。
		// 它之所以存在，是因为有时我们可能想要动态地覆盖为简化的执行映射。
		// 当我们不覆盖时，我们使用默认的，这样代码就不必改变。
		ExecutionMap default_execution_map;

		// 堆分配的参数数据，当太大而无法放入 `operations` 时使用。
		// 我们持有对它们的引用，以便在程序被清除时释放。
		StdVector<HeapResource> heap_resources;

		// 堆分配的参数数据，当太大而无法放入 `operations` 时使用。
		// 我们持有对它们的引用，以便它们在程序被清除之前不会被释放。
		StdVector<Ref<RefCounted>> ref_resources;

		// 描述在程序可以运行之前需要在 `State` 中准备的缓冲区列表
		StdVector<BufferSpec> buffer_specs;

		// `operations` 中的地址，从此处开始的操作将不再仅依赖于标记为"外部
		// 组"的输入。它用于优化掉在平面地形使用情况下会相同的计算。
		uint32_t inner_group_start_op_index;

		StdVector<InputInfo> inputs;

		FixedArray<OutputInfo, MAX_OUTPUTS> outputs;
		unsigned int outputs_count = 0;

		// 此程序完成一次完整运行所需的缓冲区最大数量。
		// 缓冲区用于保存每个操作的参数和输出的值。
		unsigned int buffer_count = 0;
		// 此程序完成一次完整运行所需的缓冲区数据最大数量。
		unsigned int buffer_data_count = 0;

		// 将展开图中的端口与其在编译程序中的相应地址关联起来。
		// 这用于调试中间值。
		StdUnorderedMap<ProgramGraph::PortLocation, uint16_t> output_port_addresses;

		// 如果你有来自原始用户图的端口位置，在查询 `output_port_addresses` 之前，请先进行重映射，
		// 以防它在编译期间被展开为不同的节点。
		StdUnorderedMap<ProgramGraph::PortLocation, ProgramGraph::PortLocation> user_port_to_expanded_port;

		// 将展开图 ID 关联到用户图节点 ID。
		StdUnorderedMap<uint32_t, uint32_t> expanded_node_id_to_user_node_id;

		// 上次编译尝试的结果。如果失败，则不应运行该程序。
		CompilationResult compilation_result;

		void clear() {
			operations.clear();
			buffer_specs.clear();
			inner_group_start_op_index = 0;
			default_execution_map.clear();
			output_port_addresses.clear();
			user_port_to_expanded_port.clear();
			expanded_node_id_to_user_node_id.clear();
			dependency_graph.clear();
			inputs.clear();
			outputs_count = 0;
			compilation_result = CompilationResult();
			for (HeapResource &r : heap_resources) {
				r.free();
			}
			heap_resources.clear();
			ref_resources.clear();
			buffer_count = 0;
			buffer_data_count = 0;
		}
	};

	Program _program;
};

} // namespace voxel::pg

#endif // VOXEL_GRAPH_RUNTIME_H
