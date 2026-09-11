#include <core/math/math_funcs.h>
#ifndef VOXEL_MATH_FUNCS_H
#define VOXEL_MATH_FUNCS_H

#include "../errors.h"


#include "constants.h"
#include <cmath>
#include <type_traits>

namespace voxel::math {

// 通用数学函数，仅使用标量类型。

template <typename T>
inline constexpr T min(const T a, const T b) {
	// Godot 的向量类型带有 operator<，这意味着如果包含了错误的头文件，本函数可能被
	// 误当作逐分量的专用版本使用而不会报错。因此我们必须防范这种静默的
	// 二义性问题。
	static_assert(std::is_scalar<T>::value);
	return a < b ? a : b;
}

template <typename T>
inline constexpr T max(const T a, const T b) {
	static_assert(std::is_scalar<T>::value);
	return a > b ? a : b;
}

template <typename T>
inline T min(const T a, const T b, const T c) {
	static_assert(std::is_scalar<T>::value);
	return min(min(a, b), c);
}

template <typename T>
inline T max(const T a, const T b, const T c) {
	static_assert(std::is_scalar<T>::value);
	return max(max(a, b), c);
}

template <typename T>
inline T min(const T a, const T b, const T c, const T d) {
	static_assert(std::is_scalar<T>::value);
	return min(min(a, b), min(c, d));
}

template <typename T>
inline T max(const T a, const T b, const T c, const T d) {
	static_assert(std::is_scalar<T>::value);
	return max(max(a, b), max(c, d));
}

template <typename T>
inline T min(const T a, const T b, const T c, const T d, const T e, const T f) {
	static_assert(std::is_scalar<T>::value);
	return min(min(min(a, b), min(c, d)), min(e, f));
}

template <typename T>
inline T max(const T a, const T b, const T c, const T d, const T e, const T f) {
	static_assert(std::is_scalar<T>::value);
	return max(max(max(a, b), max(c, d)), max(e, f));
}

template <typename T>
inline T min(const T a, const T b, const T c, const T d, const T e, const T f, const T g, const T h) {
	static_assert(std::is_scalar<T>::value);
	return min(min(a, b, c, d), min(e, f, g, h));
}

template <typename T>
inline T max(const T a, const T b, const T c, const T d, const T e, const T f, const T g, const T h) {
	static_assert(std::is_scalar<T>::value);
	return max(max(a, b, c, d), max(e, f, g, h));
}

// 模板版本需要显式指定类型。
// float 版本无需始终强制转换，因此在使用 `real_t` 时，可选的 double 精度支持在
// 传入不同精度的参数时更方便。

inline float minf(float a, float b) {
	return a < b ? a : b;
}

inline double minf(double a, double b) {
	return a < b ? a : b;
}

inline float maxf(float a, float b) {
	return a > b ? a : b;
}

inline double maxf(double a, double b) {
	return a > b ? a : b;
}

template <typename T>
inline constexpr T clamp(const T x, const T min_value, const T max_value) {
	static_assert(std::is_scalar<T>::value);
	// TODO 强制 T 为数值类型
	return min(max(x, min_value), max_value);
}

inline float clampf(float x, float min_value, float max_value) {
	return min(max(x, min_value), max_value);
}

inline double clampf(double x, double min_value, double max_value) {
	return min(max(x, min_value), max_value);
}

template <typename T>
inline T squared(const T x) {
	return x * x;
}

template <typename T>
inline T cubed(const T x) {
	return x * x * x;
}

inline float lerp(float a, float b, float t) {
	return Math::lerp(a, b, t);
}

inline double lerp(double a, double b, double t) {
	return Math::lerp(a, b, t);
}

// 执行欧几里得除法，即向下取整除法。
// 本实现要求除数严格为正。
//
// 以除以 3 为例：
//
//    x   | `/` | floordiv | ceildiv
// ----------------------------------
//    -6  | -2  | -2       | -2
//    -5  | -1  | -2       | -1
//    -4  | -1  | -2       | -1
//    -3  | -1  | -1       | -1
//    -2  | 0   | -1       | 0
//    -1  | 0   | -1       | 0
//    0   | 0   | 0        | 0
//    1   | 0   | 0        | 1
//    2   | 0   | 0        | 1
//    3   | 1   | 1        | 1
//    4   | 1   | 1        | 2
//    5   | 1   | 1        | 2
//    6   | 2   | 2        | 2
inline int floordiv(int x, int d) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(d > 0);
#endif
	if (x < 0) {
		return (x - d + 1) / d;
	} else {
		return x / d;
	}
}

