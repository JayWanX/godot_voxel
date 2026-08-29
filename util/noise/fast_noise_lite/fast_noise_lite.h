#ifndef VOXEL_FAST_NOISE_LITE_H
#define VOXEL_FAST_NOISE_LITE_H

#include "fast_noise_lite_gradient.h"

namespace voxel {

// 我为 Godot Engine 实现的 FastNoiseLite。
// Godot 4 自带自己的 FastNoiseLite，但我的版本比它早。所以需要加前缀以避免冲突。
// 两者各有利弊。Godot 的实现有一些实际差异：
//
// - get_noise* 方法不是内联的。这意味着多次调用时可能有性能损失
//  （本模块中基本一直都在调用）。
//
// - get_noise* 方法不是 `const`。这意味着任何创建 Noise 实现的人都可以修改内部状态，
//   这对多线程使用不利。在我看来噪声不应该有状态，即便有也必须是显式的，不能
//    "惰性"地改变。开发者也还没确定是否应该改变这一点。
//
// - 到处使用 `real_t`，而不只是坐标。这意味着 `float=64` 的构建可能更慢，
//   尤其是在*对噪声生成这个用例*来说并不需要这种精度的情况下。
//
// - 域扭曲（Domain warp）没有作为独立的东西暴露，所以无法在单次调用中从 (x,y,z) 坐标生成
//
// - FastNoiseLite 对象的内部实例不可访问，而且它没有模块版本中的那些访问更改，
//   所以无法做更精确的范围分析。这对 `VoxelGeneratorGraph` 很重要。
//
// - 不使用 `GDVirtual`，所以无法被脚本扩展。
//
class Voxel_FastNoiseLite : public Resource {
	GDCLASS(Voxel_FastNoiseLite, Resource)

	typedef ::fast_noise_lite::FastNoiseLite _FastNoise;

public:
	static const int MAX_OCTAVES = 32;

	enum NoiseType {
		TYPE_OPEN_SIMPLEX_2 = _FastNoise::NoiseType_OpenSimplex2,
		TYPE_OPEN_SIMPLEX_2S = _FastNoise::NoiseType_OpenSimplex2S,
		TYPE_CELLULAR = _FastNoise::NoiseType_Cellular,
		TYPE_PERLIN = _FastNoise::NoiseType_Perlin,
		TYPE_VALUE_CUBIC = _FastNoise::NoiseType_ValueCubic,
		TYPE_VALUE = _FastNoise::NoiseType_Value
	};

	enum FractalType {
		FRACTAL_NONE = _FastNoise::FractalType_None,
		FRACTAL_FBM = _FastNoise::FractalType_FBm,
		FRACTAL_RIDGED = _FastNoise::FractalType_Ridged,
		FRACTAL_PING_PONG = _FastNoise::FractalType_PingPong
	};

	enum RotationType3D {
		ROTATION_3D_NONE = _FastNoise::RotationType3D_None,
		ROTATION_3D_IMPROVE_XY_PLANES = _FastNoise::RotationType3D_ImproveXYPlanes,
		ROTATION_3D_IMPROVE_XZ_PLANES = _FastNoise::RotationType3D_ImproveXZPlanes
	};

	enum CellularDistanceFunction {
		CELLULAR_DISTANCE_EUCLIDEAN = _FastNoise::CellularDistanceFunction_Euclidean,
		CELLULAR_DISTANCE_EUCLIDEAN_SQ = _FastNoise::CellularDistanceFunction_EuclideanSq,
		CELLULAR_DISTANCE_MANHATTAN = _FastNoise::CellularDistanceFunction_Manhattan,
		CELLULAR_DISTANCE_HYBRID = _FastNoise::CellularDistanceFunction_Hybrid
	};

	enum CellularReturnType {
		CELLULAR_RETURN_CELL_VALUE = _FastNoise::CellularReturnType_CellValue,
		CELLULAR_RETURN_DISTANCE = _FastNoise::CellularReturnType_Distance,
		CELLULAR_RETURN_DISTANCE_2 = _FastNoise::CellularReturnType_Distance2,
		CELLULAR_RETURN_DISTANCE_2_ADD = _FastNoise::CellularReturnType_Distance2Add,
		CELLULAR_RETURN_DISTANCE_2_SUB = _FastNoise::CellularReturnType_Distance2Sub,
		CELLULAR_RETURN_DISTANCE_2_MUL = _FastNoise::CellularReturnType_Distance2Mul,
		CELLULAR_RETURN_DISTANCE_2_DIV = _FastNoise::CellularReturnType_Distance2Div
	};

