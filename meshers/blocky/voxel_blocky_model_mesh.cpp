#include "voxel_blocky_model_mesh.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/array_mesh.h"
#include "../../util/godot/classes/object.h"
#include "../../util/godot/classes/point_mesh.h"
#include "../../util/godot/classes/surface_tool.h"
#include "../../util/godot/core/array.h"
#include "../../util/godot/core/packed_arrays.h"
#include "../../util/godot/core/string.h"
#include "../../util/math/conv.h"
#include "../../util/math/ortho_basis.h"
#include "../../util/string/format.h"
#include "blocky_material_indexer.h"
#include "blocky_model_baking_context.h"
#include "voxel_blocky_library.h"
#include "voxel_blocky_model_empty.h"

namespace voxel {

void VoxelBlockyModelMesh::set_mesh(Ref<Mesh> mesh) {
	{
		Ref<PointMesh> point_mesh = mesh;
		if (point_mesh.is_valid()) {
			VOXEL_PRINT_ERROR(format("PointMesh is not supported by {}.", VOXEL_CLASS_NAME_C(VoxelBlockyModelMesh)));
			return;
		}
	}
	_mesh = mesh;
	if (_mesh.is_valid()) {
		set_surface_count(_mesh->get_surface_count());
	} else {
		set_surface_count(0);
	}
	emit_changed();
}

namespace {

#ifdef TOOLS_ENABLED
// 基于 UV 生成切线（不会像正确导入的切线那样好）
PackedFloat32Array generate_tangents_from_uvs(
		const PackedVector3Array &positions,
		const PackedVector3Array &normals,
		const PackedVector2Array &uvs,
		const PackedInt32Array &indices
) {
	PackedFloat32Array tangents;
	tangents.resize(positions.size() * 4);

	FixedArray<Vector3f, 3> tri_positions;

	for (int i = 0; i < indices.size(); i += 3) {
		tri_positions[0] = to_vec3f(positions[indices[i]]);
		tri_positions[1] = to_vec3f(positions[indices[i + 1]]);
		tri_positions[2] = to_vec3f(positions[indices[i + 2]]);

		FixedArray<float, 4> tangent;

		const Vector2f delta_uv1 = to_vec2f(uvs[indices[i + 1]] - uvs[indices[i]]);
		const Vector2f delta_uv2 = to_vec2f(uvs[indices[i + 2]] - uvs[indices[i]]);
		const Vector3f delta_pos1 = tri_positions[1] - tri_positions[0];
		const Vector3f delta_pos2 = tri_positions[2] - tri_positions[0];
		const float r = 1.0f / (delta_uv1[0] * delta_uv2[1] - delta_uv1[1] * delta_uv2[0]);
		const Vector3f t = (delta_pos1 * delta_uv2[1] - delta_pos2 * delta_uv1[1]) * r;
		const Vector3f bt = (delta_pos2 * delta_uv1[0] - delta_pos1 * delta_uv2[0]) * r;
		tangent[0] = t[0];
		tangent[1] = t[1];
		tangent[2] = t[2];
		tangent[3] = (math::dot(bt, math::cross(to_vec3f(normals[indices[i]]), t))) < 0 ? -1.0f : 1.0f;

		tangents.append(tangent[0]);
		tangents.append(tangent[1]);
		tangents.append(tangent[2]);
		tangents.append(tangent[3]);
	}

	return tangents;
}
#endif

void add(Span<Vector3> vectors, Vector3 rhs) {
	for (Vector3 &v : vectors) {
		v += rhs;
	}
}

void mul(Span<Vector3> vectors, const Basis &basis) {
	for (Vector3 &v : vectors) {
		v = basis.xform(v);
	}
}

void add(PackedVector3Array &vectors, Vector3 rhs) {
	add(Span<Vector3>(vectors.ptrw(), vectors.size()), rhs);
}

void rotate_mesh_arrays(Span<Vector3> vertices, Span<Vector3> normals, Span<float> &tangents, const Basis &basis) {
	mul(vertices, basis);

	if (tangents.size() == 0) {
		mul(normals, basis);

	} else {
		const unsigned int tangent_count = tangents.size() / 4;
		VOXEL_ASSERT_RETURN(tangent_count == normals.size());

		for (unsigned int ti = 0; ti < tangent_count; ++ti) {
			const unsigned int i0 = ti * 4;

			Vector3 normal = normals[ti];
			Vector3 tangent(tangents[i0], tangents[i0 + 1], tangents[i0 + 2]);
			Vector3 bitangent(normal.cross(tangent) * tangents[i0 + 3]);

			normal = basis.xform(normal);
			tangent = basis.xform(tangent);
			bitangent = basis.xform(bitangent);

			const float bitangent_s = math::sign_nonzero(bitangent.dot(normal.cross(tangent)));

			normals[ti] = normal;

			tangents[i0] = tangent.x;
			tangents[i0 + 1] = tangent.y;
			tangents[i0 + 2] = tangent.z;
			tangents[i0 + 3] = bitangent_s;
		}
	}
}

void mul(Span<Vector3f> vectors, const Basis3f &basis) {
	for (Vector3f &v : vectors) {
		v = basis.xform(v);
	}
}

} // namespace

void rotate_mesh_arrays(Span<Vector3f> vertices, Span<Vector3f> normals, Span<float> tangents, const Basis3f &basis) {
	mul(vertices, basis);

	if (tangents.size() == 0) {
		mul(normals, basis);

	} else {
		const unsigned int tangent_count = tangents.size() / 4;
		VOXEL_ASSERT_RETURN(tangent_count == normals.size());

		for (unsigned int ti = 0; ti < tangent_count; ++ti) {
			const unsigned int i0 = ti * 4;

			Vector3f normal = normals[ti];
			Vector3f tangent(tangents[i0], tangents[i0 + 1], tangents[i0 + 2]);
			Vector3f bitangent(math::cross(normal, tangent) * tangents[i0 + 3]);

			normal = basis.xform(normal);
			tangent = basis.xform(tangent);
			bitangent = basis.xform(bitangent);

			const float bitangent_s = math::sign_nonzero(math::dot(bitangent, math::cross(normal, tangent)));

			normals[ti] = normal;

			tangents[i0] = tangent.x;
			tangents[i0 + 1] = tangent.y;
			tangents[i0 + 2] = tangent.z;
			tangents[i0 + 3] = bitangent_s;
		}
	}
}

namespace {

void rotate_mesh_arrays(
		PackedVector3Array &vertices,
		PackedVector3Array &normals,
		PackedFloat32Array &tangents,
		const Basis &basis
) {
	Span<Vector3> vertices_w(vertices.ptrw(), vertices.size());
	Span<Vector3> normals_w(normals.ptrw(), normals.size());
	Span<float> tangents_w(tangents.ptrw(), tangents.size());

	rotate_mesh_arrays(vertices_w, normals_w, tangents_w, basis);
}

void rotate_mesh_arrays_ortho(
		PackedVector3Array &vertices,
		PackedVector3Array &normals,
		PackedFloat32Array &tangents,
		unsigned int ortho_basis_index
) {
	const math::OrthoBasis ortho_basis = math::get_ortho_basis_from_index(ortho_basis_index);
	const Basis basis(to_vec3(ortho_basis.x), to_vec3(ortho_basis.y), to_vec3(ortho_basis.z));
	rotate_mesh_arrays(vertices, normals, tangents, basis);
}

bool validate_indices(Span<const int> indices, int vertex_count) {
	VOXEL_ASSERT_RETURN_V(vertex_count >= 0, false);
	for (const int index : indices) {
		if (index < 0 || index >= vertex_count) {
			VOXEL_PRINT_ERROR(
					format("Invalid index found in mesh indices. Maximum is {}, found {}", vertex_count - 1, index)
			);
			return false;
		}
	}
	return true;
}

} // namespace

namespace blocky {

void bake_mesh_geometry(
		const Span<const Array> surfaces,
		const Span<const Ref<Material>> materials,
		BakedModel &baked_data,
		const bool bake_tangents,
		MaterialIndexer &material_indexer,
		const unsigned int ortho_rotation,
		const float side_vertex_tolerance
) {
	baked_data.model.surface_count = surfaces.size();

	for (unsigned int surface_index = 0; surface_index < surfaces.size(); ++surface_index) {
		const Array &arrays = surfaces[surface_index];

		ERR_CONTINUE(arrays.size() == 0);

		PackedInt32Array indices = arrays[Mesh::ARRAY_INDEX];
		PackedVector3Array positions = arrays[Mesh::ARRAY_VERTEX];
		if (indices.size() == 0) {
			if (positions.size() == 0) {
				VOXEL_PRINT_ERROR(
						format("Mesh surface {} is empty (no vertices, no index buffer). If you want an empty "
							   "model, use {}.",
							   surface_index,
							   VOXEL_CLASS_NAME_C(VoxelBlockyModelEmpty))
				);
				continue;
			} else {
				VOXEL_PRINT_ERROR(
						format("Mesh surface {} is missing an index buffer. Indexed meshes are expected. If you're "
							   "generating the mesh with {}, you may use the {}() method.",
							   surface_index,
							   VOXEL_CLASS_NAME_C(SurfaceTool),
							   VOXEL_METHOD_NAME_C(SurfaceTool, index))
				);
				continue;
			}
		}
		VOXEL_ASSERT_CONTINUE_MSG(
				(indices.size() % 3) == 0,
				format("Mesh surface has an invalid number of indices. "
					   "Expected multiple of 3 (for triangles), found {}",
					   indices.size())
		);

		PackedVector3Array normals = arrays[Mesh::ARRAY_NORMAL];
		PackedVector2Array uvs = arrays[Mesh::ARRAY_TEX_UV];
		PackedFloat32Array tangents = arrays[Mesh::ARRAY_TANGENT];

		// Godot 实际上允许创建带无效索引的 ArrayMesh。烘焙需要有效索引，所以我们必须检查它。
		if (!validate_indices(to_span(indices), positions.size())) {
			continue;
		}

		baked_data.empty = positions.size() == 0;

		VOXEL_ASSERT_CONTINUE_MSG(normals.size() != 0, "The mesh is missing normals, this is not supported.");

		VOXEL_ASSERT_CONTINUE(positions.size() == normals.size());
		// VOXEL_ASSERT_CONTINUE(positions.size() == uvs.size());
		// VOXEL_ASSERT_CONTINUE(positions.size() == tangents.size() * 4);

		if (ortho_rotation != math::ORTHOGONAL_BASIS_IDENTITY_INDEX) {
			// 将网格移到原点以便更容易旋转，因为烘焙的网格范围是 0..1 而非 -0.5..0.5
			// 注意：由于写时复制（CoW），源网格不会被修改
			add(positions, Vector3(-0.5, -0.5, -0.5));
			rotate_mesh_arrays_ortho(positions, normals, tangents, ortho_rotation);
			add(positions, Vector3(0.5, 0.5, 0.5));
		}

		struct L {
			static uint8_t get_sides(Vector3f pos, float tolerance) {
				uint8_t mask = 0;
				mask |= Math::is_equal_approx(pos.x, 0.f, tolerance) << Cube::SIDE_NEGATIVE_X;
				mask |= Math::is_equal_approx(pos.x, 1.f, tolerance) << Cube::SIDE_POSITIVE_X;
				mask |= Math::is_equal_approx(pos.y, 0.f, tolerance) << Cube::SIDE_NEGATIVE_Y;
				mask |= Math::is_equal_approx(pos.y, 1.f, tolerance) << Cube::SIDE_POSITIVE_Y;
				mask |= Math::is_equal_approx(pos.z, 0.f, tolerance) << Cube::SIDE_NEGATIVE_Z;
				mask |= Math::is_equal_approx(pos.z, 1.f, tolerance) << Cube::SIDE_POSITIVE_Z;
				return mask;
			}

			static bool get_triangle_side(
					const Vector3f &a,
					const Vector3f &b,
					const Vector3f &c,
					Cube::SideAxis &out_side,
					float tolerance
			) {
				const uint8_t m = get_sides(a, tolerance) & get_sides(b, tolerance) & get_sides(c, tolerance);
				if (m == 0) {
					// 至少有一个点不属于某个面
					return false;
				}
				for (unsigned int side = 0; side < Cube::SIDE_COUNT; ++side) {
					if (m == (1 << side)) {
						// 所有点都属于同一个面
						out_side = (Cube::SideAxis)side;
						return true;
					}
				}
				// 该三角形不在一个面内
				return false;
			}
		};

#ifdef TOOLS_ENABLED
		const bool tangents_empty = (tangents.size() == 0);
		if (tangents_empty && bake_tangents) {
			if (uvs.size() == 0) {
				// TODO 提供模型使用位置的上下文，它们不能总是被命名
				VOXEL_PRINT_ERROR(
						format("Voxel model is missing tangents and UVs. The model won't be "
							   "baked. You should consider providing a mesh with tangents, or at least UVs and "
							   "normals, or turn off tangents baking in {}.",
							   VOXEL_CLASS_NAME_C(VoxelBlockyLibrary))
				);
				continue;
			}
			VOXEL_PRINT_WARNING(
					format("Voxel model does not have tangents. They will be generated."
						   "You should consider providing a mesh with tangents, or at least UVs and normals, "
						   "or turn off tangents baking in {}.",
						   VOXEL_CLASS_NAME_C(VoxelBlockyLibrary))
			);

			tangents = generate_tangents_from_uvs(positions, normals, uvs, indices);
		}
#endif

		if (uvs.size() == 0) {
			// TODO 如果没有 UV，正确生成它们
			uvs = PackedVector2Array();
			uvs.resize(positions.size());
		}

		// 分离属于立方体面的三角形

		BakedModel::Model &model = baked_data.model;

		BakedModel::Surface &surface = model.surfaces[surface_index];
		Ref<Material> material = materials[surface_index];
		// 注意，空材质计为"默认材质"。
		surface.material_id = material_indexer.get_or_create_index(material);

		FixedArray<StdUnorderedMap<int, int>, Cube::SIDE_COUNT> added_side_indices;
		StdUnorderedMap<int, int> added_regular_indices;
		FixedArray<Vector3f, 3> tri_positions;

		for (int i = 0; i < indices.size(); i += 3) {
			tri_positions[0] = to_vec3f(positions[indices[i]]);
			tri_positions[1] = to_vec3f(positions[indices[i + 1]]);
			tri_positions[2] = to_vec3f(positions[indices[i + 2]]);

			Cube::SideAxis side;
			if (L::get_triangle_side(
						tri_positions[0], tri_positions[1], tri_positions[2], side, side_vertex_tolerance
				)) {
				// 该三角形在面上

				BakedModel::SideSurface &side_surface = model.sides_surfaces[side][surface_index];

				int next_side_index = side_surface.positions.size();

				for (int j = 0; j < 3; ++j) {
					const int src_index = indices[i + j];
					StdUnorderedMap<int, int> &added_indices = added_side_indices[side];
					const auto existing_dst_index_it = added_indices.find(src_index);

					if (existing_dst_index_it == added_indices.end()) {
						// 添加新顶点

						side_surface.indices.push_back(next_side_index);
						side_surface.positions.push_back(tri_positions[j]);
						side_surface.uvs.push_back(to_vec2f(uvs[indices[i + j]]));

						if (bake_tangents) {
							// i 是每个三角形的第一个顶点，以 3 为步长递增。
							// 每个切线有 4 个浮点数。
							int ti = indices[i + j] * 4;
							side_surface.tangents.push_back(tangents[ti]);
							side_surface.tangents.push_back(tangents[ti + 1]);
							side_surface.tangents.push_back(tangents[ti + 2]);
							side_surface.tangents.push_back(tangents[ti + 3]);
						}

						added_indices.insert({ src_index, next_side_index });
						++next_side_index;

					} else {
						// 顶点已添加，只需添加引用它的索引
						side_surface.indices.push_back(existing_dst_index_it->second);
					}
				}

			} else {
				// 该三角形不在面上

				int next_regular_index = surface.positions.size();

				for (int j = 0; j < 3; ++j) {
					const int src_index = indices[i + j];
					const auto existing_dst_index_it = added_regular_indices.find(src_index);

					if (existing_dst_index_it == added_regular_indices.end()) {
						surface.indices.push_back(next_regular_index);
						surface.positions.push_back(tri_positions[j]);
						surface.normals.push_back(to_vec3f(normals[indices[i + j]]));
						surface.uvs.push_back(to_vec2f(uvs[indices[i + j]]));

						if (bake_tangents) {
							// i 是每个三角形的第一个顶点，以 3 为步长递增。
							// 每个切线有 4 个浮点数。
							int ti = indices[i + j] * 4;
							surface.tangents.push_back(tangents[ti]);
							surface.tangents.push_back(tangents[ti + 1]);
							surface.tangents.push_back(tangents[ti + 2]);
							surface.tangents.push_back(tangents[ti + 3]);
						}

						added_regular_indices.insert({ src_index, next_regular_index });
						++next_regular_index;

					} else {
						surface.indices.push_back(existing_dst_index_it->second);
					}
				}
			}
		}
	}
}

void bake_mesh_geometry(
		const VoxelBlockyModelMesh &config,
		BakedModel &baked_data,
		const bool bake_tangents,
		MaterialIndexer &material_indexer,
		const float side_vertex_tolerance,
		const bool side_cutout_enabled
) {
	Ref<Mesh> mesh = config.get_mesh();

	baked_data.cutout_sides_enabled = side_cutout_enabled;

	if (mesh.is_null()) {
		baked_data.empty = true;
		return;
	}

	// TODO 如果发现表面具有相同材质则合并它们（但如果材质不同或为空，仍要打印警告）
	const uint32_t src_surface_count = mesh->get_surface_count();
	if (mesh->get_surface_count() > int(blocky::MAX_SURFACES)) {
		VOXEL_PRINT_WARNING(
				format("Mesh has more than {} surfaces, extra surfaces will not be baked.", blocky::MAX_SURFACES)
		);
	}

	const unsigned int surface_count = math::min(src_surface_count, blocky::MAX_SURFACES);

	StdVector<Ref<Material>> materials;
	StdVector<Array> surfaces;

	for (unsigned int i = 0; i < surface_count; ++i) {
		surfaces.push_back(mesh->surface_get_arrays(i));
		materials.push_back(mesh->surface_get_material(i));
	}

	bake_mesh_geometry(
			to_span(surfaces),
			to_span(materials),
			baked_data,
			bake_tangents,
			material_indexer,
			config.get_mesh_ortho_rotation_index(),
			side_vertex_tolerance
	);
}

} // namespace blocky

void VoxelBlockyModelMesh::bake(blocky::ModelBakingContext &ctx) const {
	blocky::BakedModel &baked_data = ctx.model;
	blocky::MaterialIndexer &materials = ctx.material_indexer;

	baked_data.clear();
	blocky::bake_mesh_geometry(
			*this, baked_data, ctx.tangents_enabled, materials, _side_vertex_tolerance, _side_cutout_enabled
	);
	VoxelBlockyModel::bake(ctx);
}

bool VoxelBlockyModelMesh::is_empty() const {
	if (_mesh.is_null()) {
		return true;
	}
	Ref<ArrayMesh> array_mesh = _mesh;
	if (array_mesh.is_valid()) {
		return godot::is_mesh_empty(**array_mesh);
	}
	return false;
}

Ref<Mesh> VoxelBlockyModelMesh::get_preview_mesh() const {
	const float bake_tangents = false;
	blocky::BakedModel baked_data;
	baked_data.color = get_color();
	StdVector<Ref<Material>> materials;
	blocky::MaterialIndexer material_indexer{ materials };
	bake_mesh_geometry(*this, baked_data, bake_tangents, material_indexer, _side_vertex_tolerance, false);

	Ref<Mesh> mesh = make_mesh_from_baked_data(baked_data, bake_tangents);

	// 如果之前失败过，可能根本没有材质。
	if (materials.size() > 0) {
		for (unsigned int surface_index = 0; surface_index < baked_data.model.surface_count; ++surface_index) {
			const blocky::BakedModel::Surface &surface = baked_data.model.surfaces[surface_index];
			Ref<Material> material = materials[surface.material_id];
			Ref<Material> material_override = get_material_override(surface_index);
			if (material_override.is_valid()) {
				material = material_override;
			}
			mesh->surface_set_material(surface_index, material);
		}
	}

	return mesh;
}

void VoxelBlockyModelMesh::set_side_vertex_tolerance(float tolerance) {
	_side_vertex_tolerance = math::max(tolerance, 0.f);
}

float VoxelBlockyModelMesh::get_side_vertex_tolerance() const {
	return _side_vertex_tolerance;
}

void VoxelBlockyModelMesh::set_side_cutout_enabled(bool enabled) {
	_side_cutout_enabled = enabled;
}

bool VoxelBlockyModelMesh::is_side_cutout_enabled() const {
	return _side_cutout_enabled;
}

void VoxelBlockyModelMesh::_bind_methods() {
	using Self = VoxelBlockyModelMesh;

	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &Self::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &Self::get_mesh);

	ClassDB::bind_method(D_METHOD("set_side_vertex_tolerance", "tolerance"), &Self::set_side_vertex_tolerance);
	ClassDB::bind_method(D_METHOD("get_side_vertex_tolerance"), &Self::get_side_vertex_tolerance);

	ClassDB::bind_method(
			D_METHOD("set_side_cutout_enabled", "enabled"), &VoxelBlockyModelMesh::set_side_cutout_enabled
	);
	ClassDB::bind_method(D_METHOD("is_side_cutout_enabled"), &VoxelBlockyModelMesh::is_side_cutout_enabled);

	ADD_PROPERTY(
			PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, Mesh::get_class_static()),
			"set_mesh",
			"get_mesh"
	);
	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "mesh_ortho_rotation_index", PROPERTY_HINT_RANGE, "0,24"),
			"set_mesh_ortho_rotation_index",
			"get_mesh_ortho_rotation_index"
	);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "side_vertex_tolerance", PROPERTY_HINT_RANGE, "0,1,0.0001"),
			"set_side_vertex_tolerance",
			"get_side_vertex_tolerance"
	);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "side_cutout_enabled"), "set_side_cutout_enabled", "is_side_cutout_enabled"
	);
}

} // namespace voxel
