#ifndef VOXEL_NOISE_RANGE_UTILITY_H
#define VOXEL_NOISE_RANGE_UTILITY_H

#include "../math/interval.h"

// 获取噪声区间估计的通用工具。
// 主要技术是采样中点，并用导数找出最大变化量。

// TODO 如果锚点位于波峰或波谷，我们可以使最大导数估计出现偏差，
// 因为在这些情况下，噪声不可能再往上或往下走更远

namespace voxel {

template <typename Noise_F>
inline math::Interval get_noise_range_2d(
		Noise_F noise_func,
		const math::Interval &x,
		const math::Interval &y,
		float max_derivative
) {
	// 从给定求值点向任意单位向量方向，最大差值是一个固定数。
	// 我们可以用这个数在我们矩形区间内找出一个边界范围。
	const float max_derivative_half_diagonal = 0.5f * max_derivative * math::SQRT2<float>;

	const real_t mid_x = 0.5 * (x.min + x.max);
	const real_t mid_y = 0.5 * (y.min + y.max);
	const float mid_value = noise_func(mid_x, mid_y);

	const real_t diag = Math::sqrt(math::squared(x.length()) + math::squared(y.length()));

	return math::Interval( //
			math::maxf(mid_value - max_derivative_half_diagonal * diag, -1),
			math::minf(mid_value + max_derivative_half_diagonal * diag, 1)
	);
}

template <typename Noise_F>
inline math::Interval get_noise_range_3d(
		Noise_F noise_func,
		const math::Interval &x,
		const math::Interval &y,
		const math::Interval &z,
		float max_derivative
) {
	const float max_derivative_half_diagonal = 0.5f * max_derivative * math::SQRT2<float>;

	const real_t mid_x = 0.5 * (x.min + x.max);
	const real_t mid_y = 0.5 * (y.min + y.max);
	const real_t mid_z = 0.5 * (z.min + z.max);
	const float mid_value = noise_func(mid_x, mid_y, mid_z);

	const real_t diag = Math::sqrt(math::squared(x.length()) + math::squared(y.length()) + math::squared(z.length()));

	return math::Interval( //
			math::maxf(mid_value - max_derivative_half_diagonal * diag, -1),
			math::minf(mid_value + max_derivative_half_diagonal * diag, 1)
	);
}

} // namespace voxel

#endif // VOXEL_NOISE_RANGE_UTILITY_H
