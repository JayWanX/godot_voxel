#ifndef VOXEL_SPOT_NOISE_H
#define VOXEL_SPOT_NOISE_H

#include "../math/conv.h"
#include "../math/interval.h"
#include "../math/vector2i.h"
#include "../math/vector3i.h"

namespace voxel::SpotNoise {

// 一种非常特殊的细胞噪声，用于在网格中生成"斑点"。典型用途是地形中的矿脉。
// 它有一些局限，但对于这个用途来说应该不会很明显。
// 这个实现应当基本是自包含的，也可以在 GLSL 中使用。

// 斑点噪声把空间划分为网格，每个格子包含一个位于随机位置的"斑点"。计算距离
// 以判断我们是否在斑点内部。与常见的细胞噪声不同，"斑点噪声"不查询相邻
// 格子，所以最大抖动会把斑点截断。不过，矿脉生成的用途使斑点非常稀疏，
// 所以我们可以把抖动减小到恰到好处。不必查询邻居使算法更快。总会存在
// 一些沿坐标轴的平面永远找不到斑点，但这通常不是问题，而且可以用一些
// 坐标位移来掩盖。

typedef Vector2i ivec2;
typedef Vector3i ivec3;
typedef Vector2f vec2;
typedef Vector3f vec3;

const int PRIME_X = 501125321;
const int PRIME_Y = 1136930381;
const int PRIME_Z = 1720413743;

// 派生自 FastNoiseLite 的细胞噪声。
inline int hash2(Vector2i p, int seed) {
	int hash = seed ^ (p.x * PRIME_X) ^ (p.y * PRIME_Y);
	hash *= 0x27d4eb2d;
	return hash;
}

// 派生自 FastNoiseLite 的细胞噪声。
inline int hash3(ivec3 p, int seed) {
	int hash = seed ^ (p.x * PRIME_X) ^ (p.y * PRIME_Y) ^ (p.z * PRIME_Z);
	hash *= 0x27d4eb2d;
	return hash;
}

// 独立的网格哈希函数将来可能会用到。暂时注释掉，因为它们还没被使用

// inline float grid_hash_2d(vec2 pos, float cell_size, int seed) {
// 	ivec2 pi = to_vec2i(math::floor(pos / cell_size));
// 	int h = hash2(pi, seed);
// 	return float(h & 0xffff) / 65535.0;
// }

// inline float grid_hash_3d(vec3 pos, float cell_size, int seed) {
// 	ivec3 pi = to_vec3i(math::floor(pos / cell_size));
// 	int h = hash3(pi, seed);
// 	return float(h & 0xffff) / 65535.0;
// }

// inline vec2 grid_hash_3d_x2(vec3 pos, float cell_size, int seed) {
// 	ivec3 pi = to_vec3i(math::floor(pos / cell_size));
// 	int h = hash3(pi, seed);
// 	return to_vec2f(ivec2(h, h >> 16) & 0xffff) / 65535.0;
// }

inline vec2 hash_to_vec2(int h) {
	// 每个轴上有 65536 个可能位置
	return to_vec2f(ivec2(h, h >> 16) & 0xffff) / 65535.0;
}

inline vec3 hash_to_vec3(int h) {
	// 每个轴上有 1024 个可能位置
	return to_vec3f(ivec3(h, h >> 10, h >> 20) & 0x3ff) / 1024.0;
}

inline Vector2f get_spot_position_2d_norm(Vector2i cell_position, float jitter, int seed) {
	int h = hash2(cell_position, seed);
	vec2 h2 = hash_to_vec2(h);
	return math::lerp(vec2(0.5), h2, jitter);
}

inline Vector3f get_spot_position_3d_norm(Vector3i cell_position, float jitter, int seed) {
	int h = hash3(cell_position, seed);
	vec3 h3 = hash_to_vec3(h);
	return math::lerp(vec3(0.5), h3, jitter);
}

inline float spot_noise_2d(vec2 pos, float cell_size, float spot_size, float jitter, int seed) {
	vec2 cell_origin_norm = math::floor(pos / cell_size);
	ivec2 cell_origin_norm_i = to_vec2i(cell_origin_norm);
	int h = hash2(cell_origin_norm_i, seed);
	vec2 h2 = hash_to_vec2(h);
	vec2 spot_pos_norm = math::lerp(vec2(0.5), h2, jitter);
	float ds = math::distance_squared((cell_origin_norm + spot_pos_norm) * cell_size, pos);
	return float(ds < spot_size * spot_size);
}

inline float spot_noise_3d(vec3 pos, float cell_size, float spot_size, float jitter, int seed) {
	vec3 cell_origin_norm = math::floor(pos / cell_size);
	ivec3 cell_origin_norm_i = to_vec3i(cell_origin_norm);
	int h = hash3(cell_origin_norm_i, seed);
	vec3 h3 = hash_to_vec3(h);
	vec3 spot_pos_norm = math::lerp(vec3(0.5), h3, jitter);
	float ds = math::distance_squared((cell_origin_norm + spot_pos_norm) * cell_size, pos);
	return float(ds < spot_size * spot_size);
}

inline bool box_intersects(Vector2f a_min, Vector2f a_max, Vector2f b_min, Vector2f b_max) {
	if (a_min.x >= b_max.x) {
		return false;
	}
	if (a_min.y >= b_max.y) {
		return false;
	}
	if (b_min.x >= a_max.x) {
		return false;
	}
	if (b_min.y >= a_max.y) {
		return false;
	}
	return true;
}

inline bool box_intersects(Vector3f a_min, Vector3f a_max, Vector3f b_min, Vector3f b_max) {
	if (a_min.x >= b_max.x) {
		return false;
	}
	if (a_min.y >= b_max.y) {
		return false;
	}
	if (a_min.z >= b_max.z) {
		return false;
	}
	if (b_min.x >= a_max.x) {
		return false;
	}
	if (b_min.y >= a_max.y) {
		return false;
	}
	if (b_min.z >= a_max.z) {
		return false;
	}
	return true;
}

inline math::Interval spot_noise_2d_range(
		math::Interval2 pos,
		float cell_size,
		math::Interval spot_size,
		float jitter,
		int seed
) {
	vec2 min_cell_origin_norm = math::floor(vec2(pos.x.min, pos.y.min) / cell_size);
	vec2 max_cell_origin_norm = math::floor(vec2(pos.x.max, pos.y.max) / cell_size);

	ivec2 min_cell_origin_norm_i = to_vec2i(min_cell_origin_norm);
	ivec2 max_cell_origin_norm_i = to_vec2i(max_cell_origin_norm);

	if (Vector2iUtil::get_area(max_cell_origin_norm_i - min_cell_origin_norm_i + ivec2(1, 1)) > 10) {
		// 不要费心检查太多格子，假定我们一定会与某个斑点相交。
		return math::Interval(0, 1);
	}

	vec2 box_size(pos.x.max - pos.x.min, pos.y.max - pos.y.min);
	if (math::min(box_size.x, box_size.y) >= 2.f * cell_size) {
		// 我们一定会与某个斑点相交。
		return math::Interval(0, 1);
	}

	// 检查所有与该区域相交的格子，看是否有斑点与它相交
	for (int yi = min_cell_origin_norm_i.y; yi <= max_cell_origin_norm_i.y; ++yi) {
		for (int xi = min_cell_origin_norm_i.x; xi <= max_cell_origin_norm_i.x; ++xi) {
			int h = hash2(ivec2(xi, yi), seed);
			vec2 h2 = hash_to_vec2(h);
			vec2 spot_pos_norm = math::lerp(vec2(0.5), h2, jitter);
			vec2 spot_pos = cell_size * (vec2(xi, yi) + spot_pos_norm);

			if (box_intersects(
						spot_pos - vec2(spot_size.max),
						spot_pos + vec2(spot_size.max),
						vec2(pos.x.min, pos.y.min),
						vec2(pos.x.max, pos.y.max)
				)) {
				return math::Interval(0, 1);
			}
		}
	}

	return math::Interval::from_single_value(0);
}

inline math::Interval spot_noise_3d_range(
		math::Interval3 pos,
		float cell_size,
		math::Interval spot_size,
		float jitter,
		int seed
) {
	vec3 min_cell_origin_norm = math::floor(vec3(pos.x.min, pos.y.min, pos.z.min) / cell_size);
	vec3 max_cell_origin_norm = math::floor(vec3(pos.x.max, pos.y.max, pos.z.max) / cell_size);

	ivec3 min_cell_origin_norm_i = to_vec3i(min_cell_origin_norm);
	ivec3 max_cell_origin_norm_i = to_vec3i(max_cell_origin_norm);

	if (Vector3iUtil::get_volume_u64(max_cell_origin_norm_i - min_cell_origin_norm_i + ivec3(1, 1, 1)) > 30) {
		// 不要费心检查太多格子，假定我们一定会与某个斑点相交。
		return math::Interval(0, 1);
	}

	vec3 box_size(pos.x.max - pos.x.min, pos.y.max - pos.y.min, pos.z.max - pos.z.min);
	if (math::min(box_size.x, math::min(box_size.y, box_size.z)) >= 2.f * cell_size) {
		// 我们一定会与某个斑点相交。
		return math::Interval(0, 1);
	}

	// 检查所有与该区域相交的格子，看是否有斑点与它相交
	for (int zi = min_cell_origin_norm_i.z; zi <= max_cell_origin_norm_i.z; ++zi) {
		for (int yi = min_cell_origin_norm_i.y; yi <= max_cell_origin_norm_i.y; ++yi) {
			for (int xi = min_cell_origin_norm_i.x; xi <= max_cell_origin_norm_i.x; ++xi) {
				int h = hash3(ivec3(xi, yi, zi), seed);
				vec3 h3 = hash_to_vec3(h);
				vec3 spot_pos_norm = math::lerp(vec3(0.5), h3, jitter);
				vec3 spot_pos = cell_size * (vec3(xi, yi, zi) + spot_pos_norm);

				if (box_intersects(
							spot_pos - vec3(spot_size.max),
							spot_pos + vec3(spot_size.max),
							vec3(pos.x.min, pos.y.min, pos.z.min),
							vec3(pos.x.max, pos.y.max, pos.z.max)
					)) {
					return math::Interval(0, 1);
				}
			}
		}
	}

	return math::Interval::from_single_value(0);
}

} // namespace voxel::SpotNoise

#endif // VOXEL_SPOT_NOISE_H