// ceildiv(0, 10) == 0
// ceildiv(1, 10) == 1
// ceildiv(5, 10) == 1
// ceildiv(10, 10) == 1
// ceildiv(11, 10) == 2
inline int ceildiv(int x, int d) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(d > 0);
#endif
	if (x > 0) {
		return (x + d - 1) / d;
	} else {
		return x / d;
	}
	// return -floordiv(-x, d);
}

inline int ceildiv(unsigned int x, unsigned int d) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(d > 0);
#endif
	return (x + d - 1) / d;
}

// TODO 重命名 `wrapi`
// `Math::wrapi`，最小值为 0
inline int wrap(int x, int d) {
#ifdef DEV_ENABLED
	VOXEL_ASSERT(d > 0);
#endif
	// return x % d; // 仅正数
	return ((x % d) + d) % d;
}

// `Math::wrapf`，最小值为 0
inline float wrapf(float x, float d) {
	return Math::is_zero_approx(d) ? 0.f : x - (d * Math::floor(x / d));
}

inline double wrapf(double x, double d) {
	return Math::is_zero_approx(d) ? 0.0 : x - (d * Math::floor(x / d));
}

// 类似于 Math::smoothstep，但不使用宏进行钳制
inline float smoothstep(float p_from, float p_to, float p_weight) {
	if (Math::is_equal_approx(p_from, p_to)) {
		return p_from;
	}
	float x = clamp((p_weight - p_from) / (p_to - p_from), 0.0f, 1.0f);
	return x * x * (3.0f - 2.0f * x);
}

inline double smoothstep(double p_from, double p_to, double p_weight) {
	if (Math::is_equal_approx(p_from, p_to)) {
		return p_from;
	}
	double x = clamp((p_weight - p_from) / (p_to - p_from), 0.0, 1.0);
	return x * x * (3.0 - 2.0 * x);
}

inline float fract(float x) {
	return x - Math::floor(x);
}

inline double fract(double x) {
	return x - Math::floor(x);
}

inline bool is_power_of_two(size_t x) {
	return x != 0 && (x & (x - 1)) == 0;
}

// 若 `x` 是 2 的幂，返回 `x`。
// 否则返回大于 `x` 且最接近 2 的幂。
inline unsigned int get_next_power_of_two_32(unsigned int x) {
	if (x == 0) {
		return 0;
	}
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return ++x;
}

// 查找小于某整数的前一个 2 的幂的函数。
inline unsigned int get_previous_power_of_two_32(unsigned int x) {
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x - (x >> 1);
}

// 假设 `pot == (1 << i)`，返回 `i`。
inline unsigned int get_shift_from_power_of_two_32(unsigned int pot) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(is_power_of_two(pot));
#endif
	for (unsigned int i = 0; i < 32; ++i) {
		if (pot == (1u << i)) {
			return i;
		}
	}
	VOXEL_CRASH_MSG("Input was not a valid power of two");
	return 0;
}

// 若 `num` == 2^N，返回 N。否则返回下一个 2 的幂的指数。
// 0 => 0
// 1 => 0
// 2 => 1
// 3 => 2
// 4 => 2
// 5 => 3
inline unsigned int get_next_power_of_two_32_shift(unsigned int num) {
	for (unsigned int i = 0; i < 32; ++i) {
		if (num <= (1u << i)) {
			return i;
		}
	}
	VOXEL_CRASH_MSG("Number too big");
	return 0;
}

