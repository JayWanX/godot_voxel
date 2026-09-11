#include "voxel_blocky_model.h"
#include "../../util/containers/container_funcs.h"
#include "../../util/godot/classes/array_mesh.h"
#include <scene/resources/material.h>
#include <scene/resources/material.h>
#include <core/variant/array.h>
#include "../../util/godot/core/string.h"
#include "../../util/math/conv.h"
#include <core/math/vector3.h>
#include "../../util/string/format.h"
#include "blocky_material_indexer.h"
#include "blocky_model_baking_context.h"
#include "voxel_blocky_library.h"
#include <scene/resources/material.h>
#include <scene/resources/material.h>
#include <core/variant/array.h>
#include "../../util/math/vector3.h"
#include <core/object/class_db.h>


// TODO 只是因为 MAX_MATERIALS 才需要……也许反转该依赖就够了
#include "voxel_mesher_blocky.h"

#include "voxel_blocky_model_cube.h"

namespace voxel {

VoxelBlockyModel::VoxelBlockyModel() : _color(1.f, 1.f, 1.f) {}

bool VoxelBlockyModel::_set(const StringName &p_name, const Variant &p_value) {
	String property_name = p_name;

	if (property_name.begins_with("material_override_")) {
		const int index = property_name.substr(string_literal_length("material_override_")).to_int();
		set_material_override(index, p_value);
		return true;

	} else if (property_name.begins_with("collision_enabled_")) {
		const int index = property_name.substr(string_literal_length("collision_enabled_")).to_int();
		set_mesh_collision_enabled(index, p_value);
		return true;
	}

	// 旧版

	if (property_name.begins_with("cube_tiles_")) {
		String s = property_name.substr(string_literal_length("cube_tiles_"), property_name.length());
		Cube::Side side = VoxelBlockyModelCube::name_to_side(s);
		if (side != Cube::SIDE_COUNT) {
			Vector2 v = p_value;
			_legacy_properties.cube_tiles[side] = Vector2f(v.x, v.y);
			return true;
		}
		_legacy_properties.found = true;
		return true;

	} else if (property_name == "geometry_type") {
		_legacy_properties.geometry_type = LegacyProperties::GeometryType(int(p_value));
		_legacy_properties.found = true;
		return true;

	} else if (property_name == "voxel_name") {
		_legacy_properties.name = p_value;
		_legacy_properties.found = true;
		return true;

	} else if (property_name == "custom_mesh") {
		_legacy_properties.custom_mesh = p_value;
		_legacy_properties.found = true;
		return true;
	}

	return false;
}

bool VoxelBlockyModel::_get(const StringName &p_name, Variant &r_ret) const {
	String property_name = p_name;

	if (property_name.begins_with("material_override_")) {
		const int index = property_name.substr(string_literal_length("material_override_")).to_int();
		r_ret = get_material_override(index);
		return true;

	} else if (property_name.begins_with("collision_enabled_")) {
		const int index = property_name.substr(string_literal_length("collision_enabled_")).to_int();
		r_ret = is_mesh_collision_enabled(index);
		return true;
	}

	// 旧版

	if (property_name.begins_with("cube_tiles_")) {
		String s = property_name.substr(string_literal_length("cube_tiles_"), property_name.length());
		Cube::Side side = VoxelBlockyModelCube::name_to_side(s);
		if (side != Cube::SIDE_COUNT) {
			const Vector2f f = _legacy_properties.cube_tiles[side];
			r_ret = Vector2(f.x, f.y);
			return true;
		}

	} else if (property_name == "geometry_type") {
		r_ret = int(_legacy_properties.geometry_type);
		return true;

	} else if (property_name == "voxel_name") {
		r_ret = _legacy_properties.name;
		return true;

	} else if (property_name == "custom_mesh") {
		r_ret = _legacy_properties.custom_mesh;
		return true;
	}

	return false;
}

void VoxelBlockyModel::_get_property_list(List<PropertyInfo> *p_list) const {
	if (_surface_count > 0) {
		p_list->push_back(PropertyInfo(
				Variant::NIL, "Material overrides", PROPERTY_HINT_NONE, "material_override_", PROPERTY_USAGE_GROUP
		));

		for (unsigned int i = 0; i < _surface_count; ++i) {
			p_list->push_back(PropertyInfo(
					Variant::OBJECT,
					String("material_override_{0}").format(varray(i)),
					PROPERTY_HINT_RESOURCE_TYPE,
					voxel::godot::MATERIAL_3D_PROPERTY_HINT_STRING
			));
		}

		p_list->push_back(PropertyInfo(
				Variant::NIL, "Mesh collision", PROPERTY_HINT_NONE, "collision_enabled_", PROPERTY_USAGE_GROUP
		));

		for (unsigned int i = 0; i < _surface_count; ++i) {
			p_list->push_back(PropertyInfo(Variant::BOOL, String("collision_enabled_{0}").format(varray(i))));
		}
	}
}

// void VoxelBlockyModel::set_voxel_name(String name) {
// 	_name = name;
// }

// void VoxelBlockyModel::set_id(int id) {
// 	ERR_FAIL_COND(id < 0 || (unsigned int)id >= VoxelBlockyLibrary::MAX_VOXEL_TYPES);
// 	// 创建后无法修改 ID
// 	ERR_FAIL_COND_MSG(_id != -1, "ID cannot be modified after being added to a library");
// 	_id = id;
// }

void VoxelBlockyModel::set_color(Color color) {
	if (color != _color) {
		_color = color;
		emit_changed();
	}
}

void VoxelBlockyModel::set_material_override(int index, Ref<Material> material) {
	// TODO 不能改用它检查 `_surface_count`，因为加载资源时无法保证 Godot 设置属性的顺序。
	// 网格可能稍后才被设置，因此无法知道表面的数量。
	ERR_FAIL_INDEX(index, int(_surface_params.size()));
	_surface_params[index].material_override = material;
	emit_changed();
}

Ref<Material> VoxelBlockyModel::get_material_override(int index) const {
	// TODO 不能改用它检查 `_surface_count`，因为加载资源时无法保证 Godot 设置属性的顺序。
	// 网格可能稍后才被设置，因此无法知道表面的数量。
	ERR_FAIL_INDEX_V(index, int(_surface_params.size()), Ref<Material>());
	return _surface_params[index].material_override;
}

bool VoxelBlockyModel::has_material_override() const {
	for (const SurfaceParams &sp : _surface_params) {
		if (sp.material_override.is_valid()) {
			return true;
		}
	}
	return false;
}

void VoxelBlockyModel::set_mesh_collision_enabled(int surface_index, bool enabled) {
	// TODO 不能改用它检查 `_surface_count`，因为加载资源时无法保证 Godot 设置属性的顺序。
	// 网格可能稍后才被设置，因此无法知道表面的数量。
	ERR_FAIL_INDEX(surface_index, int(_surface_params.size()));
	_surface_params[surface_index].collision_enabled = enabled;
}

bool VoxelBlockyModel::is_mesh_collision_enabled(int surface_index) const {
	// TODO 不能改用它检查 `_surface_count`，因为加载资源时无法保证 Godot 设置属性的顺序。
	// 网格可能稍后才被设置，因此无法知道表面的数量。
	ERR_FAIL_INDEX_V(surface_index, int(_surface_params.size()), false);
	return _surface_params[surface_index].collision_enabled;
}

void VoxelBlockyModel::set_transparency_index(int i) {
	_transparency_index = math::clamp(i, 0, 255);
}

void VoxelBlockyModel::set_culls_neighbors(bool cn) {
	_culls_neighbors = cn;
}

void VoxelBlockyModel::set_lod_skirts_enabled(bool enabled) {
	_lod_skirts = enabled;
}

bool VoxelBlockyModel::get_lod_skirts_enabled() const {
	return _lod_skirts;
}

void VoxelBlockyModel::set_surface_count(unsigned int new_count) {
	if (new_count != _surface_count) {
		_surface_count = new_count;
#ifdef TOOLS_ENABLED
		notify_property_list_changed();
#endif
	}
}

void VoxelBlockyModel::set_collision_mask(uint32_t mask) {
	_collision_mask = mask;
}

void VoxelBlockyModel::bake(blocky::ModelBakingContext &ctx) const {
	// TODO 这有点不确定，能否设计得更好？
	// 以下逻辑必须在派生类之后运行，不应直接调用

	blocky::BakedModel &baked_data = ctx.model;
	blocky::MaterialIndexer &materials = ctx.material_indexer;

	// baked_data.contributes_to_ao 由侧面剔除阶段设置
	baked_data.transparency_index = _transparency_index;
	baked_data.culls_neighbors = _culls_neighbors;
	baked_data.color = _color;
	baked_data.is_random_tickable = _random_tickable;
	baked_data.box_collision_mask = _collision_mask;
	baked_data.tags_mask = _tags_mask;
	baked_data.box_collision_aabbs = _collision_aabbs;
	baked_data.lod_skirts = _lod_skirts;

	blocky::BakedModel::Model &model = baked_data.model;

	// 注意：网格旋转不在这里实现，它是在派生类中完成的。

	// 设置空侧面掩码
	model.empty_sides_mask = 0;
	for (unsigned int side = 0; side < Cube::SIDE_COUNT; ++side) {
		if (!voxel::is_empty(model.sides_surfaces[side])) {
			continue;
		}
		model.empty_sides_mask |= (1 << side);
	}

	// 如果存在，分配材质覆盖
	for (unsigned int surface_index = 0; surface_index < model.surface_count; ++surface_index) {
		if (surface_index < _surface_count) {
			const SurfaceParams &surface_params = _surface_params[surface_index];

			blocky::BakedModel::Surface &surface = model.surfaces[surface_index];

			if (surface_params.material_override.is_valid()) {
				const unsigned int material_index = materials.get_or_create_index(surface_params.material_override);
				surface.material_id = material_index;
			}

			surface.collision_enabled = surface_params.collision_enabled;
		}
	}
}

TypedArray<AABB> VoxelBlockyModel::_b_get_collision_aabbs() const {
	TypedArray<AABB> array;
	array.resize(_collision_aabbs.size());
	for (size_t i = 0; i < _collision_aabbs.size(); ++i) {
		array[i] = _collision_aabbs[i];
	}
	return array;
}

void VoxelBlockyModel::_b_set_collision_aabbs(TypedArray<AABB> array) {
	for (int i = 0; i < array.size(); ++i) {
		const Variant v = array[i];
		// ERR_FAIL_COND(v.get_type() != Variant::AABB);
		// TODO 即使在类型化数组中，Godot 数组检视器中的"添加元素"也总会添加一个 null 元素！
		if (v.get_type() != Variant::AABB) {
			VOXEL_PRINT_WARNING(
					format("Item {} of the array is not an AABB (found {}). It will be replaced.",
						   i,
						   Variant::get_type_name(v.get_type()))
			);
			array[i] = AABB(Vector3(), Vector3(1, 1, 1));
		}
	}
	_collision_aabbs.resize(array.size());
	for (int i = 0; i < array.size(); ++i) {
		const AABB aabb = array[i];
		_collision_aabbs[i] = aabb;
	}
	emit_changed();
}

unsigned int VoxelBlockyModel::get_collision_aabb_count() const {
	return _collision_aabbs.size();
}

void VoxelBlockyModel::set_collision_aabb(unsigned int i, AABB aabb) {
	VOXEL_ASSERT_RETURN(i < _collision_aabbs.size());
	_collision_aabbs[i] = aabb;
}

void VoxelBlockyModel::set_collision_aabbs(Span<const AABB> aabbs) {
	_collision_aabbs.resize(aabbs.size());
	for (unsigned int i = 0; i < aabbs.size(); ++i) {
		_collision_aabbs[i] = aabbs[i];
	}
}

void VoxelBlockyModel::set_random_tickable(bool rt) {
	_random_tickable = rt;
}

bool VoxelBlockyModel::is_random_tickable() const {
	return _random_tickable;
}

void VoxelBlockyModel::set_tags_mask(const uint32_t mask) {
	_tags_mask = mask;
}

uint32_t VoxelBlockyModel::get_tags_mask() const {
	return _tags_mask;
}

#ifdef TOOLS_ENABLED

void VoxelBlockyModel::get_configuration_warnings(PackedStringArray &out_warnings) const {
	// 子类中可能有实现
}

#endif

bool VoxelBlockyModel::is_empty() const {
	VOXEL_PRINT_ERROR("Not implemented");
	// 在子类中实现
	return true;
}

void VoxelBlockyModel::copy_base_properties_from(const VoxelBlockyModel &src) {
	_surface_params = src._surface_params;
	// _surface_count = src._surface_count;
	_transparency_index = src._transparency_index;
	_culls_neighbors = src._culls_neighbors;
	_random_tickable = src._random_tickable;
	_color = src._color;
	_collision_aabbs = src._collision_aabbs;
	_collision_mask = src._collision_mask;
}

Ref<Mesh> VoxelBlockyModel::get_preview_mesh() const {
	VOXEL_PRINT_ERROR("Not implemented");
	// 在子类中实现
	return Ref<Mesh>();
}

Ref<Mesh> VoxelBlockyModel::make_mesh_from_baked_data(
		const blocky::BakedModel &baked_data,
		const bool tangents_enabled
) {
	return make_mesh_from_baked_data(
			to_span(baked_data.model.surfaces),
			to_span(baked_data.model.sides_surfaces),
			baked_data.color,
			tangents_enabled
	);
}

Ref<Mesh> VoxelBlockyModel::make_mesh_from_baked_data(
		Span<const blocky::BakedModel::Surface> inner_surfaces,
		Span<const FixedArray<blocky::BakedModel::SideSurface, blocky::MAX_SURFACES>> sides_surfaces,
		const Color model_color,
		const bool tangents_enabled
) {
	Ref<ArrayMesh> mesh;
	mesh.instantiate();

	for (unsigned int surface_index = 0; surface_index < inner_surfaces.size(); ++surface_index) {
		const blocky::BakedModel::Surface &surface = inner_surfaces[surface_index];

		// 获取表面中的顶点和索引数量
		unsigned int vertex_count = surface.positions.size();
		unsigned int index_count = surface.indices.size();
		for (const FixedArray<blocky::BakedModel::SideSurface, blocky::MAX_SURFACES> &side_surfaces : sides_surfaces) {
			const blocky::BakedModel::SideSurface &side_surface = side_surfaces[surface_index];
			vertex_count += side_surface.positions.size();
			index_count += side_surface.indices.size();
		}

		// Godot 不喜欢在向网格添加表面时传入空数组
		if (index_count == 0) {
			continue;
		}

		// 分配表面数组

		PackedVector3Array vertices;
		PackedVector3Array normals;
		PackedFloat32Array tangents;
		PackedColorArray colors;
		PackedVector2Array uvs;

		vertices.resize(vertex_count);
		normals.resize(vertex_count);
		if (tangents_enabled) {
			tangents.resize(vertex_count * 4);
		}
		colors.resize(vertex_count);
		uvs.resize(vertex_count);

		PackedInt32Array indices;
		indices.resize(index_count);

		Span<Vector3> vertices_w(vertices.ptrw(), vertices.size());
		Span<Vector3> normals_w(normals.ptrw(), normals.size());
		Span<float> tangents_w;
		if (tangents_enabled) {
			tangents_w = Span<float>(tangents.ptrw(), tangents.size());
		}
		Span<Color> colors_w(colors.ptrw(), colors.size());
		Span<Vector2> uvs_w(uvs.ptrw(), uvs.size());
		Span<int> indices_w(indices.ptrw(), indices.size());

		// 填充数组

		unsigned int vi = 0;
		unsigned int ti = 0;
		unsigned int ii = 0;

		for (unsigned int i = 0; i < surface.positions.size(); ++i) {
			vertices_w[vi] = to_vec3(surface.positions[i]);
			normals_w[vi] = to_vec3(surface.normals[i]);
			colors_w[vi] = model_color;
			uvs_w[vi] = to_vec2(surface.uvs[i]);
			++vi;
		}
		if (tangents_enabled) {
			for (unsigned int i = 0; i < surface.tangents.size(); ++i) {
				tangents_w[ti] = surface.tangents[i];
				++ti;
			}
		}
		for (unsigned int i = 0; i < surface.indices.size(); ++i) {
			indices_w[ii] = surface.indices[i];
			++ii;
		}

		for (unsigned int side = 0; side < sides_surfaces.size(); ++side) {
			const blocky::BakedModel::SideSurface &side_surface = sides_surfaces[side][surface_index];
			Span<const Vector3f> side_positions = to_span(side_surface.positions);
			Span<const Vector2f> side_uvs = to_span(side_surface.uvs);
			Span<const int> side_indices = to_span(side_surface.indices);
			Span<const float> side_tangents = to_span(side_surface.tangents);
			const Vector3 side_normal = to_vec3(Cube::g_side_normals[side]);

			const unsigned int vi0 = vi;

			for (unsigned int i = 0; i < side_positions.size(); ++i) {
				vertices_w[vi] = to_vec3(side_positions[i]);
				normals_w[vi] = side_normal;
				colors_w[vi] = model_color;
				uvs_w[vi] = to_vec2(side_uvs[i]);
				++vi;
			}
			if (tangents_enabled) {
				for (unsigned int i = 0; i < side_tangents.size(); ++i) {
					tangents_w[ti] = side_tangents[i];
					++ti;
				}
			}
			for (unsigned int i = 0; i < side_indices.size(); ++i) {
				indices_w[ii] = vi0 + side_indices[i];
				++ii;
			}
		}

		Array arrays;
		arrays.resize(Mesh::ARRAY_MAX);
		arrays[Mesh::ARRAY_VERTEX] = vertices;
		arrays[Mesh::ARRAY_NORMAL] = normals;
		arrays[Mesh::ARRAY_TEX_UV] = uvs;
		arrays[Mesh::ARRAY_COLOR] = colors;
		if (tangents_enabled) {
			arrays[Mesh::ARRAY_TANGENT] = tangents;
		}
		arrays[Mesh::ARRAY_INDEX] = indices;

		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	}

	return mesh;
}

void VoxelBlockyModel::rotate_collision_boxes_90(math::Axis axis, bool clockwise) {
	for (AABB &aabb : _collision_aabbs) {
		// 使其居中，旋转轴是体素的中心
		aabb.position -= Vector3(0.5, 0.5, 0.5);

		FixedArray<Vector3f, 2> points;
		points[0] = to_vec3f(aabb.position);
		points[1] = to_vec3f(aabb.position + aabb.size);
		// TODO 是否将 Axis 枚举移到 vectors 之外？
		math::rotate_90(to_span(points), math::Axis(axis), clockwise);
		const Vector3f min_pos = math::min(points[0], points[1]);
		const Vector3f max_pos = math::max(points[0], points[1]);
		aabb = AABB(to_vec3(min_pos), to_vec3(max_pos - min_pos));

		aabb.position += Vector3(0.5, 0.5, 0.5);
	}
}

void VoxelBlockyModel::rotate_collision_boxes_ortho(math::OrthoBasis ortho_basis) {
	Basis basis(to_vec3(ortho_basis.x), to_vec3(ortho_basis.y), to_vec3(ortho_basis.z));

	for (AABB &aabb : _collision_aabbs) {
		// 使其居中，旋转轴是体素的中心
		aabb.position -= Vector3(0.5, 0.5, 0.5);

		const Vector3 p0 = basis.xform(aabb.position);
		const Vector3 p1 = basis.xform(aabb.position + aabb.size);

		const Vector3 min_pos = math::min(p0, p1);
		const Vector3 max_pos = math::max(p0, p1);
		aabb = AABB(min_pos, max_pos - min_pos);

		aabb.position += Vector3(0.5, 0.5, 0.5);
	}
}

void VoxelBlockyModel::set_mesh_ortho_rotation_index(int i) {
	VOXEL_ASSERT_RETURN(i >= 0 && i < math::ORTHOGONAL_BASIS_COUNT);
	if (i != int(_mesh_ortho_rotation)) {
		_mesh_ortho_rotation = i;
	}
}

int VoxelBlockyModel::get_mesh_ortho_rotation_index() const {
	return _mesh_ortho_rotation;
}

void VoxelBlockyModel::rotate_90(math::Axis axis, bool clockwise) {
	math::OrthoBasis ortho_basis = math::get_ortho_basis_from_index(_mesh_ortho_rotation);
	ortho_basis.rotate_90(axis, clockwise);
	_mesh_ortho_rotation = math::get_index_from_ortho_basis(ortho_basis);

	rotate_collision_boxes_90(axis, clockwise);

	emit_changed();
}

void VoxelBlockyModel::rotate_ortho(math::OrthoBasis p_ortho_basis) {
	math::OrthoBasis ortho_basis = math::get_ortho_basis_from_index(_mesh_ortho_rotation);
	ortho_basis = p_ortho_basis * ortho_basis;
	_mesh_ortho_rotation = math::get_index_from_ortho_basis(ortho_basis);

	rotate_collision_boxes_ortho(p_ortho_basis);

	emit_changed();
}

void VoxelBlockyModel::_b_rotate_90(Vector3i::Axis axis, bool clockwise) {
	ERR_FAIL_INDEX(axis, Vector3i::AXIS_COUNT);
	rotate_90(math::Axis(axis), clockwise);
}

// void ortho_simplify(Span<const Vector3f> vertices, Span<const int> indices, StdVector<int> &output) {
// TODO 优化：基于轴对齐的三角形实现网格简化。
// 它可能对 blocky 网格生成器的网格碰撞非常有效。
// }

void VoxelBlockyModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_color", "color"), &VoxelBlockyModel::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &VoxelBlockyModel::get_color);

