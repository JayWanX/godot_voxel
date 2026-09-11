#ifndef VOXEL_CURVE_UTILITY_H
#define VOXEL_CURVE_UTILITY_H

#include "../../util/containers/std_vector.h"
#include "../../util/godot/core/rect2i.h"
#include "../../util/godot/macros.h"
#include "../../util/math/interval.h"

class Curve;

namespace voxel {

struct CurveMonotonicSection {
	float x_min;
	float x_max;
	// 注意：Y 值不一定按递增顺序排列。
	// 它们的命名仅表示与 X 坐标对应。
	float y_min;
	float y_max;
};

struct CurveRangeData {
	StdVector<CurveMonotonicSection> sections;
};

static const float CURVE_RANGE_MARGIN = CMP_EPSILON;

// 以烘焙分辨率收集曲线的单调区间。
// 在一个区间内，曲线只具有以下属性之一：
// - 保持不变或递减
// - 保持不变或递增
// 这意味着，在一个区间内，给定由最小值和最大值定义的输入值范围，
// 我们只需在两端采样曲线，就能快速计算出准确的输出值范围。
void get_curve_monotonic_sections(Curve &curve, StdVector<CurveMonotonicSection> &sections);
// 获取曲线上某个 X 值范围对应的 Y 值范围，使用预计算的单调区间
math::Interval get_curve_range(Curve &curve, const StdVector<CurveMonotonicSection> &sections, math::Interval x);

// 旧版
math::Interval get_curve_range(Curve &curve, bool &is_monotonic_increasing);

} // namespace voxel

#endif // VOXEL_CURVE_UTILITY_H