	Voxel_FastNoiseLite();

	// 属性

	void set_noise_type(NoiseType type);
	NoiseType get_noise_type() const;

	void set_seed(int seed);
	int get_seed() const;

	void set_period(float p);
	float get_period() const;

	void set_warp_noise(Ref<Voxel_FastNoiseLiteGradient> warp_noise);
	Ref<Voxel_FastNoiseLiteGradient> get_warp_noise() const;

	void set_fractal_type(FractalType type);
	FractalType get_fractal_type() const;

	void set_fractal_octaves(int octaves);
	int get_fractal_octaves() const;

	void set_fractal_lacunarity(float lacunarity);
	float get_fractal_lacunarity() const;

	void set_fractal_gain(float gain);
	float get_fractal_gain() const;

	void set_fractal_ping_pong_strength(float s);
	float get_fractal_ping_pong_strength() const;

	void set_fractal_weighted_strength(float s);
	float get_fractal_weighted_strength() const;

	void set_cellular_distance_function(CellularDistanceFunction cdf);
	CellularDistanceFunction get_cellular_distance_function() const;

	void set_cellular_return_type(CellularReturnType rt);
	CellularReturnType get_cellular_return_type() const;

	void set_cellular_jitter(float jitter);
	float get_cellular_jitter() const;

	void set_rotation_type_3d(RotationType3D type);
	RotationType3D get_rotation_type_3d() const;

	// 查询

	inline float get_noise_2d(real_t x, real_t y) const {
		if (_warp_noise.is_valid()) {
			_warp_noise->warp_2d(x, y);
		}
		return _fn.GetNoise(x, y);
	}

	inline float get_noise_3d(real_t x, real_t y, real_t z) const {
		if (_warp_noise.is_valid()) {
			_warp_noise->warp_3d(x, y, z);
		}
		return _fn.GetNoise(x, y, z);
	}

	// TODO 要不要单独做一个细胞噪声？它输出多种东西，但我们只需要一种。
	// 要获取其他值，API 迫使我们再计算一次，而这又是最昂贵的噪声……

	// 内部

	inline float get_noise_2d_unwarped(const real_t x, const real_t y) const {
		return _fn.GetNoise(x, y);
	}

	inline float get_noise_3d_unwarped(const real_t x, const real_t y, const real_t z) const {
		return _fn.GetNoise(x, y, z);
	}

	const ::fast_noise_lite::FastNoiseLite &get_noise_internal() const {
		return _fn;
	}

private:
	static void _bind_methods();

	void _on_warp_noise_changed();

	float _b_get_noise_2d(real_t x, real_t y) {
		return get_noise_2d(x, y);
	}
	float _b_get_noise_3d(real_t x, real_t y, real_t z) {
		return get_noise_3d(x, y, z);
	}

	float _b_get_noise_2dv(Vector2 p) {
		return get_noise_2d(p.x, p.y);
	}
	float _b_get_noise_3dv(Vector3 p) {
		return get_noise_3d(p.x, p.y, p.z);
	}

	::fast_noise_lite::FastNoiseLite _fn;

	// TODO FastNoiseLite 更应该提供 getter

	NoiseType _noise_type = TYPE_OPEN_SIMPLEX_2;
	int _seed = 0;
	float _period = 64.f;

	FractalType _fractal_type = FRACTAL_FBM;
	int _fractal_octaves = 3;
	float _fractal_lacunarity = 2.f;
	float _fractal_ping_pong_strength = 2.f;
	float _fractal_gain = 0.5f;
	float _fractal_weighted_strength = 0.f;

	CellularDistanceFunction _cellular_distance_function = CELLULAR_DISTANCE_EUCLIDEAN_SQ;
	CellularReturnType _cellular_return_type = CELLULAR_RETURN_DISTANCE;
	float _cellular_jitter = 1.f;

	RotationType3D _rotation_type_3d = ROTATION_3D_NONE;

	Ref<Voxel_FastNoiseLiteGradient> _warp_noise;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::Voxel_FastNoiseLite::NoiseType);
VARIANT_ENUM_CAST(voxel::Voxel_FastNoiseLite::FractalType);
VARIANT_ENUM_CAST(voxel::Voxel_FastNoiseLite::RotationType3D);
VARIANT_ENUM_CAST(voxel::Voxel_FastNoiseLite::CellularDistanceFunction);
VARIANT_ENUM_CAST(voxel::Voxel_FastNoiseLite::CellularReturnType);

#endif // VOXEL_FAST_NOISE_LITE_H
