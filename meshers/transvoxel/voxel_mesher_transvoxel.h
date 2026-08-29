#ifndef VOXEL_MESHER_TRANSVOXEL_H
#define VOXEL_MESHER_TRANSVOXEL_H

#include "../../util/macros.h"
#include "../voxel_mesher.h"
#include "transvoxel.h"

VOXEL_GODOT_FORWARD_DECLARE(class ArrayMesh);
VOXEL_GODOT_FORWARD_DECLARE(class ShaderMaterial);

namespace voxel {

namespace godot {
class VoxelBuffer;
}

class VoxelMesherTransvoxel : public VoxelMesher {
	GDCLASS(VoxelMesherTransvoxel, VoxelMesher)

public:
	enum TexturingMode {
		TEXTURES_NONE = transvoxel::TEXTURES_NONE,
		TEXTURES_MIXEL4_S4 = transvoxel::TEXTURES_MIXEL4_S4,
		TEXTURES_SINGLE_S4 = transvoxel::TEXTURES_SINGLE_S4,
		TEXTURES_MODE_COUNT = transvoxel::TEXTURES_MODE_COUNT
	};

	static const int TEXTURES_BLEND_4_OVER_16 = 1;

	VoxelMesherTransvoxel();
	~VoxelMesherTransvoxel();

	void build(VoxelMesher::Output &output, const VoxelMesher::Input &input) override;
	Ref<ArrayMesh> build_transition_mesh(Ref<godot::VoxelBuffer> voxels, int direction);

	int get_used_channels_mask() const override;

	bool is_generating_collision_surface() const override;

	void set_texturing_mode(TexturingMode mode);
	TexturingMode get_texturing_mode() const;

	void set_textures_ignore_air_voxels(const bool enable);
	bool get_textures_ignore_air_voxels() const;

	void set_mesh_optimization_enabled(bool enabled);
	bool is_mesh_optimization_enabled() const;

	void set_mesh_optimization_error_threshold(float threshold);
	float get_mesh_optimization_error_threshold() const;

	void set_mesh_optimization_target_ratio(float ratio);
	float get_mesh_optimization_target_ratio() const;

	void set_transitions_enabled(bool enable);
	bool get_transitions_enabled() const;

	void set_edge_clamp_margin(float margin);
	float get_edge_clamp_margin() const;

	Ref<ShaderMaterial> get_default_lod_material() const override;

	// 内部

	static void load_static_resources();
	static void free_static_resources();

	// 为快速路径暴露。返回值仅在调用线程下一次调用 build() 之前有效。
	static const transvoxel::MeshArrays &get_mesh_cache_from_current_thread();
	// 为快速路径暴露。只有当输入给 `build` 的 `detail_texture_hint` 为 true 时返回值才有效，
	// 并且仅在调用线程下一次调用 build() 之前保持有效。
	static Span<const transvoxel::CellInfo> get_cell_info_from_current_thread();

	// 不确定是否有必要，目前过渡网格要么合并到主网格，要么不生成
	// enum TransitionMode {
	// 	// 不会生成过渡网格
	// 	TRANSITION_NONE,
	// 	// 生成独立的过渡网格
	// 	TRANSITION_SEPARATE,
	// 	// 过渡网格将是主网格的一部分
	// 	TRANSITION_COMBINED
	// };

protected:
	static void _bind_methods();

private:
	TexturingMode _texture_mode = TEXTURES_NONE;

	struct MeshOptimizationParams {
		bool enabled = false;
		float error_threshold = 0.005;
		float target_ratio = 0.0;
	};

	MeshOptimizationParams _mesh_optimization_params;

	// 计算行进立方体单元时，顶点可能位于单元的边上的任意位置，包括非常接近
	// 角点的位置。这可能导致非常薄或非常小的三角形，尤其是在碰撞方面会成为问题。该
	// 边距是从角点的最小距离，低于该距离的顶点会被钳制到此值。增大此值
	// 会降低网格质量。
	float _edge_clamp_margin = 0.02f;

	bool _transitions_enabled = true;

	bool _textures_ignore_air_voxels = false;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelMesherTransvoxel::TexturingMode);

#endif // VOXEL_MESHER_TRANSVOXEL_H
