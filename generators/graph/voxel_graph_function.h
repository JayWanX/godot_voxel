#ifndef VOXEL_GRAPH_FUNCTION_H
#define VOXEL_GRAPH_FUNCTION_H

#include "../../engine/gpu/compute_shader_resource.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/resource.h"
#include "../../util/string/std_string.h"
#include "../../util/thread/mutex.h"
#include "program_graph.h"
#include "voxel_graph_runtime.h"
#include <memory>

namespace voxel::pg {

struct ShaderParameter {
	StdString name;
	std::shared_ptr<ComputeShaderResource> resource;
};

struct ShaderOutput {
	enum Type {
		TYPE_SDF,
		TYPE_SINGLE_TEXTURE,
		TYPE_TYPE,
	};
	Type type;
};

// 由操作节点构成的通用处理图。
// TODO 这个类不得不加上 `VoxelGraph` 前缀，但我更希望它就叫 `pg::Function`。
// 之所以如此，是因为 Godot 的类没有命名空间。
// 这个类正逐渐变得更通用，与体素的关联不大（但对处理数据很有用）。
class VoxelGraphFunction : public Resource {
	GDCLASS(VoxelGraphFunction, Resource)
public:
	static const char *SIGNAL_NODE_NAME_CHANGED;
	static const char *SIGNAL_COMPILED;

	// 数据库中的节点索引。
	// 不要将这些值用于保存的数据中，
	// 它们会随模块编译时启用的功能而变化。
	enum NodeTypeID {
		NODE_CONSTANT,
		NODE_INPUT_X,
		NODE_INPUT_Y,
		NODE_INPUT_Z,
		NODE_OUTPUT_SDF,
		NODE_ADD,
		NODE_SUBTRACT,
		NODE_MULTIPLY,
		NODE_DIVIDE,
		NODE_SIN,
		NODE_FLOOR,
		NODE_ABS,
		NODE_SQRT,
		NODE_FRACT,
		NODE_STEPIFY,
		NODE_WRAP,
		NODE_MIN,
		NODE_MAX,
		NODE_DISTANCE_2D,
		NODE_DISTANCE_3D,
		NODE_CLAMP,
		NODE_CLAMP_C,
		NODE_MIX,
		NODE_REMAP,
		NODE_SMOOTHSTEP,
		NODE_CURVE,
		NODE_SELECT,
		NODE_NOISE_2D,
		NODE_NOISE_3D,
		NODE_IMAGE_2D,
		NODE_SDF_PLANE,
		NODE_SDF_BOX,
		NODE_SDF_SPHERE,
		NODE_SDF_TORUS,
		NODE_SDF_PREVIEW, // 用于调试
		NODE_SDF_SPHERE_HEIGHTMAP,
		NODE_SDF_SMOOTH_UNION,
		NODE_SDF_SMOOTH_SUBTRACT,
		NODE_NORMALIZE_3D,
		NODE_FAST_NOISE_2D,
		NODE_FAST_NOISE_3D,
		NODE_FAST_NOISE_GRADIENT_2D,
		NODE_FAST_NOISE_GRADIENT_3D,
		NODE_OUTPUT_WEIGHT,
		NODE_OUTPUT_TYPE,
		NODE_OUTPUT_SINGLE_TEXTURE,
		NODE_EXPRESSION,
		NODE_POWI, // pow(x, 常量正整数)
		NODE_POW, // pow(x, y)
		NODE_INPUT_SDF,
		NODE_COMMENT,
		NODE_FUNCTION,
		NODE_CUSTOM_INPUT,
		NODE_CUSTOM_OUTPUT,
		NODE_RELAY,
		NODE_SPOTS_2D,
		NODE_SPOTS_3D,

	// 以下为可选功能（避免在构建两种版本时文档产生差异）
	// 请记住，此枚举的值不应在持久化上下文（存档）中使用

#ifdef VOXEL_ENABLE_FAST_NOISE_2
		NODE_FAST_NOISE_2_2D,
		NODE_FAST_NOISE_2_3D,
#endif

