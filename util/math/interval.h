#ifndef VOXEL_INTERVAL_H
#define VOXEL_INTERVAL_H

#include "funcs.h"
#include <limits>
#include <type_traits>

namespace voxel {
namespace math {

namespace interval_impl {
void check_range_once(float min, float max);
void check_range_once(double min, double max);
} // namespace interval_impl

// 用于区间运算
template <typename T>
struct IntervalT {
	// TODO 我希望该结构体仅用于浮点数，但 `static_assert` 会破坏数学函数的
	// 重载解析。例如，调用 `clamp<int>(int, int, int)` 会触发 `static_assert`，因为编译器
	// 会尝试匹配 `clamp<int>(Interval<int>, Interval<int>, Interval<int>)`，而它非但没有被跳过，
	// 反而导致编译报错中断。有什么更好的办法吗？
	//
	// static_assert(std::is_floating_point<T>::value);

	// 两端均包含
	T min;
	T max;

	inline IntervalT() : min(0), max(0) {}

	inline IntervalT(T p_min, T p_max) : min(p_min), max(p_max) {
#if DEV_ENABLED
		VOXEL_ASSERT(p_min <= p_max);
#elif DEBUG_ENABLED
		// 不崩溃，但继续发出信号
		interval_impl::check_range_once(p_min, p_max);
#endif
	}

	inline static IntervalT from_single_value(T p_val) {
		return IntervalT(p_val, p_val);
	}

	inline static IntervalT from_unordered_values(T a, T b) {
		return IntervalT(math::min(a, b), math::max(a, b));
	}

	inline static IntervalT from_infinity() {
		return IntervalT(-std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity());
	}

	inline static IntervalT from_union(const IntervalT a, const IntervalT b) {
		return IntervalT(math::min(a.min, b.min), math::max(a.max, b.max));
	}

	inline bool contains(T v) const {
		return v >= min && v <= max;
	}

	inline bool contains(IntervalT other) const {
		return other.min >= min && other.max <= max;
	}

	inline bool is_single_value() const {
		return min == max;
	}

	inline bool is_valid() const {
		return min <= max;
	}

	inline void add_point(T x) {
		if (x < min) {
			min = x;
		} else if (x > max) {
			max = x;
		}
	}

	inline IntervalT padded(T e) const {
		return IntervalT(min - e, max + e);
	}

	inline void add_interval(IntervalT other) {
		add_point(other.min);
		add_point(other.max);
	}

	inline T length() const {
		return max - min;
	}

	inline bool operator==(const IntervalT &other) const {
		return min == other.min && max == other.max;
	}

	inline bool operator!=(const IntervalT &other) const {
		return min != other.min || max != other.max;
	}

	inline IntervalT operator+(T x) const {
		return IntervalT{ min + x, max + x };
	}

	inline IntervalT operator+(const IntervalT &other) const {
		return IntervalT{ min + other.min, max + other.max };
	}

	inline void operator+=(const IntervalT &other) {
		*this = *this + other;
	}

	inline IntervalT operator-(T x) const {
		return IntervalT{ min - x, max - x };
	}

	inline IntervalT operator-(const IntervalT &other) const {
		return IntervalT{ min - other.max, max - other.min };
	}

	inline IntervalT operator-() const {
		return IntervalT{ -max, -min };
	}

	inline IntervalT operator*(T x) const {
		const T a = min * x;
		const T b = max * x;
		if (a < b) {
			return IntervalT(a, b);
		} else {
			return IntervalT(b, a);
		}
	}

	inline IntervalT operator*(const IntervalT &other) const {
		// 注意，若两个操作数来源相同（即你在计算 x^2），可能导致结果不够精确。
		// 此时你可能更希望使用更专用的函数。
		const T a = min * other.min;
		const T b = min * other.max;
		const T c = max * other.min;
		const T d = max * other.max;
		return IntervalT{ math::min(a, b, c, d), math::max(a, b, c, d) };
	}

	inline void operator*=(T x) {
		*this = *this * x;
	}

	inline void operator*=(IntervalT x) {
		*this = *this * x;
	}

	inline IntervalT operator/(const IntervalT &other) const {
		if (other.is_single_value() && other.min == 0) {
			// 除以零。在 Voxel 图中，我们返回 0。
			return IntervalT::from_single_value(0);
		}
		if (other.contains(0.f)) {
			// TODO 可能需要更精确的实现
			return IntervalT::from_infinity();
		}
		const T a = min / other.min;
		const T b = min / other.max;
		const T c = max / other.min;
		const T d = max / other.max;
		return IntervalT{ math::min(a, b, c, d), math::max(a, b, c, d) };
	}

