#include <scene/resources/packed_scene.h>
#include <core/object/script_language.h>
#ifndef VOXEL_INSTANCE_LIBRARY_MULTIMESH_ITEM_H
#define VOXEL_INSTANCE_LIBRARY_MULTIMESH_ITEM_H

#include "../../util/containers/span.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/geometry_instance_3d.h"
#include "../../util/godot/classes/material.h"
#include "../../util/godot/classes/mesh.h"
#include <scene/resources/packed_scene.h>
#include "../../util/godot/classes/rendering_server.h"
#include "../../util/godot/classes/shape_3d.h"
#include <core/object/gdvirtual.gen.h>
#include "voxel_instance_library_item.h"

// 因为 GDVIRTUAL 我不得不包含它，否则会报窄化转换警告
#include "voxel_instancer.h"

namespace voxel {

class VoxelInstancer;

struct CollisionShapeInfo {
	Transform3D transform;
	Ref<Shape3D> shape;
};

struct InstanceLibraryMultiMeshItemSettings {
	static const int MAX_MESH_LODS = 4;

	FixedArray<Ref<Mesh>, MAX_MESH_LODS> mesh_lods;
	unsigned int mesh_lod_count = 1;
	int render_layer = 1;

	// 最好将材质直接放在网格上，
	// 但这是针对使用 OBJ 网格的情况，此类网格通常没有自己的材质
	Ref<Material> material_override;

	RenderingServerEnums::ShadowCastingSetting shadow_casting_setting = RenderingServerEnums::SHADOW_CASTING_SETTING_ON;
	GeometryInstance3D::GIMode gi_mode = GeometryInstance3D::GIMode::GI_MODE_STATIC;

	int collision_mask = 1;
	int collision_layer = 1;
	StdVector<CollisionShapeInfo> collision_shapes;
	// 如果碰撞体使用节点，则会添加到碰撞体的组
	StdVector<StringName> group_names;
};

// 可供 VoxelInstancer 使用的模型设置
class VoxelInstanceLibraryMultiMeshItem : public VoxelInstanceLibraryItem {
	GDCLASS(VoxelInstanceLibraryMultiMeshItem, VoxelInstanceLibraryItem)
public:
	using Settings = InstanceLibraryMultiMeshItemSettings;

	static const int MAX_MESH_LODS = Settings::MAX_MESH_LODS;
	static const char *MANUAL_SETTINGS_GROUP_NAME;
	static const char *SCENE_SETTINGS_GROUP_NAME;

	static constexpr float MIN_DISTANCE_RATIO = 0.f;
	// 可以大于 1，因为与 VoxelTerrain 一起使用时它基于可见区域的半边长，
	// 该区域是正方形，因此网格 LOD 覆盖的圆形区域在需要时实际上可以向外延伸一点。
	static constexpr float MAX_DISTANCE_RATIO = 2.f;

	enum RemovalBehavior {
		REMOVAL_BEHAVIOR_NONE,
		REMOVAL_BEHAVIOR_INSTANTIATE,
		REMOVAL_BEHAVIOR_CALLBACK,
		REMOVAL_BEHAVIOR_COUNT
	};

	VoxelInstanceLibraryMultiMeshItem();

	// 设置或获取指定 LOD 的网格
	void set_mesh(Ref<Mesh> mesh, int mesh_lod_index);
	Ref<Mesh> get_mesh(int mesh_lod_index) const;

	// 设置或获取指定网格 LOD 的切换距离比例
	void set_mesh_lod_distance_ratio(int mesh_lod_index, float ratio);
	float get_mesh_lod_distance_ratio(int mesh_lod_index) const;

	// 网格 LOD 的数量
	int get_mesh_lod_count() const;

	// 渲染层
	void set_render_layer(int render_layer);
	int get_render_layer() const;

	// 材质覆盖
	void set_material_override(Ref<Material> material);
	Ref<Material> get_material_override() const;

	// 阴影投射设置
	void set_cast_shadows_setting(RenderingServerEnums::ShadowCastingSetting mode);
	RenderingServerEnums::ShadowCastingSetting get_cast_shadows_setting() const;

	// 全局光照模式
	void set_gi_mode(GeometryInstance3D::GIMode mode);
	GeometryInstance3D::GIMode get_gi_mode() const;

	// 碰撞层
	void set_collision_layer(int collision_layer);
	int get_collision_layer() const;

	// 碰撞掩码
	void set_collision_mask(int collision_mask);
	int get_collision_mask() const;

	// 碰撞体所属的组
	void set_collider_group_names(TypedArray<StringName> names);
	TypedArray<StringName> get_collider_group_names() const;

	// 从模板节点应用设置
	void setup_from_template(Node *root);

	// 用作模型的场景
	void set_scene(Ref<PackedScene> scene);
	Ref<PackedScene> get_scene() const;

	// 超出最大 LOD 时是否隐藏
	bool get_hide_beyond_max_lod() const;
	void set_hide_beyond_max_lod(bool enabled);

	// 实例被移除时的行为
	void set_removal_behavior(const RemovalBehavior rb);
	RemovalBehavior get_removal_behavior() const;

