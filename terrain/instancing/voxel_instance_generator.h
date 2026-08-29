#ifndef VOXEL_INSTANCE_GENERATOR_H
#define VOXEL_INSTANCE_GENERATOR_H

// #include "../../storage/voxel_buffer.h"
#include "../../generators/graph/voxel_graph_function.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/noise.h"
#include "../../util/math/transform3f.h"
#include "../../util/math/vector3i.h"
#include "../../util/thread/short_lock.h"
#include "up_mode.h"

#include <limits>

namespace voxel {

class VoxelGenerator;

// TODO 有一天这可能不得不移到网格化线程中

// 决定在体素表面上的哪个位置生成实例。
// 注意：若要生成体素本身，请参阅 `VoxelGenerator`。
class VoxelInstanceGenerator : public Resource {
	GDCLASS(VoxelInstanceGenerator, Resource)
public:
	enum EmitMode {
		// 最快，但在使用高密度或简化网格时可能出现明显图案
		EMIT_FROM_VERTICES,
		// 较慢，但图案应不那么明显。假设所有三角形面积相近，
		// 使用移动立方体获得的非简化网格正是这种情况。
		EMIT_FROM_FACES_FAST,
		// 较慢，但尝试不假设三角形面积。
		EMIT_FROM_FACES,
		EMIT_ONE_PER_TRIANGLE,

		EMIT_MODE_COUNT
	};

	enum Distribution {
		DISTRIBUTION_LINEAR = 0,
		DISTRIBUTION_QUADRATIC,
		DISTRIBUTION_CUBIC,
		DISTRIBUTION_QUINTIC,
		DISTRIBUTION_COUNT
	};

	enum Dimension { //
		DIMENSION_2D = 0,
		DIMENSION_3D,
		DIMENSION_COUNT
	};

	// 此 API 可能会更改，因此目前不向脚本公开。
	// 使用 32 位浮点变换，因为这些变换被分块，其原点实际上不需要保存
	// 大坐标。
	void generate_transforms(
			StdVector<Transform3f> &out_transforms,
			const Vector3i grid_position,
			const int lod_index,
			const int layer_id,
			Array surface_arrays,
			// 如果非负，则忽略此索引及之后的顶点
			const int32_t vertex_range_end,
			// 如果非负，则忽略此索引及之后的索引
			const int32_t index_range_end,
			const UpMode up_mode,
			// 生成 2x2x2 数据块区域时，`octant_mask` 中的位指示应生成哪个卦限。
			// 置零的位将导致对应卦限中的所有实例不被生成。
			const uint8_t octant_mask,
			// 这是世界空间中的数据块大小，与 LOD 索引无关
			const float block_size,
			Ref<VoxelGenerator> voxel_generator
	);

	void set_density(float d);
	float get_density() const;

	void set_emit_mode(EmitMode mode);
	EmitMode get_emit_mode() const;

	void set_jitter(const float p_jitter);
	float get_jitter() const;

	void set_triangle_area_threshold(const float p_threshold);
	float get_triangle_area_threshold() const;

	void set_vertical_alignment(float valign);
	float get_vertical_alignment() const;

	void set_min_scale(float min_scale);
	float get_min_scale() const;

	void set_max_scale(float max_scale);
	float get_max_scale() const;

	void set_scale_distribution(Distribution distribution);
	Distribution get_scale_distribution() const;

	// TODO 添加缩放曲线，现实生活中小物件远多于大物件

	void set_offset_along_normal(float offset);
	float get_offset_along_normal() const;

	void set_min_slope_degrees(float p_degrees);
	float get_min_slope_degrees() const;

	void set_max_slope_degrees(float p_degrees);
	float get_max_slope_degrees() const;

	void set_min_slope_falloff_degrees(float p_degrees);
	float get_min_slope_falloff_degrees() const;

	void set_max_slope_falloff_degrees(float p_degrees);
	float get_max_slope_falloff_degrees() const;

	void set_min_height(float h);
	float get_min_height() const;

	void set_max_height(float h);
	float get_max_height() const;

	void set_min_height_falloff(float f);
	float get_min_height_falloff() const;

	void set_max_height_falloff(float f);
	float get_max_height_falloff() const;

	void set_random_vertical_flip(bool flip);
	bool get_random_vertical_flip() const;

	void set_random_rotation(bool enabled);
	bool get_random_rotation() const;

	void set_noise(Ref<Noise> noise);
	Ref<Noise> get_noise() const;