	inline IntervalT operator/(T x) const {
		// TODO 实现正确的区间除法
		return *this * (T(1.0) / x);
	}

	inline void operator/=(T x) {
		*this = *this / x;
	}
};

template <typename T>
struct Interval2T {
	IntervalT<T> x;
	IntervalT<T> y;
};

template <typename T>
struct Interval3T {
	IntervalT<T> x;
	IntervalT<T> y;
	IntervalT<T> z;
};

// TODO 在图相关代码中使用 float
using Interval = IntervalT<real_t>;
using Interval2 = Interval2T<real_t>;
using Interval3 = Interval3T<real_t>;

template <typename T>
inline IntervalT<T> operator*(float b, const IntervalT<T> &a) {
	return a * b;
}

// 在外部声明函数，这样使用区间或数值可以共用同一套代码（可模板化）

template <typename T>
inline IntervalT<T> min_interval(const IntervalT<T> &a, const IntervalT<T> &b) {
	return IntervalT<T>(min(a.min, b.min), min(a.max, b.max));
}

template <typename T>
inline IntervalT<T> max_interval(const IntervalT<T> &a, const IntervalT<T> &b) {
	return IntervalT<T>(max(a.min, b.min), max(a.max, b.max));
}

template <typename T>
inline IntervalT<T> min_interval(const IntervalT<T> &a, const T b) {
	return IntervalT<T>(min(a.min, b), min(a.max, b));
}

template <typename T>
inline IntervalT<T> max_interval(const IntervalT<T> &a, const T b) {
	return IntervalT<T>(max(a.min, b), max(a.max, b));
}

template <typename T>
inline IntervalT<T> sqrt(const IntervalT<T> &i) {
	// 避免使用负数，因为负数无定义；同时因为 VoxelGeneratorGraph 将其 SQRT 节点定义为
	// way.
	// TODO 是否将函数 `sqrt_or_zero` 改名以明确这一点？
	return IntervalT<T>{ sqrt(max<T>(0, i.min)), sqrt(max<T>(0, i.max)) };
}

template <typename T>
inline IntervalT<T> abs(const IntervalT<T> &i) {
	return IntervalT<T>{ i.contains(0) ? 0 : min(abs(i.min), abs(i.max)), max(abs(i.min), abs(i.max)) };
}

template <typename T>
inline IntervalT<T> clamp(const IntervalT<T> &i, const IntervalT<T> &p_min, const IntervalT<T> &p_max) {
	if (p_min.is_single_value() && p_max.is_single_value()) {
		return { clamp(i.min, p_min.min, p_max.min), clamp(i.max, p_min.min, p_max.min) };
	}
	if (i.min >= p_min.max && i.max <= p_max.min) {
		return i;
	}
	if (i.min >= p_max.max) {
		return IntervalT<T>::from_single_value(p_max.max);
	}
	if (i.max <= p_min.min) {
		return IntervalT<T>::from_single_value(p_min.min);
	}
	return IntervalT<T>(p_min.min, p_max.max);
}

template <typename T>
inline IntervalT<T> lerp(const IntervalT<T> &a, const IntervalT<T> &b, const IntervalT<T> &t) {
	if (t.is_single_value()) {
		return IntervalT<T>(lerp(a.min, b.min, t.min), lerp(a.max, b.max, t.min));
	}

	// TODO 是否可以直接写标量的 `lerp` 调用？
	const T v0 = a.min + t.min * (b.min - a.min);
	const T v1 = a.max + t.min * (b.min - a.max);
	const T v2 = a.min + t.max * (b.min - a.min);
	const T v3 = a.max + t.max * (b.min - a.max);
	const T v4 = a.min + t.min * (b.max - a.min);
	const T v5 = a.max + t.min * (b.max - a.max);
	const T v6 = a.min + t.max * (b.max - a.min);
	const T v7 = a.max + t.max * (b.max - a.max);

	return IntervalT<T>(min(v0, v1, v2, v3, v4, v5, v6, v7), max(v0, v1, v2, v3, v4, v5, v6, v7));
}

template <typename T>
inline IntervalT<T> sin(const IntervalT<T> &i) {
	if (i.is_single_value()) {
		return IntervalT<T>::from_single_value(sin(i.min));
	} else {
		// TODO 需要更高精度
		// 简化版
		return IntervalT<T>(-1, 1);
	}
}

template <typename T>
inline IntervalT<T> atan(const IntervalT<T> &t) {
	if (t.is_single_value()) {
		return IntervalT<T>::from_single_value(atan(t.min));
	}
	// arctan 是单调的
	return IntervalT<T>{ atan(t.min), atan(t.max) };
}

template <typename T>
struct OptionalIntervalT {
	IntervalT<T> value;
	bool valid = false;
};

using OptionalInterval = OptionalIntervalT<real_t>;

template <typename T>
inline IntervalT<T> atan2(const IntervalT<T> &y, const IntervalT<T> &x, OptionalIntervalT<T> *secondary_output) {
	if (y.is_single_value() && x.is_single_value()) {
		return Interval::from_single_value(atan2(y.min, x.max));
	}

	//          y
	//      1   |    0
	//          |
	//    ------o------x
	//          |
	//      2   |    3

	const bool in_nx = x.min <= 0;
	const bool in_px = x.max >= 0;
	const bool in_ny = y.min <= 0;
	const bool in_py = y.max >= 0;

	if (secondary_output != nullptr) {
		secondary_output->valid = false;
	}

	if (in_nx && in_px && in_ny && in_py) {
		// 所有象限
		return IntervalT<T>{ -PI<T>, PI<T> };
	}

	const bool in_q0 = in_px && in_py;
	const bool in_q1 = in_nx && in_py;
	const bool in_q2 = in_nx && in_ny;
	const bool in_q3 = in_px && in_ny;

	// 双象限

	if (in_q0 && in_q1) {
		return IntervalT<T>(atan2(y.min, x.max), atan2(y.min, x.min));
	}
	if (in_q1 && in_q2) {
		if (secondary_output == nullptr) {
			// 当跨越这两个象限时，角度从 PI 回绕到 -PI。
			// 我们本应把区间拆分为两段，但只能返回一个。
			// 为保证正确性，我们必须返回完整的范围……
			return IntervalT<T>{ -PI<T>, PI<T> };
		} else {
			// 但有时我们可以承担将区间拆分，
			// 尤其是当我们的使用场景随后又将其合并为一个时。
			// Q1
			secondary_output->value = IntervalT<T>(atan2(y.max, x.max), PI<T>);
			secondary_output->valid = true;
			// Q2
			return IntervalT<T>(-PI<T>, atan2(y.min, x.max));
		}
	}
	if (in_q2 && in_q3) {
		return IntervalT<T>(atan2(y.max, x.min), atan2(y.max, x.max));
	}
	if (in_q3 && in_q0) {
		return IntervalT<T>(atan2(y.min, x.min), atan2(y.max, x.min));
	}

	// 单个象限

	if (in_q0) {
		return IntervalT<T>(atan2(y.min, x.max), atan2(y.max, x.min));
	}
	if (in_q1) {
		return IntervalT<T>(atan2(y.max, x.max), atan2(y.min, x.min));
	}
	if (in_q2) {
		return IntervalT<T>(atan2(y.max, x.min), atan2(y.min, x.max));
	}
	if (in_q3) {
		return IntervalT<T>(atan2(y.min, x.min), atan2(y.max, x.max));
	}

	// Bwarf.
	return IntervalT<T>{ -math::PI<T>, math::PI<T> };
}

template <typename T>
inline IntervalT<T> floor(const IntervalT<T> &i) {
	// Floor 是单调的，所以我想我们可以直接这么做？
	return IntervalT<T>(floor(i.min), floor(i.max));
}

template <typename T>
inline IntervalT<T> round(const IntervalT<T> &i) {
	// Floor 是单调的，所以我想我们可以直接这么做？
	return IntervalT<T>(floor(i.min + T(0.5)), floor(i.max + T(0.5)));
}

template <typename T>
inline IntervalT<T> snapped(const IntervalT<T> &p_value, const IntervalT<T> &p_step) {
	// TODO 除以零返回 0，与 Godot 的 stepify 不同。可能需修改
	return floor(p_value / p_step + IntervalT<T>::from_single_value(T(0.5))) * p_step;
}

template <typename T>
inline IntervalT<T> wrapf(const IntervalT<T> &x, const IntervalT<T> &d) {
	return x - (d * floor(x / d));
}

template <typename T>
inline IntervalT<T> smoothstep(const T p_from, const T p_to, const IntervalT<T> p_weight) {
	if (Math::is_equal_approx(p_from, p_to)) {
		return IntervalT<T>::from_single_value(p_from);
	}
	// Smoothstep 是单调的
	const T v0 = smoothstep(p_from, p_to, p_weight.min);
	const T v1 = smoothstep(p_from, p_to, p_weight.max);
	if (v0 <= v1) {
		return IntervalT<T>(v0, v1);
	} else {
		return IntervalT<T>(v1, v0);
	}
}

// 优先使用此函数而非 x*x，可获得更优结果
template <typename T>
inline IntervalT<T> squared(const IntervalT<T> &x) {
	if (x.min < 0 && x.max > 0) {
		// 区间包含 0
		return IntervalT<T>{ T(0), max(x.min * x.min, x.max * x.max) };
	}
	// 区间仅位于抛物线的一侧
	if (x.max <= 0) {
		// 负侧：单调递减
		return IntervalT<T>{ x.max * x.max, x.min * x.min };
	} else {
		// 正侧：单调递增
		return IntervalT<T>{ x.min * x.min, x.max * x.max };
	}
}

// 优先使用此函数而非用单个区间做多项式，可获得更优结果
template <typename T>
inline IntervalT<T> polynomial_second_degree(const IntervalT<T> x, T a, T b, T c) {
	// a*x*x + b*x + c

	if (a == 0) {
		if (b == 0) {
			return IntervalT<T>::from_single_value(c);
		} else {
			return b * x + c;
		}
	}

	const T parabola_x = -b / (T(2) * a);

	const T y0 = a * x.min * x.min + b * x.min + c;
	const T y1 = a * x.max * x.max + b * x.max + c;

	if (x.min < parabola_x && x.max > parabola_x) {
		// 区间包含顶点
		const T parabola_y = a * parabola_x * parabola_x + b * parabola_x + c;
		if (a < 0) {
			return IntervalT<T>(min(y0, y1), parabola_y);
		} else {
			return IntervalT<T>(parabola_y, max(y0, y1));
		}
	}
	// 区间仅位于抛物线的一侧
	if ((a >= 0 && x.min >= parabola_x) || (a < 0 && x.max < parabola_x)) {
		// 单调递增
		return IntervalT<T>(y0, y1);
	} else {
		// 单调递减
		return IntervalT<T>(y1, y0);
	}
}

// 优先使用此函数而非 x*x*x，可获得更优结果
template <typename T>
inline IntervalT<T> cubed(const IntervalT<T> &x) {
	// x^3 单调递增
	const T minv = x.min * x.min * x.min;
	const T maxv = x.max * x.max * x.max;
	return IntervalT<T>{ minv, maxv };
}

template <typename T>
inline IntervalT<T> get_length(const IntervalT<T> &x, const IntervalT<T> &y) {
	return sqrt(squared(x) + squared(y));
}

template <typename T>
inline IntervalT<T> get_length(const IntervalT<T> &x, const IntervalT<T> &y, const IntervalT<T> &z) {
	return sqrt(squared(x) + squared(y) + squared(z));
}

template <typename T>
inline IntervalT<T> powi(IntervalT<T> x, int pi) {
	const T pf = pi;
	if (pi >= 0) {
		if (pi % 2 == 1) {
			// 正奇数次幂：递增
			return IntervalT<T>{ pow(x.min, pf), pow(x.max, pf) };
		} else {
			// 正偶数次幂：抛物线
			if (x.min < 0 && x.max > 0) {
				// 区间包含 0
				return IntervalT<T>{ 0, max(pow(x.min, pf), pow(x.max, pf)) };
			}
			// 区间仅位于抛物线的一侧
			if (x.max <= 0) {
				// 负侧：单调递减
				return IntervalT<T>{ pow(x.max, pf), pow(x.min, pf) };
			} else {
				// 正侧：单调递增
				return IntervalT<T>{ pow(x.min, pf), pow(x.max, pf) };
			}
		}
	} else {
		// TODO 负整数次幂
		return IntervalT<T>::from_infinity();
	}
}

template <typename T>
inline IntervalT<T> pow(IntervalT<T> x, float pf) {
	const int pi = pf;
	if (Math::is_equal_approx(pi, pf)) {
		return powi(x, pi);
	} else {
		// TODO 小数次幂
		return IntervalT<T>::from_infinity();
	}
}

template <typename T>
inline IntervalT<T> pow(IntervalT<T> x, IntervalT<T> p) {
	if (p.is_single_value()) {
		return pow(x, p.min);
	} else {
		// TODO 可变次幂
		return IntervalT<T>::from_infinity();
	}
}

} // namespace math

class TextWriter;
TextWriter &operator<<(TextWriter &ss, const math::Interval &v);

} // namespace voxel

#endif // VOXEL_INTERVAL_H
