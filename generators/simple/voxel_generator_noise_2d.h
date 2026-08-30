#ifndef VOXEL_GENERATOR_NOISE_2D_H
#define VOXEL_GENERATOR_NOISE_2D_H

#include "../../util/containers/span.h"
#include "../../util/godot/macros.h"
#include "../../util/math/vector3f.h"
#include "../../util/thread/rw_lock.h"
#include "voxel_generator_heightmap.h"

VOXEL_GODOT_FORWARD_DECLARE(class Curve)
VOXEL_GODOT_FORWARD_DECLARE(class Noise)

namespace voxel {

class VoxelGeneratorNoise2D : public VoxelGeneratorHeightmap {
	GDCLASS(VoxelGeneratorNoise2D, VoxelGeneratorHeightmap)

public:
	VoxelGeneratorNoise2D();
	~VoxelGeneratorNoise2D();

	// 用于生成高度图的 2D 噪声源
	void set_noise(Ref<Noise> noise);
	Ref<Noise> get_noise() const;

	// 用于调整高度值的曲线
	void set_curve(Ref<Curve> curve);
	Ref<Curve> get_curve() const;

	// 生成单个数据块的体素数据
	Result generate_block(VoxelGenerator::VoxelQueryData input) override;

	// 支持批量序列生成
	bool supports_series_generation() const override {
		return true;
	}

	// 批量生成体素值序列
	void generate_series(
			Span<const float> positions_x,
			Span<const float> positions_y,
			Span<const float> positions_z,
			unsigned int channel,
			Span<float> out_values,
			Vector3f min_pos,
			Vector3f max_pos
	) override;

private:
	void _on_noise_changed();
	void _on_curve_changed();

	static void _bind_methods();

private:
	Ref<Noise> _noise;
	Ref<Curve> _curve;

	struct Parameters {
		Ref<Noise> noise;
		Ref<Curve> curve;
	};

	Parameters _parameters;
	RWLock _parameters_lock;
};

} // namespace voxel

#endif // VOXEL_GENERATOR_NOISE_2D_H