	void set_noise_graph(Ref<pg::VoxelGraphFunction> func);
	Ref<pg::VoxelGraphFunction> get_noise_graph() const;

	void set_noise_dimension(Dimension dim);
	Dimension get_noise_dimension() const;

	void set_noise_falloff(const float p_falloff);
	float get_noise_falloff() const;

	void set_noise_threshold(const float threshold);
	float get_noise_threshold() const;

	void set_noise_on_scale(float amount);
	float get_noise_on_scale() const;

	void set_voxel_material_filter_enabled(bool enabled);
	bool is_voxel_material_filter_enabled() const;

	void set_voxel_material_filter_mask(const uint32_t mask);
	uint32_t get_voxel_material_filter_mask() const;

	void set_voxel_material_filter_threshold(const float p_threshold);
	float get_voxel_material_filter_threshold() const;

	void set_snap_to_generator_sdf_enabled(bool enabled);
	bool get_snap_to_generator_sdf_enabled() const;

	void set_snap_to_generator_sdf_search_distance(float new_distance);
	float get_snap_to_generator_sdf_search_distance() const;

	void set_snap_to_generator_sdf_sample_count(int new_sample_count);
	int get_snap_to_generator_sdf_sample_count() const;

	static inline int get_octant_index(const Vector3f pos, float half_block_size) {
		return get_octant_index(pos.x > half_block_size, pos.y > half_block_size, pos.z > half_block_size);
	}

	static inline int get_octant_index(bool x, bool y, bool z) {
		return int(x) | (int(y) << 1) | (int(z) << 2);
	}

#ifdef TOOLS_ENABLED
	void get_configuration_warnings(PackedStringArray &warnings) const;
	void _validate_property(PropertyInfo &p_property) const;
#endif

private:
	void _on_noise_changed();
	void _on_noise_graph_changed();

	PackedInt32Array _b_get_voxel_material_filter_array() const;
	void _b_set_voxel_material_filter_array(PackedInt32Array material_indices);

	static void _bind_methods();

	float _density = 0.1f;
	float _jitter = 1.f;
	float _triangle_area_threshold_lod0 = 0.f;
	float _vertical_alignment = 1.f;
	float _min_scale = 1.f;
	float _max_scale = 1.f;
	float _offset_along_normal = 0.f;

	float _min_height = std::numeric_limits<float>::min();
	float _max_height = std::numeric_limits<float>::max();
	float _min_height_falloff = 0.0f;
	float _max_height_falloff = 0.0f;

	float _min_slope_degrees = 0.f;
	float _max_slope_degrees = 180.f;
	float _min_slope_falloff_degrees = 0.f;
	float _max_slope_falloff_degrees = 0.f;

	bool _random_vertical_flip = false;
	bool _random_rotation = true;
	EmitMode _emit_mode = EMIT_FROM_VERTICES;
	Distribution _scale_distribution = DISTRIBUTION_QUADRATIC;
	Ref<Noise> _noise;
	Dimension _noise_dimension = DIMENSION_3D;
	float _noise_on_scale = 0.f;
	bool _voxel_material_filter_enabled = false;
	uint32_t _voxel_material_filter_mask = 1;
	float _voxel_material_filter_threshold = 0.5f;

	struct GeneratorSDFSnapSettings {
		bool enabled = false;
		uint8_t sample_count = 2;
		float search_distance = 1.f;
	};
	GeneratorSDFSnapSettings _gen_sdf_snap_settings;

	// TODO 保护噪声和噪声图成员免受多线程访问

	// 必需输入：
	// - X
	// - Y
	// - Z
	// 可能的输出：
	// - Density（密度）
	Ref<pg::VoxelGraphFunction> _noise_graph;
	// TODO 采样模式：
	// - 逐顶点：如果许多高密度项目需要它，则推荐（将在它们之间共享）
	// - 逐实例：如果低密度项目需要它，则推荐

	float _noise_falloff = 0.f;
	float _noise_threshold = 0.f;

	// 访问指针设置时使用，因为编辑器线程可能修改设置的同时，该生成器可能在另一个线程中使用。
	mutable ShortLock _ptr_settings_lock;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelInstanceGenerator::EmitMode);
VARIANT_ENUM_CAST(voxel::VoxelInstanceGenerator::Distribution);
VARIANT_ENUM_CAST(voxel::VoxelInstanceGenerator::Dimension);

#endif // VOXEL_INSTANCE_GENERATOR_H