		NODE_TYPE_COUNT
	};

	struct Port {
		NodeTypeID type;
		// 用于可多次出现但索引不同的端口类型。
		// 最初用于 OutputWeight 节点。
		unsigned int sub_index = 0;
		// 端口名称。如果是自定义端口，它用于标识该端口（否则无关紧要）。
		String name;

		Port() {}
		Port(NodeTypeID p_type, const String &p_name) : type(p_type), name(p_name) {}

		inline bool is_custom() const {
			return type == NODE_CUSTOM_INPUT || type == NODE_CUSTOM_OUTPUT;
		}

		inline bool equals(const Port &other) const {
			if (is_custom()) {
				return name == other.name;
			} else {
				return type == other.type && sub_index == other.sub_index;
			}
		}
	};

	void clear();

	// 图的编辑 API
	// 重要：编辑图的函数不是线程安全的。
	// 它们预期由主线程（编辑器或游戏逻辑）使用。

	uint32_t create_node(NodeTypeID type_id, Vector2 position = Vector2(), uint32_t id = ProgramGraph::NULL_ID);
	void remove_node(uint32_t node_id);

	uint32_t create_function_node(
			Ref<VoxelGraphFunction> func,
			Vector2 position = Vector2(),
			uint32_t p_id = ProgramGraph::NULL_ID
	);

	// 检查是否可以创建指定的连接
	bool can_connect(
			uint32_t src_node_id,
			uint32_t src_port_index,
			uint32_t dst_node_id,
			uint32_t dst_port_index
	) const;

	// 检查指定的连接是否有效（不考虑现有连接）
	bool is_valid_connection(
			uint32_t src_node_id,
			uint32_t src_port_index,
			uint32_t dst_node_id,
			uint32_t dst_port_index
	) const;

	void add_connection(uint32_t src_node_id, uint32_t src_port_index, uint32_t dst_node_id, uint32_t dst_port_index);
	void remove_connection(
			uint32_t src_node_id,
			uint32_t src_port_index,
			uint32_t dst_node_id,
			uint32_t dst_port_index
	);
	void get_connections(StdVector<ProgramGraph::Connection> &p_connections) const;

	// 找出连接到给定目标端口的是哪个源端口。
	// 如果 `dst` 没有入站连接，则返回 false。
	bool try_get_connection_to(ProgramGraph::PortLocation dst, ProgramGraph::PortLocation &out_src) const;

	bool has_node(uint32_t node_id) const;

	void set_node_name(uint32_t node_id, StringName p_name);
	StringName get_node_name(uint32_t node_id) const;
	uint32_t find_node_by_name(StringName p_name) const;

	Variant get_node_param(uint32_t node_id, int param_index) const;
	void set_node_param(uint32_t node_id, int param_index, Variant value);
	void set_node_param_by_name(const uint32_t node_id, const String &param_name, const Variant &value);
	void set_node_param_unchecked(ProgramGraph::Node &node, const int param_index, const Variant &value);

	static bool get_expression_variables(std::string_view code, StdVector<std::string_view> &vars);
	void get_expression_node_inputs(uint32_t node_id, StdVector<StdString> &out_names) const;
	void set_expression_node_inputs(uint32_t node_id, PackedStringArray input_names);

	int get_node_input_index(const uint32_t node_id, String input_name) const;
	int get_node_output_index(const uint32_t node_id, const String output_name) const;
	Variant get_node_default_input(uint32_t node_id, int input_index) const;
	void set_node_default_input(uint32_t node_id, int input_index, Variant value);
	void set_node_default_input_by_name(const uint32_t node_id, const String &input_name, const Variant &value);

	bool get_node_default_inputs_autoconnect(uint32_t node_id) const;
	void set_node_default_inputs_autoconnect(uint32_t node_id, bool enabled);

	Vector2 get_node_gui_position(uint32_t node_id) const;
	void set_node_gui_position(uint32_t node_id, Vector2 pos);

	Vector2 get_node_gui_size(uint32_t node_id) const;
	void set_node_gui_size(uint32_t node_id, Vector2 size);

