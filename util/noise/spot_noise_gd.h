#ifndef VOXEL_SPOT_NOISE_GD_H
#define VOXEL_SPOT_NOISE_GD_H

#include "../godot/classes/resource.h"

namespace voxel {

class Voxel_SpotNoise : public Resource {
	GDCLASS(Voxel_SpotNoise, Resource);

public:
	// 随机种子
	int get_seed() const;
	void set_seed(int seed);

	// 单元格大小
	float get_cell_size() const;
	void set_cell_size(float cell_size);

	// 斑点半径
	float get_spot_radius() const;
	void set_spot_radius(float r);

	// 抖动强度
	float get_jitter() const;
	void set_jitter(float jitter);

	// 采样单个坐标点的 2D 斑点噪声
	float get_noise_2d(real_t x, real_t y) const;
	// 采样单个坐标点的 3D 斑点噪声
	float get_noise_3d(real_t x, real_t y, real_t z) const;

	// 采样单个坐标点的 2D 斑点噪声
	float get_noise_2dv(Vector2 pos) const;
	// 采样单个坐标点的 3D 斑点噪声
	float get_noise_3dv(Vector3 pos) const;

	// 获取 2D 区域内的斑点位置
	PackedVector2Array get_spot_positions_in_area_2d(Rect2 rect) const;
	// 获取 3D 区域内的斑点位置
	PackedVector3Array get_spot_positions_in_area_3d(AABB aabb) const;

private:
	static void _bind_methods();

	int _seed = 1337;
	float _cell_size = 32.f;
	float _spot_radius = 3.f;
	float _jitter = 0.9f;
};

}; // namespace voxel

#endif // VOXEL_SPOT_NOISE_GD_H