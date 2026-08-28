#ifndef FAST_NOISE_LITE_RANGE_H
#define FAST_NOISE_LITE_RANGE_H

#include "../../math/interval.h"

// Interval estimation for VOXEL_FastNoiseLite

namespace voxel {

class VOXEL_FastNoiseLite;
class VOXEL_FastNoiseLiteGradient;

math::Interval get_fnl_range_2d(const VOXEL_FastNoiseLite &noise, math::Interval x, math::Interval y);
math::Interval get_fnl_range_3d(const VOXEL_FastNoiseLite &noise, math::Interval x, math::Interval y, math::Interval z);
math::Interval2 get_fnl_gradient_range_2d(const VOXEL_FastNoiseLiteGradient &noise, math::Interval x, math::Interval y);
math::Interval3 get_fnl_gradient_range_3d(
		const VOXEL_FastNoiseLiteGradient &noise,
		math::Interval x,
		math::Interval y,
		math::Interval z
);

} // namespace voxel

#endif // FAST_NOISE_LITE_RANGE_H
