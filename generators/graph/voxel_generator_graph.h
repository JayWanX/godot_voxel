#ifndef VOXEL_GENERATOR_GRAPH_H
#define VOXEL_GENERATOR_GRAPH_H

#include "../../util/containers/fixed_array.h"
#include "../../util/containers/span.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/core/dictionary.h"
#include "../../util/macros.h"
#include "../../util/math/vector2.h"
#include "../../util/math/vector3.h"
#include "../../util/math/vector3f.h"
#include "../../util/math/vector3i.h"
#include "../../util/thread/rw_lock.h"
#include "../voxel_generator.h"
#include "program_graph.h"
#include "voxel_graph_function.h"
#include "voxel_graph_runtime.h"

#include <memory>

VOXEL_GODOT_FORWARD_DECLARE(class Image)

namespace voxel {

class VoxelBuffer;

// 使用内部的 VoxelGraphFunction 生成体素数据。
class VoxelGeneratorGraph : public VoxelGenerator {
	GDCLASS(VoxelGeneratorGraph, VoxelGenerator)
public:
	static const char *SIGNAL_NODE_NAME_CHANGED;

	// 选择纹理输出产生的格式
	enum TextureMode {
		// 体素包含 4 个索引和 4 个权重，编码在 16 位通道 INDICES 和 WEIGHTS 中
		TEXTURE_MODE_MIXEL4,
		// 体素包含 1 个索引，位于 8 位通道 INDICES 中
		TEXTURE_MODE_SINGLE,
		TEXTURE_MODE_COUNT
	};

	VoxelGeneratorGraph();
	~VoxelGeneratorGraph();

	void clear();
	void load_plane_preset();

	Ref<pg::VoxelGraphFunction> get_main_function() const;

	// 性能调优（高级）

	bool is_using_optimized_execution_map() const;
	void set_use_optimized_execution_map(bool use);

	float get_sdf_clip_threshold() const;
	void set_sdf_clip_threshold(float t);

	void set_use_subdivision(bool use);
	bool is_using_subdivision() const;

	void set_subdivision_size(int size);
	int get_subdivision_size() const;

	void set_debug_clipped_blocks(bool enabled);
	bool is_debug_clipped_blocks() const;

	void set_use_xz_caching(bool enabled);
	bool is_using_xz_caching() const;

	void set_texture_mode(const TextureMode mode);
	TextureMode get_texture_mode() const;

	// VoxelGenerator 实现

	int get_used_channels_mask() const override;

	Result generate_block(VoxelGenerator::VoxelQueryData input) override;
	bool generate_broad_block(VoxelGenerator::VoxelQueryData input) override;
	// float generate_single(const Vector3i &position);
	bool supports_single_generation() const override {
		return true;
	}
	bool supports_series_generation() const override {
		return true;
	}
	VoxelSingleValue generate_single(Vector3i position, unsigned int channel) override;

	void generate_series(
			Span<const float> positions_x,
			Span<const float> positions_y,
			Span<const float> positions_z,
			unsigned int channel,
			Span<float> out_values,
			Vector3f min_pos,
			Vector3f max_pos
	) override;

	// Ref<Resource> duplicate(bool p_subresources) const VOXEL_OVERRIDE_UNLESS_GODOT_EXTENSION;

	// 工具

	void bake_sphere_bumpmap(Ref<Image> im, float ref_radius, float min_height, float max_height);
	void bake_sphere_normalmap(Ref<Image> im, float ref_radius, float strength);

	float raycast_sdf_approx(const Vector3 ray_origin, const Vector3 ray_end, const float stride) const;

	void generate_image_from_sdf(Ref<Image> image, const Transform3D transform, const Vector2 size);

	// 内部

	pg::CompilationResult compile(bool debug);
	bool is_good() const;

	void generate_set(Span<const float> in_x, Span<const float> in_y, Span<const float> in_z);
	void generate_series(
			Span<const float> in_x,
			Span<const float> in_y,
			Span<const float> in_z,
			Span<const float> in_sdf
	);

	// 返回当前线程中最后一次使用的生成器的状态
	static const pg::Runtime::State &get_last_state_from_current_thread();
	static Span<const uint32_t> get_last_execution_map_debug_from_current_thread();

	bool try_get_output_port_address(ProgramGraph::PortLocation port, uint32_t &out_address) const;
	int get_sdf_output_port_address() const;

	bool has_texture_output() const;

#ifdef VOXEL_ENABLE_GPU
	// GPU 支持

	bool supports_shaders() const override {
		// 在某种程度上支持。如果图中包含不兼容的节点，可能会失败。
		return true;
	}

	bool get_shader_source(ShaderSourceData &out_data) const override;
#endif

	// 调试

	math::Interval debug_analyze_range(Vector3i min_pos, Vector3i max_pos, bool optimize_execution_map) const;

	struct NodeProfilingInfo {
		uint32_t node_id;
		uint32_t microseconds;
	};

	float debug_measure_microseconds_per_voxel(bool singular, StdVector<NodeProfilingInfo> *node_profiling_info);

	void debug_load_waves_preset();