// 若提供的地址 `a` 未按 `align` 指定的字节数对齐，
// 则返回下一个对齐地址。`align` 必须是 2 的幂。
inline size_t alignup(size_t a, size_t align) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(is_power_of_two(align));
#endif
	return (a + align - 1) & ~(align - 1);
}

// inline bool is_power_of_two(int i) {
// 	return i & (i - 1);
// }

// Math::snapped 的浮点版本；Godot 中该函数仅有 `double` 变体。
inline float snappedf(float p_value, float p_step) {
	if (p_step != 0) {
		p_value = Math::floor(p_value / p_step + 0.5f) * p_step;
	}
	return p_value;
}

template <typename T>
inline void sort(T &a, T &b) {
	if (a > b) {
		std::swap(a, b);
	}
}

template <typename T>
inline void sort(T &a, T &b, T &c) {
	sort(a, c);
	sort(a, b);
	sort(b, c);
}

template <typename T>
inline void sort(T &a, T &b, T &c, T &d) {
	sort(a, b);
	sort(c, d);
	sort(a, c);
	sort(b, d);
	sort(b, c);
}

template <typename TArray, typename TLess>
inline void sort2_array(TArray &array, TLess less) {
	if (less(array[1], array[0])) {
		std::swap(array[1], array[0]);
	}
}

template <typename TArray, typename TLess>
inline void sort3_array(TArray &array, TLess less) {
	if (less(array[1], array[0])) {
		std::swap(array[1], array[0]);
	}
	if (less(array[2], array[0])) {
		std::swap(array[2], array[0]);
	}
	if (less(array[2], array[1])) {
		std::swap(array[2], array[1]);
	}
}

template <typename TArray, typename TLess>
inline void sort4_array(TArray &array, TLess less) {
	if (less(array[1], array[0])) {
		std::swap(array[1], array[0]);
	}
	if (less(array[3], array[2])) {
		std::swap(array[3], array[2]);
	}
	if (less(array[2], array[0])) {
		std::swap(array[2], array[0]);
	}
	if (less(array[3], array[1])) {
		std::swap(array[3], array[1]);
	}
	if (less(array[2], array[1])) {
		std::swap(array[2], array[1]);
	}
}

// 若 `x` 为负返回 -1，否则返回 1。
// 与 GLSL 等常见版本不同，本函数在 `x` 为 0 时返回 1 而非 0。
template <typename T>
inline T sign_nonzero(T x) {
	return x < 0 ? -1 : 1;
}

template <typename T>
constexpr const T sign(const T v) {
	return v == 0 ? 0.0f : (v < 0 ? -1.0f : +1.0f);
}

// 对单位立方体角点值进行三线性插值。
// `v***` 参数为角点值，命名为 `vXYZ`，其中坐标在立方体上为 0 或 1。
// `p` 的坐标在 0..1 之间，但未被钳制，因此可以进行外推。
//
//      6---------------7
//     /|              /|
//    / |             / |
//   5---------------4  |
//   |  |            |  |
//   |  |            |  |
//   |  |            |  |
//   |  2------------|--3        Y
//   | /             | /         | Z
//   |/              |/          |/
//   1---------------0      X----o
//
// p000, p100, p101, p001, p010, p110, p111, p011
template <typename T, typename Vec3_T>
inline T interpolate_trilinear(
		const T v000,
		const T v100,
		const T v101,
		const T v001,
		const T v010,
		const T v110,
		const T v111,
		const T v011,
		Vec3_T p
) {
	//
	const T v00 = v000 + p.x * (v100 - v000);
	const T v10 = v010 + p.x * (v110 - v010);
	const T v01 = v001 + p.x * (v101 - v001);
	const T v11 = v011 + p.x * (v111 - v011);

	const T v0 = v00 + p.y * (v10 - v00);
	const T v1 = v01 + p.y * (v11 - v01);

	const T v = v0 + p.z * (v1 - v0);

	return v;
}

inline bool is_nan(float p_val) {
	return std::isnan(p_val);
}

inline bool is_nan(double p_val) {
	return std::isnan(p_val);
}

inline bool is_inf(float p_val) {
	return std::isinf(p_val);
}

