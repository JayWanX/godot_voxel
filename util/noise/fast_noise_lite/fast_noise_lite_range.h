#ifndef FAST_NOISE_LITE_RANGE_H
#define FAST_NOISE_LITE_RANGE_H

#include "../../math/interval.h"

// Voxel_FastNoiseLite 的区间估计

namespace voxel {

class Voxel_FastNoiseLite;
class Voxel_FastNoiseLiteGradient;

math::Interval get_fnl_range_2d(const Voxel_FastNoiseLite &noise, math::Interval x, math::Interval y);
math::Interval get_fnl_range_3d(const Voxel_FastNoiseLite &noise, math::Interval x, math::Interval y, math::Interval z);
math::Interval2 get_fnl_gradient_range_2d(const Voxel_FastNoiseLiteGradient &noise, math::Interval x, math::Interval y);
math::Interval3 get_fnl_gradient_range_3d(
		const Voxel_FastNoiseLiteGradient &noise,
		math::Interval x,
		math::Interval y,
		math::Interval z
);

} // namespace voxel

#endif // FAST_NOISE_LITE_RANGE_H
