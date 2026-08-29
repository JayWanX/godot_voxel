#include "concave_polygon_shape_3d.h"
#include "../../math/conv.h"
#include "../../profiling.h"
#include "../core/packed_arrays.h"
#include "collision_shape_3d.h"
#include "mesh.h"

namespace voxel::godot {

Ref<ConcavePolygonShape3D> create_concave_polygon_shape(const Span<const Array> surfaces) {
	// 比 Mesh::create_trimesh_shape() 更快的版本，因为 `create_trimesh_shape` 在过程中会在内部创建
	// 一个 Trimesh，那非常慢

	VOXEL_PROFILE_SCOPE();

	PackedVector3Array face_points;
	int face_points_size = 0;

	// 找出 face_points 的正确大小
	for (unsigned int i = 0; i < surfaces.size(); i++) {
		const Array &surface_arrays = surfaces[i];
		if (surface_arrays.size() == 0) {
			// 该表面为空
			continue;
		}
		// 如果表面非空，则它必须含有预期数量的数据数组
		ERR_CONTINUE(surface_arrays.size() != Mesh::ARRAY_MAX);
		PackedInt32Array indices = surface_arrays[Mesh::ARRAY_INDEX];
		face_points_size += indices.size();
	}
	face_points.resize(face_points_size);

	if (face_points_size < 3) {
		return Ref<ConcavePolygonShape3D>();
	}

	// 将所有表面去索引化为一个
	unsigned int face_points_offset = 0;
	for (unsigned int i = 0; i < surfaces.size(); i++) {
		const Array &surface_arrays = surfaces[i];
		if (surface_arrays.size() == 0) {
			continue;
		}
		PackedVector3Array positions = surface_arrays[Mesh::ARRAY_VERTEX];
		PackedInt32Array indices = surface_arrays[Mesh::ARRAY_INDEX];

		ERR_FAIL_COND_V(positions.size() < 3, Ref<ConcavePolygonShape3D>());
		ERR_FAIL_COND_V(indices.size() < 3, Ref<ConcavePolygonShape3D>());
		ERR_FAIL_COND_V(indices.size() % 3 != 0, Ref<ConcavePolygonShape3D>());

		unsigned int face_points_count = face_points_offset + indices.size();

		{
			Vector3 *w = face_points.ptrw();
			const int *index_r = indices.ptr();
			const Vector3 *position_r = positions.ptr();

			for (unsigned int p = face_points_offset; p < face_points_count; ++p) {
				const int ii = p - face_points_offset;
#ifdef DEBUG_ENABLED
				CRASH_COND(ii < 0 || ii >= indices.size());
#endif
				const int index = index_r[ii];
#ifdef DEBUG_ENABLED
				CRASH_COND(index < 0 || index >= positions.size());
#endif
				w[p] = position_r[index];
			}
		}

		face_points_offset += indices.size();
	}

	Ref<ConcavePolygonShape3D> shape;
	{
		VOXEL_PROFILE_SCOPE_NAMED("Godot shape");
		shape.instantiate();
		shape->set_faces(face_points);
	}
	return shape;
}

PackedVector3Array deindex_mesh_to_packed_vector3_array(
		const Span<const Vector3f> positions,
		const Span<const int> indices
) {
	VOXEL_PROFILE_SCOPE();

	PackedVector3Array face_points;
	face_points.resize(indices.size());
	{
		Vector3 *w = face_points.ptrw();
		for (unsigned int ii = 0; ii < indices.size(); ++ii) {
			const int index = indices[ii];
			w[ii] = to_vec3(positions[index]);
		}
	}
	return face_points;
}

PackedVector3Array deindex_mesh_to_packed_vector3_array(
		const Span<const Vector3> vertices,
		const Span<const int32_t> indices
) {
	VOXEL_PROFILE_SCOPE();

	PackedVector3Array face_points;
	face_points.resize(indices.size());

	{
		Span<Vector3> dst(face_points.ptrw(), face_points.size());
		for (unsigned int ii = 0; ii < indices.size(); ++ii) {
			const int index = indices[ii];
			dst[ii] = vertices[index];
		}
	}

	return face_points;
}

Ref<ConcavePolygonShape3D> create_concave_polygon_shape(
		const Span<const Vector3f> positions,
		const Span<const int> indices
) {
	VOXEL_PROFILE_SCOPE();

	if (indices.size() < 3) {
		return Ref<ConcavePolygonShape3D>();
	}

	ERR_FAIL_COND_V(positions.size() < 3, Ref<ConcavePolygonShape3D>());
	ERR_FAIL_COND_V(indices.size() < 3, Ref<ConcavePolygonShape3D>());
	ERR_FAIL_COND_V(indices.size() % 3 != 0, Ref<ConcavePolygonShape3D>());

	const PackedVector3Array face_points = deindex_mesh_to_packed_vector3_array(positions, indices);

	Ref<ConcavePolygonShape3D> shape;
	{
		VOXEL_PROFILE_SCOPE_NAMED("Godot shape");
		shape.instantiate();
		shape->set_faces(face_points);
	}
	return shape;
}

// 此变体可以使用更少的索引数，从而用网格的一个子集来创建碰撞形状。
Ref<ConcavePolygonShape3D> create_concave_polygon_shape(
		const Array &surface_arrays,
		const unsigned int vertex_count,
		const unsigned int index_count
) {
	VOXEL_PROFILE_SCOPE();

	Ref<ConcavePolygonShape3D> shape;

	if (surface_arrays.size() == 0) {
		// Empty
		return shape;
	}
	VOXEL_ASSERT(surface_arrays.size() == Mesh::ARRAY_MAX);

	const PackedInt32Array indices = surface_arrays[Mesh::ARRAY_INDEX];
	ERR_FAIL_COND_V(index_count > static_cast<unsigned int>(indices.size()), shape);
	if (indices.size() < 3) {
		// Empty
		return shape;
	}

	const PackedVector3Array positions = surface_arrays[Mesh::ARRAY_VERTEX];
	ERR_FAIL_COND_V(vertex_count > static_cast<unsigned int>(positions.size()), shape);

	ERR_FAIL_COND_V(positions.size() < 3, shape);
	ERR_FAIL_COND_V(indices.size() < 3, shape);
	ERR_FAIL_COND_V(indices.size() % 3 != 0, shape);

	const PackedVector3Array face_points = deindex_mesh_to_packed_vector3_array(
			to_span(positions).sub(0, vertex_count), //
			to_span(indices).sub(0, index_count)
	);

	{
		VOXEL_PROFILE_SCOPE_NAMED("Godot shape");
		shape.instantiate();
		shape->set_faces(face_points);
	}
	return shape;
}

} // namespace voxel::godot
