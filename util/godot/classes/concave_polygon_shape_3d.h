#ifndef VOXEL_GODOT_CONCAVE_POLYGON_SHAPE_3D_H
#define VOXEL_GODOT_CONCAVE_POLYGON_SHAPE_3D_H

#include "../../containers/span.h"
#include "../../macros.h"
#include "../../math/vector3f.h"

#include <core/version.h>

#include <scene/resources/3d/concave_polygon_shape_3d.h>


namespace voxel::godot {

// 将所有网格表面数组合并为一个碰撞体。
Ref<ConcavePolygonShape3D> create_concave_polygon_shape(const Span<const Array> surfaces);

Ref<ConcavePolygonShape3D> create_concave_polygon_shape(
		const Span<const Vector3f> positions,
		const Span<const int> indices
);

// 从网格表面的一个子区域（从 0 开始）创建形状。
Ref<ConcavePolygonShape3D> create_concave_polygon_shape(
		const Array &surface_arrays,
		const unsigned int vertex_count,
		const unsigned int index_count
);

} // namespace voxel::godot

#endif // VOXEL_GODOT_CONCAVE_POLYGON_SHAPE_3D_H
