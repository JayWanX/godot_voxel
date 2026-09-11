#ifndef VOXEL_BOX_MOVER_H
#define VOXEL_BOX_MOVER_H

#include "../../util/godot/classes/ref_counted.h"
#include "../../util/godot/macros.h"

class Node;

namespace voxel {

class VoxelData;
class VoxelMesher;

// 用于获取简单 AABB 物理的辅助类
class VoxelBoxMover : public RefCounted {
	GDCLASS(VoxelBoxMover, RefCounted)
public:
	// 计算物体在世界空间移动后的位置，并处理与地形的碰撞
	Vector3 get_motion(
			const Vector3 pos_world,
			const Vector3 motion_world,
			const AABB aabb_world,
			const VoxelData &terrain_data,
			const Transform3D &terrain_transform,
			const VoxelMesher &mesher
	);

	// 判断给定的 AABB 是否与地形相交
	bool intersects(
			const AABB aabb_world,
			const VoxelData &terrain_data,
			const Transform3D &terrain_transform,
			const VoxelMesher &mesher
	) const;

	// 碰撞掩码
	void set_collision_mask(uint32_t mask);
	inline uint32_t get_collision_mask() const {
		return _collision_mask;
	}

	// 是否启用台阶攀爬
	void set_step_climbing_enabled(bool enable);
	bool is_step_climbing_enabled() const;

	// 可攀爬的最大台阶高度
	void set_max_step_height(float height);
	float get_max_step_height() const;

	// 最近一次移动是否发生了攀爬
	bool has_stepped_up() const;

private:
	Vector3 _b_get_motion(Vector3 p_pos, Vector3 p_motion, AABB p_aabb, Node *p_terrain_node);

	bool _b_intersects(AABB p_aabb, Object *p_terrain_node) const;

	static void _bind_methods();

	// 配置
	uint32_t _collision_mask = 0xffffffff; // 全部
	bool _step_climbing_enabled = false;
	real_t _max_step_height = 0.5;

	// 状态
	bool _has_stepped_up = false;
};

} // namespace voxel

#endif // VOXEL_BOX_MOVER_H