	ClassDB::bind_method(
			D_METHOD("set_material_override", "index", "material"), &VoxelBlockyModel::set_material_override
	);
	ClassDB::bind_method(D_METHOD("get_material_override", "index"), &VoxelBlockyModel::get_material_override);

	ClassDB::bind_method(
			D_METHOD("set_transparency_index", "transparency_index"), &VoxelBlockyModel::set_transparency_index
	);
	ClassDB::bind_method(D_METHOD("get_transparency_index"), &VoxelBlockyModel::get_transparency_index);

	ClassDB::bind_method(D_METHOD("set_culls_neighbors", "culls_neighbors"), &VoxelBlockyModel::set_culls_neighbors);
	ClassDB::bind_method(D_METHOD("get_culls_neighbors"), &VoxelBlockyModel::get_culls_neighbors);

	ClassDB::bind_method(D_METHOD("is_random_tickable"), &VoxelBlockyModel::is_random_tickable);
	ClassDB::bind_method(D_METHOD("set_random_tickable"), &VoxelBlockyModel::set_random_tickable);

	ClassDB::bind_method(D_METHOD("get_tags_mask"), &VoxelBlockyModel::get_tags_mask);
	ClassDB::bind_method(D_METHOD("set_tags_mask"), &VoxelBlockyModel::set_tags_mask);

