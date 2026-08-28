#ifndef VOXEL_GODOT_AABB_H
#define VOXEL_GODOT_AABB_H

#if defined(VOXEL_GODOT)
#include <core/math/aabb.h>
#endif

namespace voxel {

class TextWriter;
TextWriter &operator<<(TextWriter &ss, const AABB &v);

inline real_t distance_squared(const AABB &aabb, const Vector3 p) {
	const Vector3 d = (aabb.position - p).max(p - (aabb.position + aabb.size)).max(Vector3());
	return d.length_squared();
}

} // namespace voxel

#endif // VOXEL_GODOT_AABB_H
