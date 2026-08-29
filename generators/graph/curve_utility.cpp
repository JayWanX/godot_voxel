#include "curve_utility.h"
#include "../../util/godot/classes/curve.h"
#include "../../util/math/vector2i.h"
#include "../../util/string/format.h"

namespace voxel {

using namespace math;

void get_curve_monotonic_sections(Curve &curve, StdVector<CurveMonotonicSection> &sections) {
	const Interval curve_domain = voxel::godot::get_curve_domain(curve);
	const float curve_domain_range = curve_domain.length();

	const int res = curve.get_bake_resolution();
	float prev_y = curve.sample_baked(curve_domain.min);

	sections.clear();
	CurveMonotonicSection section;
	section.x_min = curve_domain.min;
	section.y_min = curve.sample_baked(curve_domain.min);

	float prev_x = 0.f;
	bool current_stationary = true;
	bool current_increasing = false;

	// 迭代到包含 `res` 为止，以包含最后一个值（Godot 的 PR #76617 修复了 Curve 的一个问题，
	// 同时也让我们注意到我们的代码没有正确包含曲线的末端）
	for (int i = 1; i < res; ++i) {
		// 使用 -1 是因为 [res-1] 是烘焙数组中的最后一个值，因此 `x` 必须为 1
		const float x = curve_domain.min + curve_domain_range * static_cast<float>(i) / (res - 1);
		const float y = curve.sample_baked(x);
		// 曲线有时看起来是平的，但由于 bake() 期间产生的浮点精度误差，它仍会以极小的幅度振荡。
		// 尝试通过将误差考虑在内来规避该问题
		const bool increasing = y > prev_y + CURVE_RANGE_MARGIN;
		const bool decreasing = y < prev_y - CURVE_RANGE_MARGIN;
		const bool stationary = increasing == false && decreasing == false;

		if (current_stationary) {
			current_stationary = stationary;
			current_increasing = increasing;

		} else if (i > 1 && !stationary && increasing != current_increasing) {
			section.x_max = prev_x;
			section.y_max = prev_y;
			sections.push_back(section);

			section.x_min = prev_x;
			section.y_min = prev_y;
			current_increasing = increasing;
			// 注意，`current_stationary` 不会再变为 true，因为我们只关心变化的区间。
			// 如果曲线的某一部分变为静止，它会被包含在当前区间内，直到该部分开始递增或递减。
		}

		prev_x = x;
		prev_y = y;
	}

	// 强制为最大值，因为迭代不会进行到 `res`
	section.x_max = curve_domain.max;
	section.y_max = prev_y;
	sections.push_back(section);
}

Interval get_curve_range(Curve &curve, const StdVector<CurveMonotonicSection> &sections, Interval x) {
	// 这个实现是线性的。它假设曲线通常不会有太多点。
	// 如果曲线有太多点，我们可以考虑动态选择不同的算法。
	Interval y;
	unsigned int i = 0;
	const float x_min = sections[0].x_min;
	if (x.min < x_min) {
		// X 范围起始于曲线最小 X 之前
		y = Interval::from_single_value(curve.sample_baked(x_min));
	} else {
		// 找到范围起始所在的区间
		for (; i < sections.size(); ++i) {
			const CurveMonotonicSection &section = sections[i];
			if (x.min >= section.x_min) {
				const float begin_y = curve.sample_baked(x.min);
				if (x.max < section.x_max) {
					// X 范围在该区间内起始并结束
					return Interval::from_unordered_values(begin_y, curve.sample_baked(x.max))
							.padded(CURVE_RANGE_MARGIN);
				} else {
					// X 范围在该区间内起始，并在它之后继续。
					// 需要从这里开始继续迭代
					y = Interval::from_unordered_values(begin_y, curve.sample_baked(section.x_max));
					++i;
					break;
				}
			}
		}
	}
	for (; i < sections.size(); ++i) {
		const CurveMonotonicSection &section = sections[i];
		if (x.max >= section.x_max) {
			// X 范围覆盖整个区间，也许还有之后的更多部分
			y.add_interval(Interval::from_unordered_values(section.y_min, section.y_max));
		} else {
			// X 范围在该区间内结束
			y.add_interval(Interval::from_unordered_values(section.y_min, curve.sample_baked(x.max)));
			break;
		}
	}
	return y.padded(CURVE_RANGE_MARGIN);
}

Interval get_curve_range(Curve &curve, bool &is_monotonic_increasing) {
	// TODO 如果能有缓存直接使用就好了
	const int res = curve.get_bake_resolution();
	Interval range;
	const Interval curve_domain = voxel::godot::get_curve_domain(curve);
	const float curve_domain_range = curve_domain.length();
	float prev_v = curve.sample_baked(curve_domain.min);
	if (curve.sample_baked(curve_domain.max) > prev_v) {
		is_monotonic_increasing = true;
	}
	for (int i = 0; i < res; ++i) {
		const float a = curve_domain.min + curve_domain_range * static_cast<float>(i) / res;
		const float v = curve.sample_baked(a);
		range.add_point(v);
		if (v < prev_v) {
			is_monotonic_increasing = false;
		}
		prev_v = v;
	}
	return range;
}

} // namespace voxel