	NodeTypeID get_node_type_id(uint32_t node_id) const;
	PackedInt32Array get_node_ids() const;
	uint32_t generate_node_id() {
		return _graph.generate_node_id();
	}

	unsigned int get_nodes_count() const;

	// 编辑器

#ifdef TOOLS_ENABLED
	void get_configuration_warnings(PackedStringArray &out_warnings) const;

	// 获取一个哈希，该哈希仅在图的输出不同时才会改变。
	// 它是根据可编辑的图数据计算的，而非编译结果。
	// 注意：在比较两个图时并不保证有效。它最初是为检测变化而设计的。
	uint64_t get_output_graph_hash() const;

	bool can_load_default_graph() const {
		return _can_load_default_graph;
	}
#endif

	// 内部

	const ProgramGraph &get_graph() const;
	void find_dependencies(uint32_t node_id, StdVector<uint32_t> &out_dependencies) const;
	Dictionary get_graph_as_variant_data() const;
	bool load_graph_from_variant_data(Dictionary data);

	unsigned int get_node_input_count(uint32_t node_id) const;
	unsigned int get_node_output_count(uint32_t node_id) const;

	// TODO 这应该是直接作为一个节点类型吗？
	enum AutoConnect { //
		AUTO_CONNECT_NONE,
		AUTO_CONNECT_X,
		AUTO_CONNECT_Y,
		AUTO_CONNECT_Z
	};

	static bool try_get_node_type_id_from_auto_connect(AutoConnect ac, NodeTypeID &out_node_type);
	static bool try_get_auto_connect_from_node_type_id(NodeTypeID node_type, AutoConnect &out_ac);

	void get_node_input_info(
			uint32_t node_id,
			unsigned int input_index,
			String *out_name,
			AutoConnect *out_autoconnect
	) const;
	String get_node_output_name(uint32_t node_id, unsigned int output_index) const;
	Span<const Port> get_input_definitions() const;
	Span<const Port> get_output_definitions() const;
	// 目前仅用于测试
	void set_io_definitions(Span<const Port> inputs, Span<const Port> outputs);
	bool contains_reference_to_function(Ref<VoxelGraphFunction> p_func, int max_recursion = 16) const;
	bool contains_reference_to_function(const VoxelGraphFunction &p_func, int max_recursion = 16) const;
	void auto_pick_inputs_and_outputs();

	bool is_automatic_io_setup_enabled() const;

	bool get_node_input_index_by_name(uint32_t node_id, String input_name, unsigned int &out_input_index) const;
	bool get_node_param_index_by_name(uint32_t node_id, String param_name, unsigned int &out_param_index) const;

	void update_function_nodes(StdVector<ProgramGraph::Connection> *removed_connections);

	// 将节点及其相互之间的连接复制到另一个图中。
	// 节点参数中的资源若没有文件路径则会被复制。
	// 如果提供了非空大小的 `dst_node_ids`，则用于定义复制节点的 ID；否则将自动生成。
	void duplicate_subgraph(
			Span<const uint32_t> src_node_ids,
			Span<const uint32_t> dst_node_ids,
			VoxelGraphFunction &dst_graph,
			Vector2 gui_offset
	) const;

	void paste_graph(const VoxelGraphFunction &src_graph, Span<const uint32_t> dst_node_ids, Vector2 gui_offset);

	struct ShaderResult {
		StdString code_utf8;
		StdVector<pg::ShaderParameter> params;
		StdVector<pg::ShaderOutput> outputs;
		CompilationResult compilation;
	};

	ShaderResult get_shader_source() const;

	// 编译与运行

	pg::CompilationResult compile(bool debug);
	void execute(
			Span<const Span<const float>> inputs,
			Span<Span<float>> outputs,
			const unsigned int max_processing_chunk_size = 256,
			const bool dummy_output = false
	);

	bool is_compiled() const;

	struct CompiledGraph {
		// 编译完成后即为只读！多个线程可以同时读取它。
		// 要重新编译图，需要创建新实例。
		pg::Runtime runtime;
	};

