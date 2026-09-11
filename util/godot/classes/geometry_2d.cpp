#include "geometry_2d.h"
#include "../../math/conv.h"
#include "../../profiling.h"

namespace voxel::godot {

void geometry_2d_make_atlas(Span<const Vector2i> p_sizes, StdVector<Vector2i> &r_result, Vector2i &r_size) {
	VOXEL_PROFILE_SCOPE();


	Vector<Vector2i> sizes;
	sizes.resize(p_sizes.size());
	Vector2i *sizes_data = sizes.ptrw();
	VOXEL_ASSERT_RETURN(sizes_data != nullptr);
	memcpy(sizes_data, p_sizes.data(), p_sizes.size() * sizeof(Vector2i));

	Vector<Vector2i> result;
	Geometry2D::make_atlas(sizes, result, r_size);

	r_result.resize(result.size());
	memcpy(r_result.data(), result.ptr(), result.size() * sizeof(Vector2i));

}

void geometry_2d_clip_polygons( //
		const PackedVector2Array &polygon_a, //
		const PackedVector2Array &polygon_b, //
		StdVector<PackedVector2Array> &output //
) {
	Vector<Vector<Vector2>> result = Geometry2D::clip_polygons(polygon_a, polygon_b);
	output.resize(result.size());
	for (unsigned int i = 0; i < output.size(); ++i) {
		output[i] = result[i];
	}

}

} // namespace voxel::godot
