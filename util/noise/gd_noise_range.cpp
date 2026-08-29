#include "gd_noise_range.h"

namespace voxel {

// 对 Godot 的 `FastNoiseLite` 无法做同等程度的范围分析，因为它没有暴露
// FastNoiseLite 的内部实例，而官方库也没有暴露一些内部细节。
// 也许我们可以创建我们自己版本的一个临时实例，然后借用它？

math::Interval get_range_2d(const Noise &noise, math::Interval x, math::Interval y) {
	// TODO 实现 Godot 的 Noise 范围分析
	return { -1.f, 1.f };
}

math::Interval get_range_3d(const Noise &noise, math::Interval x, math::Interval y, math::Interval z) {
	// TODO 实现 Godot 的 Noise 范围分析
	return { -1.f, 1.f };
}

} // namespace voxel