	ClassDB::bind_method(
			D_METHOD("set_mesh_collision_enabled", "surface_index", "enabled"),
			&VoxelBlockyModel::set_mesh_collision_enabled
	);
	ClassDB::bind_method(
			D_METHOD("is_mesh_collision_enabled", "surface_index"), &VoxelBlockyModel::is_mesh_collision_enabled
	);

	ClassDB::bind_method(D_METHOD("set_collision_aabbs", "aabbs"), &VoxelBlockyModel::_b_set_collision_aabbs);
	ClassDB::bind_method(D_METHOD("get_collision_aabbs"), &VoxelBlockyModel::_b_get_collision_aabbs);

	ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &VoxelBlockyModel::set_collision_mask);
	ClassDB::bind_method(D_METHOD("get_collision_mask"), &VoxelBlockyModel::get_collision_mask);

	ClassDB::bind_method(
			D_METHOD("set_mesh_ortho_rotation_index", "i"), &VoxelBlockyModel::set_mesh_ortho_rotation_index
	);
	ClassDB::bind_method(D_METHOD("get_mesh_ortho_rotation_index"), &VoxelBlockyModel::get_mesh_ortho_rotation_index);

	// 仅为编辑器目的绑定
	ClassDB::bind_method(D_METHOD("rotate_90", "axis", "clockwise"), &VoxelBlockyModel::_b_rotate_90);

	ClassDB::bind_method(D_METHOD("set_lod_skirts_enabled", "enabled"), &VoxelBlockyModel::set_lod_skirts_enabled);
	ClassDB::bind_method(D_METHOD("get_lod_skirts_enabled"), &VoxelBlockyModel::get_lod_skirts_enabled);

	// TODO 在 Godot 4 中更新为 StringName
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "transparency_index"), "set_transparency_index", "get_transparency_index");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "culls_neighbors"), "set_culls_neighbors", "get_culls_neighbors");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "random_tickable"), "set_random_tickable", "is_random_tickable");

	// TODO 这本来不应该是 3D 图层。
	// 但无论如何我还是用了它，因为 `PROPERTY_HINT_FLAGS` 不适用，而 Godot 没有提供自定义图层的方式。
	// 我们可以自己做一个，但那工作量很大。
	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "tags_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_tags_mask", "get_tags_mask"
	);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "lod_skirts_enabled"), "set_lod_skirts_enabled", "get_lod_skirts_enabled");

	ADD_GROUP("Box collision", "");

	// TODO `hint_string` 搭配 `ARRAY` 时 `number:` 语法是什么意思？它很旧，在 Godot 的
	// 代码库中很难搜索，而且我在文档的任何地方都找不到它
	ADD_PROPERTY(
			PropertyInfo(
					Variant::ARRAY, "collision_aabbs", PROPERTY_HINT_TYPE_STRING, String::num_int64(Variant::AABB) + ":"
			),
			"set_collision_aabbs",
			"get_collision_aabbs"
	);
	ADD_PROPERTY(
			// TODO 这个碰撞掩码可能实际上与 Godot 标准物理无关。
			// 它主要用于体素射线投射、盒体碰撞以及可能其它用途
			PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS),
			"set_collision_mask",
			"get_collision_mask"
	);

	// 注意：旋转属性目前只在派生类中暴露。
	// 并非所有派生类都一定支持它。

	BIND_ENUM_CONSTANT(SIDE_NEGATIVE_X);
	BIND_ENUM_CONSTANT(SIDE_POSITIVE_X);
	BIND_ENUM_CONSTANT(SIDE_NEGATIVE_Y);
	BIND_ENUM_CONSTANT(SIDE_POSITIVE_Y);
	BIND_ENUM_CONSTANT(SIDE_NEGATIVE_Z);
	BIND_ENUM_CONSTANT(SIDE_POSITIVE_Z);
	BIND_ENUM_CONSTANT(SIDE_COUNT);
}

} // namespace voxel
