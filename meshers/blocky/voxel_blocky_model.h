#ifndef VOXEL_BLOCKY_MODEL_H
#define VOXEL_BLOCKY_MODEL_H

#include "../../constants/cube_tables.h"
#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/material.h"
#include "../../util/godot/classes/mesh.h"
#include "../../util/macros.h"
#include "../../util/math/ortho_basis.h"
#include "../../util/math/vector2f.h"
#include "../../util/math/vector3f.h"
#include "blocky_baked_library.h"

namespace voxel {

namespace blocky {
struct ModelBakingContext;
}

// TODO 在检视器中添加显示碰撞盒的预览

// 与特定体素值/状态对应的视觉效果和碰撞，供 `VoxelMesherBlocky` 使用。
// 体素可以是简单的彩色立方体，也可以是更复杂的模型。
class VoxelBlockyModel : public Resource {
	GDCLASS(VoxelBlockyModel, Resource)

public:
	// 约定为"无"。
	// 不要在该索引处分配非空模型。
	static const uint16_t AIR_ID = 0;
	static const uint8_t NULL_FLUID_INDEX = 255;
	static constexpr uint32_t MAX_SURFACES = 2;

	VoxelBlockyModel();

	enum Side {
		SIDE_NEGATIVE_X = Cube::SIDE_NEGATIVE_X,
		SIDE_POSITIVE_X = Cube::SIDE_POSITIVE_X,
		SIDE_NEGATIVE_Y = Cube::SIDE_NEGATIVE_Y,
		SIDE_POSITIVE_Y = Cube::SIDE_POSITIVE_Y,
		SIDE_NEGATIVE_Z = Cube::SIDE_NEGATIVE_Z,
		SIDE_POSITIVE_Z = Cube::SIDE_POSITIVE_Z,
		SIDE_COUNT = Cube::SIDE_COUNT
	};

	// 属性

	// 设置模型颜色
	void set_color(Color color);
	_FORCE_INLINE_ Color get_color() const {
		return _color;
	}

	// 设置或获取指定表面的材质覆盖，并查询是否存在覆盖
	void set_material_override(int index, Ref<Material> material);
	Ref<Material> get_material_override(int index) const;
	bool has_material_override() const;

	// 指定表面是否启用网格碰撞
	void set_mesh_collision_enabled(int surface_index, bool enabled);
	bool is_mesh_collision_enabled(int surface_index) const;

	// 透明度索引，用于相邻体素的面剔除判定
	void set_transparency_index(int i);
	int get_transparency_index() const {
		return _transparency_index;
	}

	// 是否剔除相邻体素的面
	void set_culls_neighbors(bool cn);
	bool get_culls_neighbors() const {
		return _culls_neighbors;
	}

	// 碰撞掩码
	void set_collision_mask(uint32_t mask);
	inline uint32_t get_collision_mask() const {
		return _collision_mask;
	}

	// 碰撞包围盒的数量
	unsigned int get_collision_aabb_count() const;
	void set_collision_aabb(unsigned int i, AABB aabb);
	void set_collision_aabbs(Span<const AABB> aabbs);

	// 是否参与随机刻更新
	void set_random_tickable(bool rt);
	bool is_random_tickable() const;

	// 标签掩码，用于与其它系统交互
	void set_tags_mask(const uint32_t mask);
	uint32_t get_tags_mask() const;

#ifdef TOOLS_ENABLED
	virtual void get_configuration_warnings(PackedStringArray &out_warnings) const;
#endif

	// 网格的正交旋转索引
	void set_mesh_ortho_rotation_index(int i);
	int get_mesh_ortho_rotation_index() const;

	// 是否生成 LOD 裙边
	void set_lod_skirts_enabled(bool rt);
	bool get_lod_skirts_enabled() const;

	//------------------------------------------
	// 仅供内部使用的属性

	// 该模型是否为空
	virtual bool is_empty() const;

	// 烘焙模型到上下文
	virtual void bake(blocky::ModelBakingContext &ctx) const;

	Span<const AABB> get_collision_aabbs() const {
		return to_span(_collision_aabbs);
	}
	const StdVector<AABB> &get_collision_aabbs_v() const {
		return _collision_aabbs;
	}

	struct LegacyProperties {
		enum GeometryType { GEOMETRY_NONE, GEOMETRY_CUBE, GEOMETRY_MESH };

		bool found = false;
		FixedArray<Vector2f, Cube::SIDE_COUNT> cube_tiles;
		GeometryType geometry_type = GEOMETRY_NONE;
		StringName name;
		int id = -1;
		Ref<Mesh> custom_mesh;
	};

	const LegacyProperties &get_legacy_properties() const {
		return _legacy_properties;
	}

	// 从其它模型复制基础属性
	void copy_base_properties_from(const VoxelBlockyModel &src);

	// 获取编辑器预览网格
	virtual Ref<Mesh> get_preview_mesh() const;

	// 将模型旋转 90 度
	void rotate_90(math::Axis axis, bool clockwise);
	void rotate_ortho(math::OrthoBasis ortho_basis);

	// 从烘焙数据生成网格
	static Ref<Mesh> make_mesh_from_baked_data(const blocky::BakedModel &baked_data, bool tangents_enabled);

	static Ref<Mesh> make_mesh_from_baked_data(
			Span<const blocky::BakedModel::Surface> inner_surfaces,
			Span<const FixedArray<blocky::BakedModel::SideSurface, MAX_SURFACES>> sides_surfaces,
			const Color model_color,
			const bool tangents_enabled
	);

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	void set_surface_count(unsigned int new_count);

	void rotate_collision_boxes_90(math::Axis axis, bool clockwise);
	void rotate_collision_boxes_ortho(math::OrthoBasis ortho_basis);

private:
	static void _bind_methods();

	TypedArray<AABB> _b_get_collision_aabbs() const;
	void _b_set_collision_aabbs(TypedArray<AABB> array);
	void _b_rotate_90(Vector3i::Axis axis, bool clockwise);

	// 属性

	struct SurfaceParams {
		// 如果已赋值，这些材质会覆盖网格本身自带的材质。
		Ref<Material> material_override;
		// 如果为 true 且启用了经典网格物理，该表面将出现在碰撞器中。
		bool collision_enabled = true;
	};

	FixedArray<SurfaceParams, MAX_SURFACES> _surface_params;

protected:
	unsigned int _surface_count = 0;

	// 仅用于 AABB 物理，不用于经典物理
	StdVector<AABB> _collision_aabbs;
	uint32_t _collision_mask = 1;

private:
	// 如果两个相邻体素本应遮挡它们共享的面，
	// 该索引决定是否遮挡。索引相同则剔除该面，不同则不剔除。
	uint8_t _transparency_index = 0;
	// 若启用，该体素会剔除其邻居的面。禁用该选项
	// 对较密集的透明体素（如树叶）很有用。
	bool _culls_neighbors = true;
	bool _random_tickable = false;
	uint32_t _tags_mask = 1;
	uint8_t _mesh_ortho_rotation = 0;

	bool _lod_skirts = true;

	Color _color;

	LegacyProperties _legacy_properties;
};

inline bool is_empty(const FixedArray<blocky::BakedModel::SideSurface, blocky::MAX_SURFACES> &surfaces) {
	for (const blocky::BakedModel::SideSurface &surface : surfaces) {
		if (surface.indices.size() > 0) {
			return false;
		}
	}
	return true;
}

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelBlockyModel::Side)

#endif // VOXEL_BLOCKY_MODEL_H
