#ifndef VOXEL_VOXEL_RAYCAST_H
#define VOXEL_VOXEL_RAYCAST_H

#include "../util/math/vector3i.h"
// #include "../util/profiling.h"
#include "errors.h"
#include "math/conv.h"
#include "math/vector3.h"

namespace voxel {

// 穿过一个格时已知的数值。
// 射线从 A 到 B 所经过的格的示意图：
//
//    |       /|
//  --o------A-o--
//    |prev /  |
//    |    /   |
//    |   /    |
//  --o--B-----o--
//    |current |
//
struct VoxelRaycastState {
	// 我们来自的格的网格坐标。
	Vector3i hit_prev_position;
	// 沿射线进入前一个格时的距离
	float prev_distance;
	// 我们刚刚命中的格的网格坐标
	Vector3i hit_position;
	// 沿射线进入命中格时的距离
	float distance;
};

// 在三维中运行 DDA 算法。
template <typename Vec3f_T, typename Predicate_F> // f(VoxelRaycastState) -> bool
bool voxel_raycast(
		Vec3f_T ray_origin,
		Vec3f_T ray_direction,
		Predicate_F predicate,
		real_t max_distance,
		Vector3i &out_hit_pos,
		Vector3i &out_prev_pos,
		float &out_distance_along_ray,
		float &out_distance_along_ray_prev
) {
	// VOXEL_PROFILE_SCOPE();

	VOXEL_ASSERT_RETURN_V(!math::has_nan(ray_origin), false);
	VOXEL_ASSERT_RETURN_V(!math::has_nan(ray_direction), false);
	VOXEL_ASSERT_RETURN_V(!math::is_nan(max_distance), false);

	const float g_infinite = 9999999;

	// 方程：p + v*t
	// p：射线起点位置（ray.pos）
	// v：射线方向向量（ray.dir）
	// t：参数变量，若 v 已归一化则等于一个距离

	// 该射线投射技术在此处描述：
	// http://www.cse.yorku.ca/~amana/research/grid.pdf

	// 另见 https://www.youtube.com/watch?v=NbSee-XM7WA

	// 注意：网格假定为边长 1 单位的方格。

#ifdef DEBUG_ENABLED
	VOXEL_ASSERT_RETURN_V(math::is_normalized(ray_direction), false); // 必须已归一化
#endif

	/* 初始化 */

	// 体素位置
	Vector3i hit_pos = math::floor_to_int(ray_origin);
	Vector3i hit_prev_pos = hit_pos;

	// 体素步进
	const int xi_step = ray_direction.x > 0 ? 1 : ray_direction.x < 0 ? -1 : 0;
	const int yi_step = ray_direction.y > 0 ? 1 : ray_direction.y < 0 ? -1 : 0;
	const int zi_step = ray_direction.z > 0 ? 1 : ray_direction.z < 0 ? -1 : 0;

	// 参数化体素步进
	const real_t tdelta_x = xi_step != 0 ? 1.f / Math::abs(ray_direction.x) : g_infinite;
	const real_t tdelta_y = yi_step != 0 ? 1.f / Math::abs(ray_direction.y) : g_infinite;
	const real_t tdelta_z = zi_step != 0 ? 1.f / Math::abs(ray_direction.z) : g_infinite;

	// 参数化网格穿越
	real_t tcross_x; // 在哪个 T 值时我们将穿过一条竖线？
	real_t tcross_y; // 在哪个 T 值时我们将穿过一条水平线？
	real_t tcross_z; // 在哪个 T 值时我们将穿过一条深度线？

	// X 初始化
	if (xi_step != 0) {
		if (xi_step == 1) {
			tcross_x = (Math::ceil(ray_origin.x) - ray_origin.x) * tdelta_x;
		} else {
			tcross_x = (ray_origin.x - Math::floor(ray_origin.x)) * tdelta_x;
		}
	} else {
		tcross_x = g_infinite; // 在 X 上永远不会穿越
	}

	// Y 初始化
	if (yi_step != 0) {
		if (yi_step == 1) {
			tcross_y = (Math::ceil(ray_origin.y) - ray_origin.y) * tdelta_y;
		} else {
			tcross_y = (ray_origin.y - Math::floor(ray_origin.y)) * tdelta_y;
		}
	} else {
		tcross_y = g_infinite; // 在 X 上永远不会穿越
	}

	// Z 初始化
	if (zi_step != 0) {
		if (zi_step == 1) {
			tcross_z = (Math::ceil(ray_origin.z) - ray_origin.z) * tdelta_z;
		} else {
			tcross_z = (ray_origin.z - Math::floor(ray_origin.z)) * tdelta_z;
		}
	} else {
		tcross_z = g_infinite; // 在 X 上永远不会穿越
	}

	// 针对整数位置的变通处理
	// 改编自 https://github.com/bulletphysics/bullet3/blob/3dbe5426bf7387e532c17df9a1c5e5a4972c298a/src/
	// BulletCollision/CollisionShapes/btHeightfieldTerrainShape.cpp#L418
	if (tcross_x == 0.0) {
		tcross_x += tdelta_x;
		// 如果往回走，我们应忽略上述向下取整会得到的位置，
		// 因为射线并非朝那个方向前进
		if (xi_step == -1) {
			hit_pos.x -= 1;
		}
	}

	if (tcross_y == 0.0) {
		tcross_y += tdelta_y;
		if (yi_step == -1) {
			hit_pos.y -= 1;
		}
	}

	if (tcross_z == 0.0) {
		tcross_z += tdelta_z;
		if (zi_step == -1) {
			hit_pos.z -= 1;
		}
	}

	/* 迭代 */

	float t = 0.f;
	float t_prev = 0.f;

	do {
		hit_prev_pos = hit_pos;
		t_prev = t;
		if (tcross_x < tcross_y) {
			if (tcross_x < tcross_z) {
				// X 碰撞
				// hit.prevPos.x = hit.pos.x;
				hit_pos.x += xi_step;
				if (tcross_x > max_distance) {
					return false;
				}
				t = tcross_x;
				tcross_x += tdelta_x;
			} else {
				// Z 碰撞（重复代码）
				// hit.prevPos.z = hit.pos.z;
				hit_pos.z += zi_step;
				if (tcross_z > max_distance) {
					return false;
				}
				t = tcross_z;
				tcross_z += tdelta_z;
			}
		} else {
			if (tcross_y < tcross_z) {
				// Y 碰撞
				// hit.prevPos.y = hit.pos.y;
				hit_pos.y += yi_step;
				if (tcross_y > max_distance) {
					return false;
				}
				t = tcross_y;
				tcross_y += tdelta_y;
			} else {
				// Z 碰撞（重复代码）
				// hit.prevPos.z = hit.pos.z;
				hit_pos.z += zi_step;
				if (tcross_z > max_distance) {
					return false;
				}
				t = tcross_z;
				tcross_z += tdelta_z;
			}
		}

	} while (!predicate({ hit_prev_pos, t_prev, hit_pos, t }));

	out_hit_pos = hit_pos;
	out_prev_pos = hit_prev_pos;
	out_distance_along_ray = t;
	out_distance_along_ray_prev = t_prev;

	return true;
}

} // namespace voxel

#endif // VOXEL_VOXEL_RAYCAST_H
