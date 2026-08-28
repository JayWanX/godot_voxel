#ifndef VOXEL_GODOT_GEOMETRY_2D_H
#define VOXEL_GODOT_GEOMETRY_2D_H

#if defined(VOXEL_GODOT)
#include <core/math/geometry_2d.h>
#endif

#include "../../containers/span.h"
#include "../../containers/std_vector.h"
#include "../../math/vector2i.h"
#include "../core/packed_vector2_array.h"

namespace voxel::godot {

void geometry_2d_make_atlas(Span<const Vector2i> p_sizes, StdVector<Vector2i> &r_result, Vector2i &r_size);

void geometry_2d_clip_polygons( //
		const PackedVector2Array &polygon_a, //
		const PackedVector2Array &polygon_b, //
		StdVector<PackedVector2Array> &output //
);

} // namespace voxel::godot

#endif // VOXEL_GODOT_GEOMETRY_2D_H
