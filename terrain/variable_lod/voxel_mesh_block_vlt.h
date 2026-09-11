#ifndef VOXEL_MESH_BLOCK_VLT_H
#define VOXEL_MESH_BLOCK_VLT_H

#include <scene/resources/material.h>
#include "../../util/memory/memory.h"
#include "../../util/tasks/time_spread_task_runner.h"
#include "../voxel_mesh_block.h"

namespace voxel {

// 为 `VoxelTerrain` 的一个数据块存储网格和碰撞体。
// 它不存储体素数据，因为可能使用不同的数据块大小或不同的数据结构。
class VoxelMeshBlockVLT : public VoxelMeshBlock {
public:
	enum FadingState { //
		FADING_NONE,
		FADING_IN,
		FADING_OUT
	};

	FadingState fading_state = FADING_NONE;
	// 1.f 表示完全不透明，0.f 表示完全透明
	float fading_progress = 0.f;
	// 体素 LOD 通过将一个数据块分裂为最多 8 个更高分辨率的数据块来工作。
	// 父数据块及其子数据块可称为“LOD 组”。
	// 一个 LOD 组中只有不重叠的数据块可以同时处于激活状态。
	// 因此当使用 LOD 淡入淡出时，我们不再用 `visible` 来判断哪个数据块处于激活状态，
	// 因为数据块可以使用交叉淡入淡出效果。同一 LOD 组中重叠的数据块可以同时可见。
	// 因此需要使用此布尔值。
	bool visual_active = false;

	// bool got_first_mesh_update = false;

	// 0 表示不使用回退纹理。
	// 1 表示使用父级 LOD（lod_index+1）的纹理。
	// 2 表示使用祖父级 LOD（lod_index+2）的纹理，依此类推。
	uint8_t detail_texture_fallback_level = 0;

	uint64_t last_collider_update_time = 0;
	UniquePtr<VoxelMesher::Output> deferred_collider_data;

	int32_t col_vertex_end = -1;
	int32_t col_index_end = -1;

	VoxelMeshBlockVLT(const Vector3i bpos, unsigned int size, unsigned int p_lod_index);
	~VoxelMeshBlockVLT();

	// 设置同时用于碰撞和视觉的世界
	void set_world(Ref<World3D> p_world);

	// 视觉

	void set_visible(bool visible);
	bool update_fading(float speed);
	void clear_fading();

	void set_parent_visible(bool parent_visible);

	void set_mesh(
			Ref<Mesh> mesh,
			GeometryInstance3D::GIMode gi_mode,
			RenderingServerEnums::ShadowCastingSetting shadow_casting,
			int render_layers_mask,
			Ref<Mesh> shadow_occluder_mesh,
			int32_t p_col_vertex_max,
			int32_t p_col_index_max
#ifdef TOOLS_ENABLED
			,
			RenderingServerEnums::ShadowCastingSetting shadow_occluder_mode
#endif
	);
	void drop_visuals();

	void set_transition_mask(uint8_t m);
	inline uint8_t get_transition_mask() const {
		return _transition_mask;
	}

	void set_gi_mode(GeometryInstance3D::GIMode mode);
	void set_shadow_casting(RenderingServerEnums::ShadowCastingSetting mode);
	void set_render_layers_mask(int mask);

	void set_transition_mesh(
			Ref<Mesh> mesh,
			unsigned int side,
			GeometryInstance3D::GIMode gi_mode,
			RenderingServerEnums::ShadowCastingSetting shadow_casting,
			int render_layers_mask
	);

	void set_shader_material(Ref<ShaderMaterial> material);
	inline Ref<ShaderMaterial> get_shader_material() const {
		return _shader_material;
	}

	// 仅当地形上的材质覆盖不是 ShaderMaterial 时使用
	void set_material_override(Ref<Material> material);

	// 变换

	void set_parent_transform(const Transform3D &parent_transform);
	void update_transition_mesh_transform(unsigned int side, const Transform3D &parent_transform);

	template <typename F>
	void for_each_mesh_instance_with_transform(F f) const {
		const Transform3D local_transform(Basis(), _position_in_voxels);
		const Transform3D world_transform = local_transform;
		f(_mesh_instance, world_transform);
		for (unsigned int i = 0; i < _transition_mesh_instances.size(); ++i) {
			const voxel::godot::DirectMeshInstance &mi = _transition_mesh_instances[i];
			if (mi.is_valid()) {
				f(mi, world_transform);
			}
		}
	}

#ifdef TOOLS_ENABLED
	inline void set_shadow_occluder_mode(RenderingServerEnums::ShadowCastingSetting mode) {
		_shadow_occluder.set_cast_shadows_setting(mode);
	}
#endif

private:
	void set_material_override_internal(Ref<Material> material);
	void _set_visible(bool visible);

	inline bool _is_transition_visible(unsigned int side) const {
		return _transition_mask & (1 << side);
	}

	Ref<ShaderMaterial> _shader_material;

	FixedArray<voxel::godot::DirectMeshInstance, Cube::SIDE_COUNT> _transition_mesh_instances;

	uint8_t _transition_mask = 0;

	// 参见 VoxelMesherBlocky。
	// 遗憾的是这必须是一个完全独立的网格实例，因为 Godot 不支持按网格表面设置
	// `cast_shadow` 模式。这可能对性能产生影响。
	voxel::godot::DirectMeshInstance _shadow_occluder;

#ifdef VOXEL_DEBUG_LOD_MATERIALS
	Ref<Material> _debug_material;
	Ref<Material> _debug_transition_material;
#endif
};

bool is_mesh_empty(Span<const VoxelMesher::Output::Surface> surfaces);

Ref<ArrayMesh> build_mesh(
		Span<const VoxelMesher::Output::Surface> surfaces,
		Mesh::PrimitiveType primitive,
		int flags,
		Ref<Material> material
);

} // namespace voxel

#endif // VOXEL_MESH_BLOCK_VLT_H