inline bool is_inf(double p_val) {
	return std::isinf(p_val);
}

inline double deg_to_rad(double p_y) {
	return p_y * PI<double> / 180.0;
}

inline float deg_to_rad(float p_y) {
	return p_y * PI<float> / 180.f;
}

// a*x+b
struct LinearFuncParams {
	float a;
	float b;
};

// 给定源区间与目标区间，返回用于 `a*x+b` 公式以执行重映射的参数。
// 若源区间近似为空，返回零值。
inline LinearFuncParams remap_intervals_to_linear_params(float min0, float max0, float min1, float max1) {
	// min1 + (max1 - min1) * (x - min0) / (max0 - min0)
	// min1 + (max1 - min1) * (x - min0) * (1/(max0 - min0))
	// min1 +       A       * (x - min0) *        B
	// min1 + A * B * (x - min0)
	// min1 + A * B * x - A * B * min0
	// min1 +   C   * x -   C   * min0
	// min1 - C * min0 + C * x
	// (min1 - C * min0) + C * x
	//         b         + a * x
	// a * x + b
	if (Math::is_equal_approx(max0, min0)) {
		return { 0.f, 0.f };
	}
	const float a = (max1 - min1) / (max0 - min0);
	const float b = min1 - a * min0;
	return { a, b };
}

// 右移运算符 `>>` 的结果在 C++20 之前由实现定义，C++20 起执行算术
// 移位。本函数显式处理 C++20 之前可能出现的兼容问题。
// https://en.cppreference.com/w/cpp/language/operator_arithmetic#Built-in_bitwise_shift_operators
inline constexpr int32_t arithmetic_rshift(int32_t a, unsigned int b) {
	// MSVC 将右移记录为算术移位。
	// https://learn.microsoft.com/en-us/cpp/cpp/left-shift-and-right-shift-operators-input-and-output?view=msvc-170#right-shifts

	// GCC 将右移记录为算术移位。
	// https://gcc.gnu.org/onlinedocs/gcc-13.1.0/gcc/Integers-implementation.html

	static_assert(-4 >> 1 == -2, "Signed right-shift is not arithmetic, patch needed to support current compiler.");

	return a >> b;
}

template <unsigned int NBits>
inline constexpr int32_t sign_extend_to_32bit(int32_t i) {
	static_assert(NBits > 0 && NBits < 32);
	struct S {
		int32_t v : NBits;
	};
	return S{ i }.v;
}

template <typename T>
inline T abs(T a) {
	return Math::abs(a);
}

template <typename T>
inline T lerp(T a, T b, T t) {
	static_assert(std::is_floating_point<T>::value);
	return Math::lerp(a, b, t);
}

template <typename T>
inline T sqrt(T x) {
	static_assert(std::is_floating_point<T>::value);
	return Math::sqrt(x);
}

template <typename T>
inline T sin(T x) {
	static_assert(std::is_floating_point<T>::value);
	return Math::sin(x);
}

template <typename T>
inline T cos(T x) {
	static_assert(std::is_floating_point<T>::value);
	return Math::cos(x);
}

template <typename T>
inline T atan(T x) {
	static_assert(std::is_floating_point<T>::value);
	return Math::atan(x);
}

template <typename T>
inline T atan2(T y, T x) {
	static_assert(std::is_floating_point<T>::value);
	return Math::atan2(y, x);
}

template <typename T>
inline T floor(T x) {
	static_assert(std::is_floating_point<T>::value);
	return Math::floor(x);
}

template <typename T>
inline T pow(T x, T y) {
	static_assert(std::is_floating_point<T>::value);
	return Math::pow(x, y);
}

inline uint64_t multiply_check_overflow_u64(const uint64_t a, const uint64_t b) {
	const uint64_t r = a * b;
#ifdef DEV_ENABLED
	if (a != 0 && r / a != b) {
		VOXEL_PRINT_ERROR("Multiplication overflow");
		return 0;
	}
#endif
	return r;
}

} // namespace voxel::math

#endif // VOXEL_MATH_FUNCS_H
