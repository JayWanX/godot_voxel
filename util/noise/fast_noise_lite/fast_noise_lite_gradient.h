#ifndef VOXEL_FAST_NOISE_LITE_GRADIENT_H
#define VOXEL_FAST_NOISE_LITE_GRADIENT_H

#include "../../../thirdparty/fast_noise/FastNoiseLite.h"
#include "../../godot/classes/resource.h"

namespace voxel {

// 域扭曲（Domain warp）是在采样实际噪声之前对坐标进行的一种变换。
// 可以用另一个噪声实例来做，但它需要对每个坐标各采样一次，
// 所以 FastNoiseLite 提供了使用梯度的专门版本。
// 这样更快，且结果质量更高。
//
// 注意：FastNoiseLite 用同一个类提供这个功能，但那会不清楚哪个作用于什么，
// 所以我做了两个类，各有特定用途。
//
class Voxel_FastNoiseLiteGradient : public Resource {
	GDCLASS(Voxel_FastNoiseLiteGradient, Resource)

	typedef ::fast_noise_lite::FastNoiseLite _FastNoise;

public:
	// TODO 必须加前缀，因为 https://github.com/godotengine/godot/issues/44860
	static const int _MAX_OCTAVES = 32;

	enum NoiseType {
		TYPE_OPEN_SIMPLEX_2 = _FastNoise::DomainWarpType_OpenSimplex2,
		TYPE_OPEN_SIMPLEX_2_REDUCED = _FastNoise::DomainWarpType_OpenSimplex2Reduced,
		TYPE_VALUE = _FastNoise::DomainWarpType_BasicGrid
	};

	// 遗憾的是这个不能直接映射到 FastNoise，
	// 因为 Godot 的 UI 希望值从 0 开始连续……
	enum FractalType { //
		FRACTAL_NONE,
		FRACTAL_DOMAIN_WARP_PROGRESSIVE,
		FRACTAL_DOMAIN_WARP_INDEPENDENT
	};

	enum RotationType3D {
		ROTATION_3D_NONE = _FastNoise::RotationType3D_None,
		ROTATION_3D_IMPROVE_XY_PLANES = _FastNoise::RotationType3D_ImproveXYPlanes,
		ROTATION_3D_IMPROVE_XZ_PLANES = _FastNoise::RotationType3D_ImproveXZPlanes
	};

	Voxel_FastNoiseLiteGradient();

	void set_noise_type(NoiseType type);
	NoiseType get_noise_type() const;

	void set_seed(int seed);
	int get_seed() const;

	void set_period(float p);
	float get_period() const;

	void set_amplitude(float amp);
	float get_amplitude() const;

	void set_fractal_type(FractalType type);
	FractalType get_fractal_type() const;
	_FastNoise::FractalType get_fractal_type_fnl() const;

	void set_fractal_octaves(int octaves);
	int get_fractal_octaves() const;

	void set_fractal_lacunarity(float lacunarity);
	float get_fractal_lacunarity() const;

	void set_fractal_gain(float gain);
	float get_fractal_gain() const;

	void set_rotation_type_3d(RotationType3D type);
	RotationType3D get_rotation_type_3d() const;

	// 这些是内联的，以确保内联真正发生。如果它们直接绑定到脚本 API，
	// 就意味着它们需要有地址，那样我就不确定它们还能不能内联了？

	inline void warp_2d(real_t &x, real_t &y) const {
		return _fn.DomainWarp(x, y);
	}

	inline void warp_3d(real_t &x, real_t &y, real_t &z) const {
		return _fn.DomainWarp(x, y, z);
	}

	// TODO 边界访问
	// TODO 区间范围分析

private:
	static void _bind_methods();

	// TODO 获取梯度而不是叠加梯度会不会更有用？

	Vector2 _b_warp_2d(Vector2 pos) {
		warp_2d(pos.x, pos.y);
		return pos;
	}

	Vector3 _b_warp_3d(Vector3 pos) {
		warp_3d(pos.x, pos.y, pos.z);
		return pos;
	}

	::fast_noise_lite::FastNoiseLite _fn;

	// TODO FastNoiseLite 更应该提供 getter

	NoiseType _noise_type = TYPE_VALUE;
	int _seed = 0;
	float _period = 64.f;
	float _amplitude = 30.f;

	FractalType _fractal_type = FRACTAL_NONE;
	int _fractal_octaves = 3;
	float _fractal_lacunarity = 2.f;
	float _fractal_gain = 0.5f;

	RotationType3D _rotation_type_3d = ROTATION_3D_NONE;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::Voxel_FastNoiseLiteGradient::NoiseType);
VARIANT_ENUM_CAST(voxel::Voxel_FastNoiseLiteGradient::FractalType);
VARIANT_ENUM_CAST(voxel::Voxel_FastNoiseLiteGradient::RotationType3D);

#endif // VOXEL_FAST_NOISE_LITE_GRADIENT_H
