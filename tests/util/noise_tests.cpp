#include "../../util/godot/classes/image.h"
#include "../../util/godot/core/array.h"
#include "../../util/godot/core/print_string.h"
#include "../../util/godot/core/random_pcg.h"
#include "../../util/io/log.h"
#include "../../util/math/funcs.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite.h"
#include "../../util/string/format.h"
#include "../tests.h"

namespace voxel::noise_tests {

const int ITERATIONS = 1000000;
const int STEP_RESOLUTION_COUNT = 100;
const double STEP_MIN = 0.0001;
const double STEP_MAX = 0.01;

enum Tests { //
	TEST_MIN_MAX = 1,
	TEST_DERIVATIVES = 2
};

// 在给定步长范围内采样最大变化量。
// 为了性能考虑，结果未做归一化。
template <typename F2, typename FloatT>
FloatT get_derivative(FloatT x, FloatT y, FloatT step, F2 noise_func_2d) {
	FloatT n0, n1, d;
	FloatT max_derivative = 0.0;

	n0 = noise_func_2d(x, y);

	n1 = noise_func_2d(x + step, y);
	d = Math::abs(n1 - n0);
	if (d > max_derivative) {
		max_derivative = d;
	}

	n1 = noise_func_2d(x, y + step);
	d = Math::abs(n1 - n0);
	if (d > max_derivative) {
		max_derivative = d;
	}

	return max_derivative;
}

template <typename F3, typename FloatT>
FloatT get_derivative(FloatT x, FloatT y, FloatT z, FloatT step, F3 noise_func_3d) {
	FloatT n0, n1, d;
	FloatT max_derivative = 0.0;

	n0 = noise_func_3d(x, y, z);

	n1 = noise_func_3d(x + step, y, z);
	d = Math::abs(n1 - n0);
	if (d > max_derivative) {
		max_derivative = d;
	}

	n1 = noise_func_3d(x, y + step, z);
	d = Math::abs(n1 - n0);
	if (d > max_derivative) {
		max_derivative = d;
	}

	n1 = noise_func_3d(x, y, z + step);
	d = Math::abs(n1 - n0);
	if (d > max_derivative) {
		max_derivative = d;
	}

	return max_derivative;
}

template <typename F2, typename F3, typename FloatT>
void test_min_max(F2 noise_func_2d, F3 noise_func_3d, RandomPCG &rng) {
	FloatT min_value_2d = std::numeric_limits<FloatT>::max();
	FloatT max_value_2d = std::numeric_limits<FloatT>::min();

	FloatT min_value_3d = std::numeric_limits<FloatT>::max();
	FloatT max_value_3d = std::numeric_limits<FloatT>::min();

	for (int i = 0; i < ITERATIONS; ++i) {
		FloatT x = rng.randd() * 2000.0 - 1000.0;
		FloatT y = rng.randd() * 2000.0 - 1000.0;
		FloatT z = rng.randd() * 2000.0 - 1000.0;

		FloatT n = noise_func_2d(x, y);

		min_value_2d = math::min(n, min_value_2d);
		max_value_2d = math::max(n, max_value_2d);

		n = noise_func_3d(x, y, z);

		min_value_3d = math::min(n, min_value_3d);
		max_value_3d = math::max(n, max_value_3d);
	}

	print_line(format("2D | Min: {}, Max: {}", min_value_2d, max_value_2d));
	print_line(format("3D | Min: {}, Max: {}", min_value_3d, max_value_3d));
}

// 针对噪声函数的通用分析
template <typename F2, typename F3, typename FloatT>
void test_derivatives_tpl(F2 noise_func_2d, F3 noise_func_3d, RandomPCG &rng) {
	const int iterations = ITERATIONS;
	const int step_resolution_count = STEP_RESOLUTION_COUNT;
	const FloatT step_min = STEP_MIN;
	const FloatT step_max = STEP_MAX;

	print_line(format("Derivatives across step from {} to {}", step_min, step_max));

	const FloatT step_resolution_count_f = step_resolution_count;

	print_line("2D:");

	FloatT min_max_derivative = std::numeric_limits<FloatT>::max();

	for (int j = 0; j < step_resolution_count; ++j) {
		FloatT max_derivative = 0.0;
		const FloatT step = Math::lerp(0.0001, 0.001, static_cast<FloatT>(j) / step_resolution_count_f);

		for (int i = 0; i < iterations; ++i) {
			const FloatT x = rng.randd() * 2000.0 - 1000.0;
			const FloatT y = rng.randd() * 2000.0 - 1000.0;

			FloatT d = get_derivative(x, y, step, noise_func_2d);
			if (d > max_derivative) {
				max_derivative = d;
			}
		}

		max_derivative /= step;

		print_line(format("{}", max_derivative));

		if (max_derivative < min_max_derivative) {
			min_max_derivative = max_derivative;
		}
	}

	print_line(format("Min max derivative: {}", min_max_derivative));

	print_line("3D:");

	min_max_derivative = std::numeric_limits<FloatT>::max();

	for (int j = 0; j < step_resolution_count; ++j) {
		FloatT max_derivative = 0.0;
		const FloatT step = Math::lerp(0.0001, 0.001, static_cast<FloatT>(j) / step_resolution_count_f);

		for (int i = 0; i < iterations; ++i) {
			const FloatT x = rng.randd() * 2000.0 - 1000.0;
			const FloatT y = rng.randd() * 2000.0 - 1000.0;
			const FloatT z = rng.randd() * 2000.0 - 1000.0;

			FloatT d = get_derivative(x, y, z, step, noise_func_3d);
			if (d > max_derivative) {
				max_derivative = d;
			}
		}

		max_derivative /= step;

		print_line(format("{}", max_derivative));

		if (max_derivative < min_max_derivative) {
			min_max_derivative = max_derivative;
		}
	}

	print_line(format("Min max derivative: {}", min_max_derivative));
}

template <typename F3>
void test_derivatives_with_image(String fpath, double step, F3 noise_func_3d) {
	const double x_min = 500.0;
	const double y = 500.0;
	const double z_min = 500.0;

	const int size_x = 512;
	const int size_z = 512;

	const double image_step = 1.0;

	const double x_max = x_min + image_step;
	const double z_max = z_min + image_step;

	const double min_value = 0.0;
	const double max_value = 10.0;

	Ref<Image> im = godot::create_empty_image(size_x, size_z, false, Image::FORMAT_RGB8);

	for (int py = 0; py < size_z; ++py) {
		for (int px = 0; px < size_x; ++px) {
			const double x = Math::lerp(x_min, x_max, static_cast<double>(px) / static_cast<double>(size_x));
			const double z = Math::lerp(z_min, z_max, static_cast<double>(py) / static_cast<double>(size_z));
			const double d = get_derivative(x, y, z, step, noise_func_3d) / step;
			const double g = (d - min_value) / (max_value - min_value);
			im->set_pixel(px, py, Color(g, g, g));
		}
	}

	::print_line(String("Saving {0}").format(varray(fpath)));
	im->save_png(fpath);
}

template <typename F3>
void test_derivatives_with_image(String fname, int steps_resolution, F3 noise_func_3d) {
	for (int i = 0; i < steps_resolution; ++i) {
		const double step =
				Math::lerp(STEP_MIN, STEP_MAX, static_cast<double>(i) / static_cast<double>(steps_resolution));
		String fpath = String("{0}_{1}.png").format(varray(fname, i));
		test_derivatives_with_image(fpath, step, noise_func_3d);
	}
}

template <typename F2, typename F3>
void test_noise(String name, int tests, F2 noise_func_2d, F3 noise_func_3d, RandomPCG &rng) {
	::print_line(String("--- {0}:").format(varray(name)));

	if (tests & TEST_MIN_MAX) {
		test_min_max<F2, F3, double>(noise_func_2d, noise_func_3d, rng);
	}
	if (tests & TEST_DERIVATIVES) {
		test_derivatives_tpl<F2, F3, double>(noise_func_2d, noise_func_3d, rng);
		test_derivatives_with_image(name + "_3D", 10, noise_func_3d);
	}
}

void test_fnl_noise(fast_noise_lite::FastNoiseLite &fnl, String name, int tests) {
	RandomPCG rng;
	rng.seed(131183);

	test_noise(
			name,
			tests,
			[&fnl](double x, double y) { return fnl.GetNoise(x, y); },
			[&fnl](double x, double y, double z) { return fnl.GetNoise(x, y, z); },
			rng
	);
}

void test_noises() {
	fast_noise_lite::FastNoiseLite fn;
	fn.SetFractalType(fast_noise_lite::FastNoiseLite::FractalType_None);
	fn.SetFrequency(1.f);

	// 根据 OpenSimplex2 作者的说法，3D 版本的最大导数应该在 4.23718 左右
	// https://www.wolframalpha.com/input/?i=max+d%2Fdx+32.69428253173828125+*+x+*+%28%280.6-x%5E2%29%5E4%29+from+-0.6+to+0.6
	// 但实测结果显示它在 8 左右。不过这种噪声中确实存在不连续点，
	// 这使得测量更加困难（也正是使用多种步长的原因）

	fn.SetNoiseType(fast_noise_lite::FastNoiseLite::NoiseType_OpenSimplex2);
	test_fnl_noise(fn, "FNL_OpenSimplex2", TEST_MIN_MAX | TEST_DERIVATIVES);

	fn.SetNoiseType(fast_noise_lite::FastNoiseLite::NoiseType_OpenSimplex2S);
	test_fnl_noise(fn, "FNL_OpenSimplex2S", TEST_MIN_MAX | TEST_DERIVATIVES);

	fn.SetNoiseType(fast_noise_lite::FastNoiseLite::NoiseType_Perlin);
	test_fnl_noise(fn, "FNL_Perlin", TEST_MIN_MAX | TEST_DERIVATIVES);

	fn.SetNoiseType(fast_noise_lite::FastNoiseLite::NoiseType_Value);
	test_fnl_noise(fn, "FNL Value", TEST_MIN_MAX | TEST_DERIVATIVES);

	// ValueCubic 的取值范围似乎小于 -1..1
	// 2D | Min: -0.714547, Max: 0.742197
	// 3D | Min: -0.542093, Max: 0.499036
	fn.SetNoiseType(fast_noise_lite::FastNoiseLite::NoiseType_ValueCubic);
	test_fnl_noise(fn, "FNL_ValueCubic", TEST_MIN_MAX | TEST_DERIVATIVES);

	fn.SetNoiseType(fast_noise_lite::FastNoiseLite::NoiseType_Cellular);

	const char *cell_distance_function_names[] = {
		"Euclidean", //
		"EuclideanSq", //
		"Manhattan", //
		"Hybrid" //
	};
	const char *cell_return_type_names[] = {
		"CellValue", //
		"Distance", //
		"Distance2", //
		"Distance2Add", //
		"Distance2Sub", //
		"Distance2Mul", //
		"Distance2Div" //
	};

	for (int cell_distance_function = 0; cell_distance_function < 4; ++cell_distance_function) {
		for (int cell_return_type = 0; cell_return_type < 7; ++cell_return_type) {
			fn.SetCellularDistanceFunction(
					static_cast<fast_noise_lite::FastNoiseLite::CellularDistanceFunction>(cell_distance_function)
			);
			fn.SetCellularReturnType(static_cast<fast_noise_lite::FastNoiseLite::CellularReturnType>(cell_return_type));

			const char *cell_distance_function_name = cell_distance_function_names[cell_distance_function];
			const char *cell_return_type_name = cell_return_type_names[cell_return_type];
			String noise_name =
					String("FNL_Cellular_{0}_{1}").format(varray(cell_distance_function_name, cell_return_type_name));

			const int jitter_resolution = 10;

			for (int i = 0; i < jitter_resolution; ++i) {
				const double jitter =
						Math::lerp(0.0, 1.0, static_cast<double>(i) / static_cast<double>(jitter_resolution));

				fn.SetCellularJitter(jitter);
				::print_line(String("Cell jitter: {0}").format(varray(jitter)));

				test_fnl_noise(fn, noise_name, TEST_MIN_MAX);
			}
		}
	}

	// 电子表格辅助输出：
	print_line("Steps:");
	for (int i = 0; i < STEP_RESOLUTION_COUNT; ++i) {
		const double step =
				Math::lerp(STEP_MIN, STEP_MAX, static_cast<double>(i) / static_cast<double>(STEP_RESOLUTION_COUNT));
		print_line(format("{}", step));
	}
}

// 这些其实并不是单元测试，而是分析。将来它们可以配合测试使用，但
// 对导数来说会比较困难，因为实测可能碰到不规则处从而产生误报，
// 所以目前导数范围是根据结果手动估算的
void run_noise_tests() {
	test_noises();
}

} // namespace voxel::noise_tests