	// 编辑器

#ifdef TOOLS_ENABLED
	void get_configuration_warnings(PackedStringArray &out_warnings) const override;
#endif

private:
	void _on_subresource_changed();
	float _b_generate_single(Vector3 pos);
	Vector2 _b_debug_analyze_range(Vector3 min_pos, Vector3 max_pos) const;
	Dictionary _b_compile();
	float _b_debug_measure_microseconds_per_voxel(bool singular);
#ifdef TOOLS_ENABLED
	// 这存在是因为某些自定义编辑器会编辑内部对象而不是资源本身（这里是“主函数”对象）。
	// 并且由于 Godot 根据 UndoRedo 判断资源是否应保存，如果包含该资源的资源没有出现在 UndoRedo 操作中，
	// 它就会认为资源没有改变而不保存…… 所以我们先调用一个空函数，只是为了让 Godot 明白这一点……
	void _b_dummy_function() {}
#endif
	Dictionary get_graph_as_variant_data() const;
	void load_graph_from_variant_data(Dictionary data);

	struct WeightOutput {
		unsigned int layer_index;
		unsigned int output_buffer_index;
	};

	static void gather_texturing_data_from_weight_outputs(
			Span<const WeightOutput> weight_outputs,
			const pg::Runtime::State &state,
			const Vector3i rmin,
			const Vector3i rmax,
			const int ry,
			VoxelBuffer &out_voxel_buffer,
			const FixedArray<uint8_t, 4> spare_indices,
			const TextureMode mode
	);

	static void _bind_methods();

	Ref<pg::VoxelGraphFunction> _main_function;

	// 该生成器使用图的节点执行范围分析。地形表面只会在 SDF 在块内穿过零时出现。
	// 对于每个生成的块，都会计算输出的估计范围。
	// 如果该范围超出此阈值（无论是负向还是正向），那么块将被赋予统一的值，要么是空气要么是实体，
	// 从而跳过所有体素的生成。
	// 设置较高的阈值会关闭该功能，提供一致的 SDF，但可能会严重影响性能。
	float _sdf_clip_threshold = 1.5f;
	// 有时块尺寸可能较大，但这会降低范围分析的精度。因此可以将块内区域的生成进行细分，
	// 而不是整体生成。
	// 块尺寸必须是细分尺寸的倍数。
	bool _use_subdivision = true;
	int _subdivision_size = 16;
	// 启用后，如果某些节点的输出范围被认为不影响最终结果，
	// 生成器将尝试优化掉在特定区域不需要运行的节点。
	bool _use_optimized_execution_map = true;
	// 启用后，当沿 Y 方向分片生成块时，仅使用 X 和 Z 坐标的节点将被缓存。
	// 这可以防止重新计算那些在每个分片上本应相同的值。
	// 例如，当图的某部分正在生成高度图时，这会带来很大帮助。
	bool _use_xz_caching = true;
	// 如果为 true，则反转被裁剪的块，使其产生视觉伪影，让被裁剪的区域可见。
	bool _debug_clipped_blocks = false;
	TextureMode _texture_mode = TEXTURE_MODE_MIXEL4;

	// 只有编译和生成方法是线程安全的。

	// 运行时封装，包含针对该用例特化的额外信息
	struct Runtime {
		// TODO 使用来自 `VoxelGraphFunction` 的运行时和状态
		pg::Runtime runtime;

		// 图中未使用的索引。
		// 当纹理权重输出少于 4 个时使用。
		FixedArray<uint8_t, 4> spare_texture_indices;

		int x_input_index = -1;
		int y_input_index = -1;
		int z_input_index = -1;
		int sdf_input_index = -1;

		int sdf_output_index = -1;
		int sdf_output_buffer_index = -1;

		int type_output_index = -1;
		int type_output_buffer_index = -1;

		int single_texture_output_index = -1;
		int single_texture_output_buffer_index = -1;

		FixedArray<WeightOutput, 16> weight_outputs;
		// 用于提供查询的索引列表。顺序无关紧要，可以与 `weight_outputs` 不同。
		FixedArray<unsigned int, 16> weight_output_indices;
		unsigned int weight_outputs_count = 0;
	};

	// 用于为运行时查询设置输入的辅助
	template <typename T>
	struct QueryInputs {
		FixedArray<T, 4> query_inputs;
		unsigned int input_count = 0;

		inline QueryInputs(const Runtime &runtime_wrapper, T x, T y, T z, T sdf) {
			if (runtime_wrapper.x_input_index != -1) {
				query_inputs[runtime_wrapper.x_input_index] = x;
			}
			if (runtime_wrapper.y_input_index != -1) {
				query_inputs[runtime_wrapper.y_input_index] = y;
			}
			if (runtime_wrapper.z_input_index != -1) {
				query_inputs[runtime_wrapper.z_input_index] = z;
			}
			if (runtime_wrapper.sdf_input_index != -1) {
				query_inputs[runtime_wrapper.sdf_input_index] = sdf;
			}
			input_count = runtime_wrapper.runtime.get_input_count();
		}

		inline Span<const T> get() {
			return to_span(query_inputs, input_count);
		}
	};

	std::shared_ptr<Runtime> _runtime = nullptr;
	RWLock _runtime_lock;

	struct Cache {
		StdVector<float> x_cache;
		StdVector<float> y_cache;
		StdVector<float> z_cache;
		StdVector<float> input_sdf_slice_cache;
		StdVector<float> input_sdf_full_cache;
		// TODO 使用来自 `VoxelGraphFunction` 的运行时和状态
		pg::Runtime::State state;
		pg::Runtime::ExecutionMap optimized_execution_map;
	};

	static Cache &get_tls_cache();
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelGeneratorGraph::TextureMode)

#endif // VOXEL_GENERATOR_GRAPH_H