	// 实例被移除时实例化的场景
	void set_removal_scene(Ref<PackedScene> scene);
	Ref<PackedScene> get_removal_scene() const;

	// 碰撞体生效的距离
	void set_collision_distance(const float distance);
	float get_collision_distance() const;

	// 内部

	// 触发实例移除回调
	void trigger_removal_callback(VoxelInstancer *instancer, const Transform3D &trans);

	// 如果给项目分配了场景，则返回从场景转换而来的设置。
	// 如果没有分配场景，则返回手动设置。
	const Settings &get_multimesh_settings() const;

	inline Span<const CollisionShapeInfo> get_collision_shapes() const {
		return to_span_const(get_multimesh_settings().collision_shapes);
	}

	inline Span<const float> get_mesh_lod_distance_ratios() const {
		return to_span(_mesh_lod_max_distance_ratios);
	}

	// 序列化或反序列化项目属性
	Array serialize_multimesh_item_properties() const;
	void deserialize_multimesh_item_properties(Array a);

#ifdef TOOLS_ENABLED
	void get_configuration_warnings(PackedStringArray &warnings) const override;
#endif

private:
	GDVIRTUAL2(_on_instance_removed, VoxelInstancer *, Transform3D)

private:
	// bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	static void _bind_methods();

	void _b_set_collision_shapes(Array shape_infos);
	Array _b_get_collision_shapes() const;

	Ref<Mesh> _b_get_mesh_lod0() const {
		return get_mesh(0);
	}
	Ref<Mesh> _b_get_mesh_lod1() const {
		return get_mesh(1);
	}
	Ref<Mesh> _b_get_mesh_lod2() const {
		return get_mesh(2);
	}
	Ref<Mesh> _b_get_mesh_lod3() const {
		return get_mesh(3);
	}

	void _b_set_mesh_lod0(Ref<Mesh> mesh) {
		set_mesh(mesh, 0);
	}
	void _b_set_mesh_lod1(Ref<Mesh> mesh) {
		set_mesh(mesh, 1);
	}
	void _b_set_mesh_lod2(Ref<Mesh> mesh) {
		set_mesh(mesh, 2);
	}
	void _b_set_mesh_lod3(Ref<Mesh> mesh) {
		set_mesh(mesh, 3);
	}

	void _b_set_mesh_lod0_distance_ratio(float ratio) {
		set_mesh_lod_distance_ratio(0, ratio);
	}
	void _b_set_mesh_lod1_distance_ratio(float ratio) {
		set_mesh_lod_distance_ratio(1, ratio);
	}
	void _b_set_mesh_lod2_distance_ratio(float ratio) {
		set_mesh_lod_distance_ratio(2, ratio);
	}
	void _b_set_mesh_lod3_distance_ratio(float ratio) {
		set_mesh_lod_distance_ratio(3, ratio);
	}

	float _b_get_mesh_lod0_distance_ratio() const {
		return get_mesh_lod_distance_ratio(0);
	}
	float _b_get_mesh_lod1_distance_ratio() const {
		return get_mesh_lod_distance_ratio(1);
	}
	float _b_get_mesh_lod2_distance_ratio() const {
		return get_mesh_lod_distance_ratio(2);
	}
	float _b_get_mesh_lod3_distance_ratio() const {
		return get_mesh_lod_distance_ratio(3);
	}

	PackedFloat32Array _b_get_mesh_lod_distance_ratios() const;
	void _b_set_mesh_lod_distance_ratios(PackedFloat32Array ratios);

	// 在检视面板和脚本中手动设置，并保存到资源文件中的设置。
	Settings _manual_settings;
	// 运行时从 `_scene` 属性收集的设置。它们不会保存到资源文件中，
	// 优先级高于手动设置。
	Settings _scene_settings;
	// 如果不为空，则会在运行时转换并使用，而不是使用手动设置。
	// 与手动设置相比，这种方式有几个好处：
	// - 使用 Godot 的场景编辑器而不是受限的检视面板来设置模型，工作流更佳
	// - 场景变化时自动更新
	// - 如果场景将自身的所有网格和纹理都内嵌其中，就不会使资源膨胀
	//   （不像在编辑器内转换场景）。导入的场景通常是这种情况：转换会把网格的引用复制
	//   到手动设置中，但 Godot 保存资源时会发现网格没有独立文件，于是复制一份
	//   并再次内嵌到资源中。
	Ref<PackedScene> _scene;
	// 当地形没有 LOD 或该项目位于最后一个 LOD 时，可使用它
	bool _hide_beyond_max_lod = false;
	FixedArray<float, MAX_MESH_LODS> _mesh_lod_max_distance_ratios;

	RemovalBehavior _removal_behavior = REMOVAL_BEHAVIOR_NONE;
	Ref<PackedScene> _removal_scene;

	float _collision_distance = -1.f;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelInstanceLibraryMultiMeshItem::RemovalBehavior)

#endif // VOXEL_INSTANCE_LIBRARY_MULTIMESH_ITEM_H
