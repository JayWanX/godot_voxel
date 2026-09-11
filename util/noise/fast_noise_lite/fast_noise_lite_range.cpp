#include "fast_noise_lite_range.h"
#include "fast_noise_lite.h"
#include "../noise_range_utility.h"
#include <modules/noise/fastnoise_lite.h>
#include "fast_noise_lite_gradient.h"

namespace voxel {

using namespace math;

namespace {

Interval fnl_single_cellular_value_2d(const fast_noise_lite::FastNoiseLite &fn, Interval x, Interval y) {
	const float c0 = fn.GetNoise(x.min, y.min);
	const float c1 = fn.GetNoise(x.max, y.min);
	const float c2 = fn.GetNoise(x.min, y.max);
	const float c3 = fn.GetNoise(x.max, y.max);
	if (c0 == c1 && c1 == c2 && c2 == c3) {
		return Interval::from_single_value(c0);
	}
	return Interval{ -1, 1 };
}

Interval fnl_single_cellular_value_3d(const fast_noise_lite::FastNoiseLite &fn, Interval x, Interval y, Interval z) {
	const float c0 = fn.GetNoise(x.min, y.min, z.min);
	const float c1 = fn.GetNoise(x.max, y.min, z.min);
	const float c2 = fn.GetNoise(x.min, y.max, z.min);
	const float c3 = fn.GetNoise(x.max, y.max, z.min);
	const float c4 = fn.GetNoise(x.max, y.max, z.max);
	const float c5 = fn.GetNoise(x.max, y.max, z.max);
	const float c6 = fn.GetNoise(x.max, y.max, z.max);
	const float c7 = fn.GetNoise(x.max, y.max, z.max);
	if (c0 == c1 && c1 == c2 && c2 == c3 && c3 == c4 && c4 == c5 && c5 == c6 && c6 == c7) {
		return Interval::from_single_value(c0);
	}
	return Interval{ -1, 1 };
}

Interval get_fnl_cellular_range_2d(const Voxel_FastNoiseLite &noise) {
	// 细胞噪声的组合很多，所以与其用区间来实现它们，
	// 我用经验测试来确定一些边界。

	// Value 模式必须单独处理。

	switch (noise.get_cellular_distance_function()) {
		case Voxel_FastNoiseLite::CELLULAR_DISTANCE_EUCLIDEAN:
			switch (noise.get_cellular_return_type()) {
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE:
					return Interval{ -1.f, 0.08f };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2:
					return Interval{ -0.92f, 0.35 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_ADD:
					return Interval{ -0.92f, 0.1 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_SUB:
					return Interval{ -1, 0.15 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_MUL:
					return Interval{ -1, 0 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_DIV:
					return Interval{ -1, 0 };
				default:
					ERR_FAIL_V(Interval(-1, 1));
			}
			break;

		case Voxel_FastNoiseLite::CELLULAR_DISTANCE_EUCLIDEAN_SQ:
			switch (noise.get_cellular_return_type()) {
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE:
					return Interval{ -1, 0.2 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2:
					return Interval{ -1, 0.8 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_ADD:
					return Interval{ -1, 0.2 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_SUB:
					return Interval{ -1, 0.7 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_MUL:
					return Interval{ -1, 0 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_DIV:
					return Interval{ -1, 0 };
				default:
					ERR_FAIL_V(Interval(-1, 1));
			}

		case Voxel_FastNoiseLite::CELLULAR_DISTANCE_MANHATTAN:
			switch (noise.get_cellular_return_type()) {
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE:
					return Interval{ -1, 0.75 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2:
					return Interval{ -0.9, 0.8 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_ADD:
					return Interval{ -0.8, 0.8 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_SUB:
					return Interval{ -1.0, 0.5 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_MUL:
					return Interval{ -1.0, 0.7 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_DIV:
					return Interval{ -1.0, 0.0 };
				default:
					ERR_FAIL_V(Interval(-1, 1));
			}

		case Voxel_FastNoiseLite::CELLULAR_DISTANCE_HYBRID:
			switch (noise.get_cellular_return_type()) {
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE:
					return Interval{ -1, 1.75 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2:
					return Interval{ -0.9, 2.3 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_ADD:
					return Interval{ -0.9, 1.9 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_SUB:
					return Interval{ -1.0, 1.85 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_MUL:
					return Interval{ -1.0, 3.4 };
				case Voxel_FastNoiseLite::CELLULAR_RETURN_DISTANCE_2_DIV:
					return Interval{ -1.0, 0.0 };
				default:
					ERR_FAIL_V(Interval(-1, 1));
			}
	}
	return Interval{ -1.f, 1.f };
}

Interval get_fnl_cellular_range_3d(const Voxel_FastNoiseLite &noise) {
	// 细胞噪声的组合很多，所以与其用区间来实现它们，
	// 我用经验测试来确定一些边界。

	// Value 模式必须单独处理。

	return get_fnl_cellular_range_2d(noise);
}

void fnl_transform_noise_coordinate_2d(const fast_noise_lite::FastNoiseLite &fn, Interval &x, Interval &y) {
	x *= fn.mFrequency;
	y *= fn.mFrequency;

	switch (fn.mNoiseType) {
		case fast_noise_lite::FastNoiseLite::NoiseType_OpenSimplex2:
		case fast_noise_lite::FastNoiseLite::NoiseType_OpenSimplex2S: {
			const float SQRT3 = 1.7320508075688772935274463415059;
			const float F2 = 0.5f * (SQRT3 - 1);
			const Interval t = (x + y) * F2;
			x += t;
			y += t;
		} break;
		default:
			break;
	}
}

void fnl_transform_noise_coordinate_3d(
		const fast_noise_lite::FastNoiseLite &fn,
		Interval &x,
		Interval &y,
		Interval &z
) {
	// 与 FastNoiseLite 内部函数的逻辑相同

	x *= fn.mFrequency;
	y *= fn.mFrequency;
	z *= fn.mFrequency;

	switch (fn.mTransformType3D) {
		case fast_noise_lite::FastNoiseLite::TransformType3D_ImproveXYPlanes: {
			Interval xy = x + y;
			Interval s2 = xy * (-0.211324865405187);
			z *= 0.577350269189626;
			x += s2 - z;
			y = y + s2 - z;
			z += xy * 0.577350269189626;
		} break;
		case fast_noise_lite::FastNoiseLite::TransformType3D_ImproveXZPlanes: {
			Interval xz = x + z;
			Interval s2 = xz * (-0.211324865405187);
			y *= 0.577350269189626;
			x += s2 - y;
			z += s2 - y;
			y += xz * 0.577350269189626;
		} break;
		case fast_noise_lite::FastNoiseLite::TransformType3D_DefaultOpenSimplex2: {
			const float R3 = (2.0 / 3.0);
			Interval r = (x + y + z) * R3; // 旋转，不是斜切
			x = r - x;
			y = r - y;
			z = r - z;
		} break;
		default:
			break;
	}
}

Interval fnl_single_opensimplex2_3d(
		const fast_noise_lite::FastNoiseLite &fn,
		int seed,
		Interval p_x,
		Interval p_y,
		Interval p_z
) {
	// 据 OpenSimplex2 的作者说，3D 版本的最大导数应该在 4.23718 左右
	// https://www.wolframalpha.com/input/?i=max+d%2Fdx+32.69428253173828125+*+x+*+%28%280.6-x%5E2%29%5E4%29+from+-0.6+to+0.6
	// 但经验测量显示它大约在 8。这种噪声中确实存在不连续点，
	// 这使得测量更加困难
	return get_noise_range_3d(
			[&fn, seed](real_t x, real_t y, real_t z) { //
				return fn.SingleOpenSimplex2(seed, x, y, z);
			},
			p_x,
			p_y,
			p_z,
			4.23718f
	);
}

Interval fnl_single_opensimplex2s_3d(
		const fast_noise_lite::FastNoiseLite &fn,
		int seed,
		Interval p_x,
		Interval p_y,
		Interval p_z
) {
	return get_noise_range_3d(
			[&fn, seed](real_t x, real_t y, real_t z) { //
				return fn.SingleOpenSimplex2S(seed, x, y, z);
			},
			// 从经验测试中得到的最大导数
			p_x,
			p_y,
			p_z,
			2.5f
	);
}

Interval fnl_single_cellular_3d(const Voxel_FastNoiseLite &noise, Interval x, Interval y, Interval z) {
	const fast_noise_lite::FastNoiseLite &fn = noise.get_noise_internal();
	if (fn.mCellularReturnType == fast_noise_lite::FastNoiseLite::CellularReturnType_CellValue) {
		return fnl_single_cellular_value_3d(fn, x, y, z);
	}
	return get_fnl_cellular_range_3d(noise);
}

Interval fnl_single_cellular_2d(const Voxel_FastNoiseLite &noise, Interval x, Interval y) {
	const fast_noise_lite::FastNoiseLite &fn = noise.get_noise_internal();
	if (fn.mCellularReturnType == fast_noise_lite::FastNoiseLite::CellularReturnType_CellValue) {
		return fnl_single_cellular_value_2d(fn, x, y);
	}
	return get_fnl_cellular_range_2d(noise);
}

Interval fnl_single_perlin_3d(
		const fast_noise_lite::FastNoiseLite &fn,
		int seed,
		Interval p_x,
		Interval p_y,
		Interval p_z
) {
	return get_noise_range_3d(
			[&fn, seed](real_t x, real_t y, real_t z) { //
				return fn.SinglePerlin(seed, x, y, z);
			},
			// 从经验测试中得到的最大导数
			p_x,
			p_y,
			p_z,
			3.2f
	);
}

Interval fnl_single_value_cubic_3d(
		const fast_noise_lite::FastNoiseLite &fn,
		int seed,
		Interval p_x,
		Interval p_y,
		Interval p_z
) {
	return get_noise_range_3d(
			[&fn, seed](real_t x, real_t y, real_t z) { //
				return fn.SingleValueCubic(seed, x, y, z);
			},
			// 从经验测试中得到的最大导数
			p_x,
			p_y,
			p_z,
			1.2f
	);
}

Interval fnl_single_value_3d(
		const fast_noise_lite::FastNoiseLite &fn,
		int seed,
		Interval p_x,
		Interval p_y,
		Interval p_z
) {
	return get_noise_range_3d(
			[&fn, seed](real_t x, real_t y, real_t z) { //
				return fn.SingleValue(seed, x, y, z);
			},
			// 从经验测试中得到的最大导数
			p_x,
			p_y,
			p_z,
			3.0f
	);
}

Interval fnl_gen_noise_single_2d(const Voxel_FastNoiseLite &noise, const int seed, Interval x, Interval y) {
	// 与 FastNoiseLite 内部函数的逻辑相同
	const fast_noise_lite::FastNoiseLite &fn = noise.get_noise_internal();

	// TODO 2D 范围变体
	// TODO 使用导数做更精确的分析

	switch (fn.mNoiseType) {
		case fast_noise_lite::FastNoiseLite::NoiseType_OpenSimplex2:
			// return fnl_single_opensimplex2_2d(fn, seed, x, y);
			return { -1.0, 1.0 };
		case fast_noise_lite::FastNoiseLite::NoiseType_OpenSimplex2S:
			// return fnl_single_opensimplex2s_2d(fn, seed, x, y);
			return { -1.0, 1.0 };
		case fast_noise_lite::FastNoiseLite::NoiseType_Cellular:
			return fnl_single_cellular_2d(noise, x, y);
		case fast_noise_lite::FastNoiseLite::NoiseType_Perlin:
			// return fnl_single_perlin_2d(fn, seed, x, y);
			return { -1.0, 1.0 };
		case fast_noise_lite::FastNoiseLite::NoiseType_ValueCubic:
			// return fnl_single_value_cubic_2d(fn, seed, x, y);
			return { -1.0, 1.0 };
		case fast_noise_lite::FastNoiseLite::NoiseType_Value:
			// return fnl_single_value_2d(fn, seed, x, y);
			return { -1.0, 1.0 };
		default:
			return Interval::from_single_value(0);
	}
}

Interval fnl_gen_noise_single_3d(const Voxel_FastNoiseLite &noise, int seed, Interval x, Interval y, Interval z) {
	// 与 FastNoiseLite 内部函数的逻辑相同
	const fast_noise_lite::FastNoiseLite &fn = noise.get_noise_internal();

	switch (fn.mNoiseType) {
		case fast_noise_lite::FastNoiseLite::NoiseType_OpenSimplex2:
			return fnl_single_opensimplex2_3d(fn, seed, x, y, z);
		case fast_noise_lite::FastNoiseLite::NoiseType_OpenSimplex2S:
			return fnl_single_opensimplex2s_3d(fn, seed, x, y, z);
		case fast_noise_lite::FastNoiseLite::NoiseType_Cellular:
			return fnl_single_cellular_3d(noise, x, y, z);
		case fast_noise_lite::FastNoiseLite::NoiseType_Perlin:
			return fnl_single_perlin_3d(fn, seed, x, y, z);
		case fast_noise_lite::FastNoiseLite::NoiseType_ValueCubic:
			return fnl_single_value_cubic_3d(fn, seed, x, y, z);
		case fast_noise_lite::FastNoiseLite::NoiseType_Value:
			return fnl_single_value_3d(fn, seed, x, y, z);
		default:
			return Interval::from_single_value(0);
	}
}

Interval fnl_gen_fractal_fbm_2d(const Voxel_FastNoiseLite &p_noise, Interval x, Interval y) {
	// 与 FastNoiseLite 内部函数的逻辑相同
	const fast_noise_lite::FastNoiseLite &fn = p_noise.get_noise_internal();

	int seed = fn.mSeed;
	Interval sum;
	Interval amp = Interval::from_single_value(fn.mFractalBounding);

	for (int i = 0; i < fn.mOctaves; i++) {
		Interval noise = fnl_gen_noise_single_2d(p_noise, seed++, x, y);
		sum += noise * amp;
		amp *=
				lerp(Interval::from_single_value(1.0),
					 min_interval<real_t>(noise + 1, 2) * 0.5,
					 Interval::from_single_value(fn.mWeightedStrength));

		x *= fn.mLacunarity;
		y *= fn.mLacunarity;
		amp *= fn.mGain;
	}

	return sum;
}

Interval fnl_gen_fractal_fbm_3d(const Voxel_FastNoiseLite &p_noise, Interval x, Interval y, Interval z) {
	// 与 FastNoiseLite 内部函数的逻辑相同
	const fast_noise_lite::FastNoiseLite &fn = p_noise.get_noise_internal();

	int seed = fn.mSeed;
	Interval sum;
	Interval amp = Interval::from_single_value(fn.mFractalBounding);

	for (int i = 0; i < fn.mOctaves; i++) {
		Interval noise = fnl_gen_noise_single_3d(p_noise, seed++, x, y, z);
		sum += noise * amp;
		amp *=
				lerp(Interval::from_single_value(1.0f),
					 (noise + Interval::from_single_value(1.0f)) * 0.5f,
					 Interval::from_single_value(fn.mWeightedStrength));

		x *= fn.mLacunarity;
		y *= fn.mLacunarity;
		z *= fn.mLacunarity;
		amp *= fn.mGain;
	}

	return sum;
}

Interval fnl_gen_fractal_ridged_2d(const Voxel_FastNoiseLite &p_noise, Interval x, Interval y) {
	// 与 FastNoiseLite 内部函数的逻辑相同
	const fast_noise_lite::FastNoiseLite &fn = p_noise.get_noise_internal();

	int seed = fn.mSeed;
	Interval sum;
	Interval amp = Interval::from_single_value(fn.mFractalBounding);

	for (int i = 0; i < fn.mOctaves; i++) {
		Interval noise = abs(fnl_gen_noise_single_2d(p_noise, seed++, x, y));
		sum += (noise * -2 + 1) * amp;
		amp *=
				lerp(Interval::from_single_value(1.0f),
					 Interval::from_single_value(1.0f) - noise,
					 Interval::from_single_value(fn.mWeightedStrength));

		x *= fn.mLacunarity;
		y *= fn.mLacunarity;
		amp *= fn.mGain;
	}

	return sum;
}

Interval fnl_gen_fractal_ridged_3d(const Voxel_FastNoiseLite &p_noise, Interval x, Interval y, Interval z) {
	// 与 FastNoiseLite 内部函数的逻辑相同
	const fast_noise_lite::FastNoiseLite &fn = p_noise.get_noise_internal();

	int seed = fn.mSeed;
	Interval sum;
	Interval amp = Interval::from_single_value(fn.mFractalBounding);

	for (int i = 0; i < fn.mOctaves; i++) {
		Interval noise = abs(fnl_gen_noise_single_3d(p_noise, seed++, x, y, z));
		sum += (noise * -2 + 1) * amp;
		amp *=
				lerp(Interval::from_single_value(1.0f),
					 Interval::from_single_value(1.0f) - noise,
					 Interval::from_single_value(fn.mWeightedStrength));

		x *= fn.mLacunarity;
		y *= fn.mLacunarity;
		z *= fn.mLacunarity;
		amp *= fn.mGain;
	}

	return sum;
}

Interval fnl_get_noise_2d(const Voxel_FastNoiseLite &noise, Interval x, Interval y) {
	// 与 FastNoiseLite 内部函数的逻辑相同
	const fast_noise_lite::FastNoiseLite &fn = noise.get_noise_internal();

	fnl_transform_noise_coordinate_2d(fn, x, y);

	switch (noise.get_fractal_type()) {
		case Voxel_FastNoiseLite::FRACTAL_NONE:
			return fnl_gen_noise_single_2d(noise, noise.get_seed(), x, y);
		case Voxel_FastNoiseLite::FRACTAL_FBM:
			return fnl_gen_fractal_fbm_2d(noise, x, y);
		case Voxel_FastNoiseLite::FRACTAL_RIDGED:
			return fnl_gen_fractal_ridged_2d(noise, x, y);
		case Voxel_FastNoiseLite::FRACTAL_PING_PONG:
			// TODO Ping pong 模式
			return Interval(-1.f, 1.f);
		default:
			VOXEL_PRINT_ERROR("Unhandled fractal type");
			return Interval(-1.f, 1.f);
	}
}

Interval fnl_get_noise_3d(const Voxel_FastNoiseLite &noise, Interval x, Interval y, Interval z) {
	// 与 FastNoiseLite 内部函数的逻辑相同
	const fast_noise_lite::FastNoiseLite &fn = noise.get_noise_internal();

	fnl_transform_noise_coordinate_3d(fn, x, y, z);

	switch (noise.get_fractal_type()) {
		case Voxel_FastNoiseLite::FRACTAL_NONE:
			return fnl_gen_noise_single_3d(noise, noise.get_seed(), x, y, z);
		case Voxel_FastNoiseLite::FRACTAL_FBM:
			return fnl_gen_fractal_fbm_3d(noise, x, y, z);
		case Voxel_FastNoiseLite::FRACTAL_RIDGED:
			return fnl_gen_fractal_ridged_3d(noise, x, y, z);
		case Voxel_FastNoiseLite::FRACTAL_PING_PONG:
			// TODO Ping pong 模式
			return Interval(-1.f, 1.f);
		default:
			VOXEL_PRINT_ERROR("Unhandled fractal type");
			return Interval(-1.f, 1.f);
	}
}

} // namespace

Interval get_fnl_range_2d(const Voxel_FastNoiseLite &noise, Interval x, Interval y) {
	Ref<Voxel_FastNoiseLiteGradient> grad = noise.get_warp_noise();
	if (grad.is_valid()) {
		math::Interval2 gr = get_fnl_gradient_range_2d(**grad, x, y);
		x.add_interval(gr.x);
		y.add_interval(gr.y);
	}
	return fnl_get_noise_2d(noise, x, y);
}

Interval get_fnl_range_3d(const Voxel_FastNoiseLite &noise, Interval x, Interval y, Interval z) {
	Ref<Voxel_FastNoiseLiteGradient> grad = noise.get_warp_noise();
	if (grad.is_valid()) {
		math::Interval3 gr = get_fnl_gradient_range_3d(**grad, x, y, z);
		x.add_interval(gr.x);
		y.add_interval(gr.y);
		z.add_interval(gr.z);
	}
	return fnl_get_noise_3d(noise, x, y, z);
}

math::Interval2 get_fnl_gradient_range_2d(const Voxel_FastNoiseLiteGradient &noise, Interval x, Interval y) {
	// TODO 更精确的分析
	const float amp = Math::abs(noise.get_amplitude());
	return math::Interval2{
		Interval{ x.min - amp, x.max + amp }, //
		Interval{ y.min - amp, y.max + amp } //
	};
}

math::Interval3 get_fnl_gradient_range_3d(const Voxel_FastNoiseLiteGradient &noise, Interval x, Interval y, Interval z) {
	// TODO 更精确的分析
	const float amp = Math::abs(noise.get_amplitude());
	return math::Interval3{
		Interval{ x.min - amp, x.max + amp }, //
		Interval{ y.min - amp, y.max + amp }, //
		Interval{ z.min - amp, z.max + amp } //
	};
}

} // namespace voxel
