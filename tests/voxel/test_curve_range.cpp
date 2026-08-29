#include "test_curve_range.h"
#include "../../generators/graph/curve_utility.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/curve.h"
#include "../../util/testing/test_macros.h"

namespace voxel::tests {

void test_get_curve_monotonic_sections() {
	// 这个测试起来有点麻烦，因为 Curve 存在源自 bake() 函数的浮点精度问题
	struct L {
		static bool is_equal_approx(float a, float b) {
			return Math::is_equal_approx(a, b, 2.f * CURVE_RANGE_MARGIN);
		}
	};
	{
		// 一个上升的线段
		Ref<Curve> curve;
		curve.instantiate();
		curve->add_point(Vector2(0, 0));
		curve->add_point(Vector2(1, 1));
		StdVector<CurveMonotonicSection> sections;
		get_curve_monotonic_sections(**curve, sections);
		VOXEL_TEST_ASSERT(sections.size() == 1);
		VOXEL_TEST_ASSERT(sections[0].x_min == 0.f);
		VOXEL_TEST_ASSERT(sections[0].x_max == 1.f);
		VOXEL_TEST_ASSERT(sections[0].y_min == 0.f);
		VOXEL_TEST_ASSERT(sections[0].y_max == 1.f);
		{
			math::Interval yi = get_curve_range(**curve, sections, math::Interval(0.f, 1.f));
			VOXEL_TEST_ASSERT(L::is_equal_approx(yi.min, 0.f));
			VOXEL_TEST_ASSERT(L::is_equal_approx(yi.max, 1.f));
		}
		{
			math::Interval yi = get_curve_range(**curve, sections, math::Interval(-2.f, 2.f));
			VOXEL_TEST_ASSERT(L::is_equal_approx(yi.min, 0.f));
			VOXEL_TEST_ASSERT(L::is_equal_approx(yi.max, 1.f));
		}
		{
			math::Interval xi(0.2f, 0.8f);
			math::Interval yi = get_curve_range(**curve, sections, xi);
			math::Interval yi_expected(curve->sample_baked(xi.min), curve->sample_baked(xi.max));
			VOXEL_TEST_ASSERT(L::is_equal_approx(yi.min, yi_expected.min));
			VOXEL_TEST_ASSERT(L::is_equal_approx(yi.max, yi_expected.max));
		}
	}
	{
		// 一个平直的线段
		Ref<Curve> curve;
		curve.instantiate();
		curve->add_point(Vector2(0, 0));
		curve->add_point(Vector2(1, 0));
		StdVector<CurveMonotonicSection> sections;
		get_curve_monotonic_sections(**curve, sections);
		VOXEL_TEST_ASSERT(sections.size() == 1);
		VOXEL_TEST_ASSERT(sections[0].x_min == 0.f);
		VOXEL_TEST_ASSERT(sections[0].x_max == 1.f);
		VOXEL_TEST_ASSERT(sections[0].y_min == 0.f);
		VOXEL_TEST_ASSERT(sections[0].y_max == 0.f);
	}
	{
		// 两个线段：先上升，再平直
		Ref<Curve> curve;
		curve.instantiate();
		curve->add_point(Vector2(0, 0));
		curve->add_point(Vector2(0.5, 1));
		curve->add_point(Vector2(1, 1));
		StdVector<CurveMonotonicSection> sections;
		get_curve_monotonic_sections(**curve, sections);
		VOXEL_TEST_ASSERT(sections.size() == 1);
	}
	{
		// 两个线段：先平直，再上升
		Ref<Curve> curve;
		curve.instantiate();
		curve->add_point(Vector2(0, 0));
		curve->add_point(Vector2(0.5, 0));
		curve->add_point(Vector2(1, 1));
		StdVector<CurveMonotonicSection> sections;
		get_curve_monotonic_sections(**curve, sections);
		VOXEL_TEST_ASSERT(sections.size() == 1);
	}
	{
		// 三个线段：平直、上升、再平直
		Ref<Curve> curve;
		curve.instantiate();
		curve->add_point(Vector2(0, 0));
		curve->add_point(Vector2(0.3, 0));
		curve->add_point(Vector2(0.6, 1));
		curve->add_point(Vector2(1, 1));
		StdVector<CurveMonotonicSection> sections;
		get_curve_monotonic_sections(**curve, sections);
		VOXEL_TEST_ASSERT(sections.size() == 1);
	}
	{
		// 三个线段：上升、下降、上升
		Ref<Curve> curve;
		curve.instantiate();
		curve->add_point(Vector2(0, 0));
		curve->add_point(Vector2(0.3, 1));
		curve->add_point(Vector2(0.6, 0));
		curve->add_point(Vector2(1, 1));
		StdVector<CurveMonotonicSection> sections;
		get_curve_monotonic_sections(**curve, sections);
		VOXEL_TEST_ASSERT(sections.size() == 3);
		VOXEL_TEST_ASSERT(sections[0].x_min == 0.f);
		VOXEL_TEST_ASSERT(sections[2].x_max == 1.f);
	}
	{
		// 两个线段：先上升，再下降
		Ref<Curve> curve;
		curve.instantiate();
		curve->add_point(Vector2(0, 0));
		curve->add_point(Vector2(0.5, 1));
		curve->add_point(Vector2(1, 0));
		StdVector<CurveMonotonicSection> sections;
		get_curve_monotonic_sections(**curve, sections);
		VOXEL_TEST_ASSERT(sections.size() == 2);
	}
	{
		// 一个线段，形如抛物线，先上升后下降
		Ref<Curve> curve;
		curve.instantiate();
		curve->add_point(Vector2(0, 0), 0.f, 1.f);
		curve->add_point(Vector2(1, 0));
		StdVector<CurveMonotonicSection> sections;
		get_curve_monotonic_sections(**curve, sections);
		VOXEL_TEST_ASSERT(sections.size() == 2);
		VOXEL_TEST_ASSERT(sections[0].x_min == 0.f);
		VOXEL_TEST_ASSERT(sections[0].y_max >= 0.1f);
		VOXEL_TEST_ASSERT(sections[1].x_max == 1.f);
	}
}

} // namespace voxel::tests