	std::shared_ptr<CompiledGraph> get_compiled_graph() const;

	// 用于运行时执行的每线程复用内存
	struct RuntimeCache {
		pg::Runtime::State state;
		StdVector<Span<const float>> input_chunks;
		pg::Runtime::ExecutionMap optimized_execution_map;
	};

	static RuntimeCache &get_runtime_cache_tls();

	CompilationResult expand_and_reduce();

	// 测试两个图是否相同，考虑其输出分支。对象参数先按引用比较，再按属性比较。
	// 输出不依赖的节点将被忽略。
	bool equals(const VoxelGraphFunction &other);

	void debug_analyze_range(Span<const math::Interval> input_ranges, const bool optimize_execution_map);

private:
	void register_subresource(Resource &resource);
	void unregister_subresource(Resource &resource);
	void register_subresources();
	void unregister_subresources();
	void _on_subresource_changed();

	int _b_get_node_type_count() const;
	Dictionary _b_get_node_type_info(int type_id) const;
	Array _b_get_connections() const;
	// TODO 存在的原因仅仅是 UndoRedo API 将 `null` 与 `缺少参数` 混淆了...
	// 参见 https://github.com/godotengine/godot/issues/36895
	void _b_set_node_param_null(int node_id, int param_index);
	void _b_set_node_name(int node_id, String node_name);

	Array _b_get_input_definitions() const;
	void _b_set_input_definitions(Array data);

	Array _b_get_output_definitions() const;
	void _b_set_output_definitions(Array data);

	void _b_paste_graph_with_pre_generated_ids(
			Ref<VoxelGraphFunction> graph,
			PackedInt32Array dst_node_ids,
			Vector2 gui_offset
	);

	static void _bind_methods();

	ProgramGraph _graph;
	// 若启用，编译时会根据图的节点自动设置输入和输出。
	// 但这无法精细控制 I/O 出现的顺序，如果需要可将其禁用。
	bool _automatic_io_setup_enabled = true;
	StdVector<Port> _inputs;
	StdVector<Port> _outputs;
	StdVector<ObjectID> _subresources; // 可能包含重复项
#ifdef TOOLS_ENABLED
	// Godot 无法区分在检查器中新建的资源、已存在的空资源或脚本创建的资源...
	// 为了在编辑器中新建图时能加载一个"hello world"图，必须知道这一点。
	// 创建后默认为 true，但如果被清空（意味着它不是全新的实例）则变为 false。
	bool _can_load_default_graph = true;
#endif
	pg::CompilationResult _last_compiling_result;

	// 已编译部分。
	// 赋值后绝不能修改。
	// 它可能被多个线程访问，因此需要保护。
	std::shared_ptr<CompiledGraph> _compiled_graph = nullptr;
	Mutex _compiled_graph_mutex;
};

ProgramGraph::Node *create_node_internal(
		ProgramGraph &graph,
		VoxelGraphFunction::NodeTypeID type_id,
		Vector2 position,
		uint32_t id,
		bool create_default_instances
);

void auto_pick_inputs_and_outputs(
		const ProgramGraph &graph,
		StdVector<VoxelGraphFunction::Port> &inputs,
		StdVector<VoxelGraphFunction::Port> &outputs
);

Array serialize_io_definitions(Span<const VoxelGraphFunction::Port> ports);

// 将节点复制到目标图中（可以是不同的图）。不复制连接。
ProgramGraph::Node *duplicate_node(
		ProgramGraph &dst_graph,
		const ProgramGraph::Node &src_node,
		bool duplicate_resources,
		uint32_t id = ProgramGraph::NULL_ID
);

#ifdef TOOLS_ENABLED

inline String get_port_display_name(const VoxelGraphFunction::Port &port) {
	return port.name.is_empty() ? String("<unnamed>") : port.name;
}

#endif

} // namespace voxel::pg

VARIANT_ENUM_CAST(voxel::pg::VoxelGraphFunction::NodeTypeID)

#endif // VOXEL_GRAPH_FUNCTION_H
