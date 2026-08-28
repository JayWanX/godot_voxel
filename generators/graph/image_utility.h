#ifndef VOXEL_HEIGHTMAP_UTILITY_H
#define VOXEL_HEIGHTMAP_UTILITY_H

#include "../../util/godot/core/rect2i.h"
#include "../../util/godot/macros.h"
#include "../../util/math/interval.h"

VOXEL_GODOT_FORWARD_DECLARE(class Image);

namespace voxel {

math::Interval get_heightmap_range(const Image &im);
math::Interval get_heightmap_range(const Image &im, Rect2i rect);

} // namespace voxel

#endif // VOXEL_HEIGHTMAP_UTILITY_H
