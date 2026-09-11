#include "mesh_sdf.h"
#include "../util/math/box3i.h"
#include "../util/math/conv.h"
#include "../util/math/triangle.h"
#include "../util/math/vector3d.h"
#include "../util/profiling.h"
#include "../util/string/format.h" // 调试
#include "../util/voxel_raycast.h"

// 调试
// #define VOXEL_MESH_SDF_DEBUG_SLICES
#ifdef VOXEL_MESH_SDF_DEBUG_SLICES
#include <core/math/color.h>
#include <core/io/image.h>
#endif
// #define VOXEL_MESH_SDF_DEBUG_BATCH
#ifdef VOXEL_MESH_SDF_DEBUG_BATCH
#include "../ddd.h"
#endif

namespace voxel::mesh_sdf {

// 一些可供后续改进参考的论文
// Jump flood（跳洪算法）
// https://www.comp.nus.edu.sg/%7Etants/jfa/i3d06.pdf
// 基于 GPU 的技术
// https://www.researchgate.net/profile/Bastian-Krayer/publication/332921884_Generating_signed_distance_fields_on_the_GPU_with_ray_maps/links/5d63d921299bf1f70b0de26b/Generating-signed-distance-fields-on-the-GPU-with-ray-maps.pdf
// 未来或许还可以顺便收集梯度，以便与 Dual Contouring（双轮廓）高效结合使用？

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 首先尝试使用 float，因为数据为 float 时是最常见的情况。
// 如果偶尔出现并行结果，则改用 double 重试，这通常能解决问题。
math::TriangleIntersectionResult ray_intersects_triangle2(
		const Vector3f &p_from,
		const Vector3f &p_dir,
		const Vector3f &p_v0,
		const Vector3f &p_v1,
		const Vector3f &p_v2
) {
	math::TriangleIntersectionResult r = math::ray_intersects_triangle(p_from, p_dir, p_v0, p_v1, p_v2);
	if (r.case_id == math::TriangleIntersectionResult::PARALLEL) {
		r = math::ray_intersects_triangle(
				to_vec3d(p_from), to_vec3d(p_dir), to_vec3d(p_v0), to_vec3d(p_v1), to_vec3d(p_v2)
		);
	}
	return r;
}

inline Vector3f get_normal(const Triangle &t) {
	return math::cross(t.v13, math::normalized(t.v1 - t.v2));
}

// const Triangle *raycast(Span<const Triangle> triangles, Vector3f ray_position, Vector3f ray_dir) {
// 	float min_d = 999999.f;
// 	const Triangle *selected_triangle = nullptr;

// 	for (unsigned int i = 0; i < triangles.size(); ++i) {
// 		const Triangle &t = triangles[i];
// 		const TriangleIntersectionResult r = ray_intersects_triangle2(ray_position, ray_dir, t.v1, t.v2, t.v3);
// 		if (r.case_id == TriangleIntersectionResult::INTERSECTION) {
// 			if (r.distance < min_d) {
// 				min_d = r.distance;
// 				selected_triangle = &t;
// 			}
// 		}
// 	}

// 	return selected_triangle;
// }

const Triangle *raycast(Span<const Triangle *const> triangles, Vector3f ray_position, Vector3f ray_dir, float &out_d) {
	float min_d = 999999.f;
	const Triangle *selected_triangle = nullptr;

	for (unsigned int i = 0; i < triangles.size(); ++i) {
		const Triangle &t = *triangles[i];
		const math::TriangleIntersectionResult r = ray_intersects_triangle2(ray_position, ray_dir, t.v1, t.v2, t.v3);
		if (r.case_id == math::TriangleIntersectionResult::INTERSECTION) {
			if (r.distance < min_d) {
				min_d = r.distance;
				selected_triangle = &t;
			}
		}
	}

	out_d = min_d;
	return selected_triangle;
}

inline bool is_valid_grid_position(const Vector3i &pos, const Vector3i &size) {
	return pos.x >= 0 && pos.y >= 0 && pos.z >= 0 && pos.x < size.x && pos.y < size.y && pos.z < size.z;
}

const Triangle *raycast(const ChunkGrid &chunk_grid, Vector3f ray_position, Vector3f ray_dir, float max_distance) {
	const Vector3f cposf = (ray_position - chunk_grid.min_pos) / chunk_grid.chunk_size;

	// 为 DDA 计算最大距离
	// const float max_distance_chunks = int(chunk_grid.size.length());
	const float max_distance_chunks = (max_distance / chunk_grid.chunk_size) + 2.f * math::SQRT3<float>;

	const Triangle *hit_triangle = nullptr;

	struct RaycastChunk {
		const ChunkGrid &chunk_grid;
		const Vector3f ray_position;
		const Vector3f ray_dir;
		const float max_distance;
		const Triangle *&hit_triangle;

		inline bool operator()(const VoxelRaycastState &rs) const {
			if (!is_valid_grid_position(rs.hit_prev_position, chunk_grid.size)) {
				return false;
			}

#ifdef VOXEL_MESH_SDF_DEBUG_BATCH
			DDD::draw_wirebox_min_max( //
					to_vec3f(cpos) * chunk_grid.chunk_size + chunk_grid.min_pos,
					to_vec3f(cpos + Vector3i(1, 1, 1)) * chunk_grid.chunk_size + chunk_grid.min_pos,
					Color(0, 1, 1)
			);
#endif
			const unsigned int loc = Vector3iUtil::get_zxy_index(rs.hit_prev_position, chunk_grid.size);
			VOXEL_ASSERT(loc < chunk_grid.chunks.size());
			const Chunk &chunk = chunk_grid.chunks[loc];
			Span<const Triangle *const> tris = to_span(chunk.triangles);

			float hit_distance;
			hit_triangle = raycast(tris, ray_position, ray_dir, hit_distance);

			// 射线有可能击中 DDA 盒之外的三角形（因为这样的三角形可能部分
			// 与该盒相交），也可能超出我们的最大目标（当 DDA 尚未到达包含
			// 最终三角形的盒时）。如果发生这种情况，将其钳制到最终三角形距离与 DDA 盒两者之间。
			if (hit_distance > math::min(max_distance, rs.distance)) {
				hit_triangle = nullptr;
			}

#ifdef VOXEL_MESH_SDF_DEBUG_BATCH
			for (unsigned int i = 0; i < tris.size(); ++i) {
				const Triangle *t = tris[i];
				DDD::draw_triangle(t->v1, t->v2, t->v3, t == hit_triangle ? Color(1, 0, 0) : Color(1, 1, 0));

				if (t == hit_triangle) {
					VOXEL_ASSERT(t != nullptr);
					const Vector3f normal = get_normal(*hit_triangle);
					const Vector3f center = (t->v1 + t->v2 + t->v3) / 3.f;
					DDD::draw_line(center, center + normal * 0.1, Color(0, 1, 1));
				}
			}

			DDD::batch_mark_segment();
#endif
			return hit_triangle != nullptr;
		}
	};

	// TODO 优化：如果初始块包含三角形，选取其中一个作为参考。
	// 若为空则选默认的。这样可以减少大量被检查的三角形。

	RaycastChunk raycast_chunk{ chunk_grid, ray_position, ray_dir, max_distance, hit_triangle };

	// if (!raycast_chunk(math::floor_to_int(cposf))) {
	Vector3i hit_chunk_pos;
	Vector3i hit_prev_chunk_pos;
	float distance_along_ray;
	float distance_along_ray_prev;
	if (!voxel_raycast(
				cposf,
				ray_dir,
				raycast_chunk,
				max_distance_chunks,
				hit_chunk_pos,
				hit_prev_chunk_pos,
				distance_along_ray,
				distance_along_ray_prev
		)) {
		// 当需要行进的距离小于一个块时，voxel_raycast 不会“进入”任何块并返回
		// false。注意，`hit_position` 未被使用。
		raycast_chunk({ math::floor_to_int(cposf), 0.f, Vector3i(), max_distance });
	}
	// }

	return hit_triangle;
}

bool find_sdf_sign_with_raycast(
		const ChunkGrid &chunk_grid,
		Vector3f ray_position,
		const Triangle &ref_triangle,
		int &out_sign
) {
	// VOXEL_PROFILE_SCOPE();

	const Vector3f ref_center = (ref_triangle.v1 + ref_triangle.v2 + ref_triangle.v3) / 3.f;
	const Vector3f ray_dir = math::normalized(ref_center - ray_position);

#ifdef VOXEL_MESH_SDF_DEBUG_BATCH
	DDD::draw_line(ray_position, ray_position + ray_dir * 10.f, Color(0, 1, 0));
	DDD::draw_triangle(ref_triangle.v1, ref_triangle.v2, ref_triangle.v3, Color(0.5, 0, 0));
#endif

	// 射线可以行进的最大距离，留有一些余量以允许击中最终三角形。
	const float max_distance = math::distance(ref_center, ray_position) * 1.01;

	const Triangle *selected_triangle = raycast(chunk_grid, ray_position, ray_dir, max_distance);

	if (selected_triangle == nullptr) {
#if DEBUG_ENABLED
		// 这种情况通常不应发生，除非 `ref_triangle` 恰好与射线完全平行
		static bool s_tri_not_found_error = false;
		if (s_tri_not_found_error == false) {
			s_tri_not_found_error = true;
			VOXEL_PRINT_VERBOSE(
					format("Could not find triangle by raycast, dp: {}", math::dot(get_normal(ref_triangle), ray_dir))
			);
		}
#endif
		return false;
	}

	const Vector3f triangle_normal = get_normal(*selected_triangle);
	const float dp = math::dot(triangle_normal, ray_dir);
	// 同向（+）：我们在内部，符号为负
	// 反向（-）：我们在外部，符号为正
	out_sign = dp < 0.f ? 1 : -1;
	return true;
}

/*bool find_sdf_sign_with_raycast(Span<const Triangle> triangles, Vector3f ray_position, const Triangle &ref_triangle) {
	//VOXEL_PROFILE_SCOPE();

	const Vector3f ref_center = (ref_triangle.v1 + ref_triangle.v2 + ref_triangle.v3) / 3.f;
	const Vector3f ray_dir = (ref_center - ray_position).normalized();

	const Triangle *selected_triangle = raycast(triangles, ray_position, ray_dir);
	if (selected_triangle == nullptr) {
#if DEBUG_ENABLED
		// 如果这种罕见情况发生，我们可以通过不断另选一个参考三角形，直到命中为止来规避它
		static bool s_tri_not_found_error = false;
		if (s_tri_not_found_error == false) {
			s_tri_not_found_error = true;
			VOXEL_PRINT_ERROR("Could not find triangle by raycast");
		}
#endif
		selected_triangle = &ref_triangle;
	}

	const Vector3f triangle_normal = get_normal(*selected_triangle);
	const float dp = triangle_normal.dot(ray_dir);
	// 相同方向（+）：我们在内部，符号为负
	// 相反方向（-）：我们在外部，符号为正
	return dp < 0.f;
}*/

float get_max_sdf_variation(Vector3f min_pos, Vector3f max_pos, Vector3i res) {
	const Vector3f box_size = max_pos - min_pos;
	const Vector3f max_variation_v = box_size / to_vec3f(res);
	return math::max(max_variation_v.x, math::max(max_variation_v.y, max_variation_v.z));
}

enum Flag { //
	FLAG_NOT_VISITED,
	FLAG_VISITED,
	FLAG_FROZEN
};

void fix_sdf_sign_from_boundary(
		Span<float> sdf_grid,
		Span<uint8_t> flag_grid,
		Vector3i res,
		Vector3f min_pos,
		Vector3f max_pos,
		StdVector<Vector3i> &seeds
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(sdf_grid.size() == flag_grid.size());

	if (res.x == 0 || res.y == 0 || res.z == 0) {
		return;
	}

	FixedArray<Vector3i, 6> dirs;
	dirs[0] = Vector3i(-1, 0, 0);
	dirs[1] = Vector3i(1, 0, 0);
	dirs[2] = Vector3i(0, -1, 0);
	dirs[3] = Vector3i(0, 1, 0);
	dirs[4] = Vector3i(0, 0, -1);
	dirs[5] = Vector3i(0, 0, 1);

	const float tolerance = 1.1f;
	const float max_variation = tolerance * get_max_sdf_variation(min_pos, max_pos, res);
	const float min_sd = max_variation * 2.f;

	// 从边界向外扩散正符号。
	// 还有一点优化空间，但性能分析显示与计算距离场相比，它只占很小一部分时间。
	while (seeds.size() > 0) {
		const Vector3i pos = seeds.back();
		seeds.pop_back();
		const unsigned int loc = Vector3iUtil::get_zxy_index(pos, res);
		const float v = sdf_grid[loc];

		for (unsigned int dir = 0; dir < dirs.size(); ++dir) {
			const Vector3i npos = pos + dirs[dir];

			if (npos.x < 0 || npos.y < 0 || npos.z < 0 || npos.x >= res.x || npos.y >= res.y || npos.z >= res.z) {
				continue;
			}

			const unsigned int nloc = Vector3iUtil::get_zxy_index(npos, res);

			VOXEL_ASSERT(nloc < flag_grid.size());
			const uint8_t flag = flag_grid[nloc];
			if (flag == FLAG_VISITED) {
				continue;
			}

			flag_grid[nloc] = FLAG_VISITED;

			// VOXEL_ASSERT(nloc < sdf_grid.size());
			const float nv = sdf_grid[nloc];

			if ((nv > 0.f && nv < min_sd) || ((nv > 0.f) != (v > 0.f) && Math::abs(nv - v) < max_variation)) {
				// 距离外表面太近，或发生了合理的符号变化。
				// 如果我们继续在靠近表面或符号在低距离处翻转的位置进行洪泛填充，
				// 就可能颠倒形状内部的符号，从而破坏有符号性。
				// 但如果符号翻转伴随着较大的距离变化，我们肯定要修正它。
				continue;
			}

			if (nv < 0.f) {
				sdf_grid[nloc] = -nv;
			}
			seeds.push_back(npos);
		}
	}
}

void fix_sdf_sign_from_boundary(Span<float> sdf_grid, Vector3i res, Vector3f min_pos, Vector3f max_pos) {
	StdVector<uint8_t> flag_grid;
	flag_grid.resize(sdf_grid.size(), FLAG_NOT_VISITED);
	// 我们将从较低的角落开始
	flag_grid[0] = FLAG_VISITED;

	StdVector<Vector3i> seeds;
	seeds.push_back(Vector3i());

	fix_sdf_sign_from_boundary(sdf_grid, to_span(flag_grid), res, min_pos, max_pos, seeds);
}

void compute_near_chunks(ChunkGrid &chunk_grid) {
	VOXEL_PROFILE_SCOPE();

	// 初始化块位置
	{
		Vector3i cpos;
		for (cpos.z = 0; cpos.z < chunk_grid.size.z; ++cpos.z) {
			for (cpos.x = 0; cpos.x < chunk_grid.size.x; ++cpos.x) {
				cpos.y = 0;
				unsigned int ci = Vector3iUtil::get_zxy_index(cpos, chunk_grid.size);

				for (; cpos.y < chunk_grid.size.y; ++cpos.y) {
					VOXEL_ASSERT(ci < chunk_grid.chunks.size());
					Chunk &chunk = chunk_grid.chunks[ci];

					chunk.pos = cpos;
					++ci;
				}
			}
		}
	}

	// 收集包含三角形的块
	StdVector<const Chunk *> nonempty_chunks;
	for (auto it = chunk_grid.chunks.begin(); it != chunk_grid.chunks.end(); ++it) {
		const Chunk &chunk = *it;
		if (chunk.triangles.size() > 0) {
			nonempty_chunks.push_back(&chunk);
		}
	}

	// TODO 优化：如果许多块都包含三角形，这实际上非常慢。
	// 遗憾的是，这意味着像二十面体这样的简单形状也会变得非常慢。
	// 因此对于此类情况，其他烘焙模式更合适。

	Vector3i cpos;
	for (cpos.z = 0; cpos.z < chunk_grid.size.z; ++cpos.z) {
		for (cpos.x = 0; cpos.x < chunk_grid.size.x; ++cpos.x) {
			cpos.y = 0;
			unsigned int ci = Vector3iUtil::get_zxy_index(cpos, chunk_grid.size);

			for (; cpos.y < chunk_grid.size.y; ++cpos.y, ++ci) {
				VOXEL_ASSERT(ci < chunk_grid.chunks.size());
				Chunk &chunk = chunk_grid.chunks[ci];

				// 找到最近的块

				const Chunk *closest_chunk = nullptr;
				// 距离以块为单位
				int closest_chunk_distance_squared = 0x0fffffff;

				if (chunk.triangles.size() > 0) {
					closest_chunk = &chunk;
					closest_chunk_distance_squared = 0;

				} else {
					for (auto it = nonempty_chunks.begin(); it != nonempty_chunks.end(); ++it) {
						const Chunk &nchunk = **it;
						const int distance_squared = (nchunk.pos - chunk.pos).length_squared();
						if (distance_squared < closest_chunk_distance_squared) {
							closest_chunk = &nchunk;
							closest_chunk_distance_squared = distance_squared;
						}
					}
				}

				VOXEL_ASSERT(closest_chunk != nullptr);

				// 寻找比最近块稍远一点的其他邻近块。
				// 这是为了应对最近块中的三角形可能比更远块中的三角形距离更远的情况。

				// TODO 当网格包含较少/较大的三角形且细分度较高时，这会产生伪影。
				// 对角三角形 D 的 AABB 可能很大，与许多块相交，但并未真正靠近它们。
				// 然而在 2 个块之外（>sqrt(3)）可能有一个由轴对齐三角形 A 相交的块，它比 D 更近，但不会被检测到。
				//
				// 要修复此问题，我们可以：
				// - 增大添加更多三角形时的余量？这可能有效但会降低效率。
				// - 与其用 AABB 判断三角形是否在块内，不如尝试盒子/三角形相交检测，
				//   或 3D 光栅化。这样我们就可以继续使用 sqrt(3) 余量，因为只要知道三角形在某个块内，
				//   从该块边上任取一点到三角形的距离总是小于该块的对角线。

				const int margin_distance_squared =
						math::squared(sqrtf(closest_chunk_distance_squared) + math::SQRT3<float>);

				for (auto it = nonempty_chunks.begin(); it != nonempty_chunks.end(); ++it) {
					const Chunk &nchunk = **it;
					const int distance_squared = (nchunk.pos - chunk.pos).length_squared();
					// 注意，这将包含最近的块
					if (distance_squared <= margin_distance_squared) {
						chunk.near_chunks.push_back(&nchunk);
					}
				}

				// println(format("Chunk {} has {} / {} near non-empty chunks", chunk.pos, chunk.near_chunks.size(),
				// 		nonempty_chunks.size()));
			}
		}
	}
}

void partition_triangles(
		int subdiv,
		Span<const Triangle> triangles,
		Vector3f min_pos,
		Vector3f max_pos,
		ChunkGrid &chunk_grid
) {
	VOXEL_PROFILE_SCOPE();

	// TODO 这很少导致 SDF 错误，但还不确定具体原因

	const Vector3f mesh_size = max_pos - min_pos;
	const float chunk_size = math::max(mesh_size.x, math::max(mesh_size.y, mesh_size.z)) / subdiv;
	const Vector3i grid_min = to_vec3i(math::floor(min_pos / chunk_size));
	const Vector3i grid_max = to_vec3i(math::ceil(max_pos / chunk_size));

	chunk_grid.size = grid_max - grid_min;
	chunk_grid.chunks.resize(Vector3iUtil::get_volume_u64(chunk_grid.size));
	chunk_grid.chunk_size = chunk_size;
	chunk_grid.min_pos = to_vec3f(grid_min) * chunk_size;

	const Vector3f margin(chunk_size * 0.01f);

	// 将重叠块的三角形分组
	{
		VOXEL_PROFILE_SCOPE_NAMED("Group triangles");

		for (unsigned int triangle_index = 0; triangle_index < triangles.size(); ++triangle_index) {
			const Triangle &t = triangles[triangle_index];
			// TODO 优化：三角形-盒子相交检测可能带来更好的分区

			const Vector3f tri_min_pos = math::min(t.v1, math::min(t.v2, t.v3)) - margin;
			const Vector3f tri_max_pos = math::max(t.v1, math::max(t.v2, t.v3)) + margin;

			const Vector3i tri_min_pos_grid = to_vec3i(math::floor((tri_min_pos - chunk_grid.min_pos) / chunk_size));
			const Vector3i tri_max_pos_grid = to_vec3i(math::floor((tri_max_pos - chunk_grid.min_pos) / chunk_size));

			// 调试
			// const Vector3f chunk_min_pos = to_vec3f(tri_min_pos_grid) * chunk_grid.chunk_size + chunk_grid.min_pos;
			// const Vector3f chunk_max_pos =
			// 		to_vec3f(tri_max_pos_grid + Vector3i(1, 1, 1)) * chunk_grid.chunk_size + chunk_grid.min_pos;
			// VOXEL_ASSERT(!(tri_min_pos.x < chunk_min_pos.x));
			// VOXEL_ASSERT(!(tri_min_pos.y < chunk_min_pos.y));
			// VOXEL_ASSERT(!(tri_min_pos.z < chunk_min_pos.z));
			// VOXEL_ASSERT(!(tri_max_pos.x > chunk_max_pos.x));
			// VOXEL_ASSERT(!(tri_max_pos.y > chunk_max_pos.y));
			// VOXEL_ASSERT(!(tri_max_pos.z > chunk_max_pos.z));

			Vector3i cpos;
			for (cpos.z = tri_min_pos_grid.z; cpos.z <= tri_max_pos_grid.z; ++cpos.z) {
				for (cpos.x = tri_min_pos_grid.x; cpos.x <= tri_max_pos_grid.x; ++cpos.x) {
					cpos.y = tri_min_pos_grid.y;
					unsigned int ci = Vector3iUtil::get_zxy_index(cpos, chunk_grid.size);

					for (; cpos.y <= tri_max_pos_grid.y; ++cpos.y) {
						VOXEL_ASSERT(ci < chunk_grid.chunks.size());
						Chunk &chunk = chunk_grid.chunks[ci];
						chunk.triangles.push_back(&t);
						++ci;
					}
				}
			}
		}
	}

#ifdef DEBUG_ENABLED
	{
		// 确保所有三角形都被拾取
		StdVector<const Triangle *> checked_triangles;
		for (const Chunk &chunk : chunk_grid.chunks) {
			for (const Triangle *t : chunk.triangles) {
				bool found = false;
				for (const Triangle *ct : checked_triangles) {
					if (ct == t) {
						found = true;
						break;
					}
				}
				if (!found) {
					checked_triangles.push_back(t);
				}
			}
		}
		VOXEL_ASSERT(checked_triangles.size() == triangles.size());
	}
#endif
}

/*
// 未优化的版本，适用于单次查询
float get_distance_to_triangle_squared(const Vector3f v1, const Vector3f v2, const Vector3f v3, const Vector3f p) {
	// https://iquilezles.org/articles/triangledistance/

	const Vector3f v21 = v2 - v1;
	const Vector3f v32 = v3 - v2;
	const Vector3f v13 = v1 - v3;

	const Vector3f p1 = p - v1;
	const Vector3f p2 = p - v2;
	const Vector3f p3 = p - v3;

	const Vector3f nor = v21.cross(v13);

	const float det = //
			signf(v21.cross(nor).dot(p1)) + //
			signf(v32.cross(nor).dot(p2)) + //
			signf(v13.cross(nor).dot(p3));

	if (det < 2.f) {
		// 在棱柱之外：取到最近边的距离
		return math::min(
				math::min( //
						(v21 * math::clamp(v21.dot(p1) / v21.length_squared(), 0.f, 1.f) - p1).length_squared(),
						(v32 * math::clamp(v32.dot(p2) / v32.length_squared(), 0.f, 1.f) - p2).length_squared()),
				(v13 * math::clamp(v13.dot(p3) / v13.length_squared(), 0.f, 1.f) - p3).length_squared());
	} else {
		// 在棱柱之内：取到平面的距离
		return math::squared(nor.dot(p1)) / nor.length_squared();
	}
}
*/

// 返回点到三角形的距离，其中部分项已预先计算。
// 如果同一三角形需要被多次查询，这可能更受青睐。
float get_distance_to_triangle_squared_precalc(const Triangle &t, const Vector3f p) {
	// https://iquilezles.org/articles/triangledistance/

	using namespace math;

	const Vector3f p1 = p - t.v1;
	const Vector3f p2 = p - t.v2;
	const Vector3f p3 = p - t.v3;

	const float det = //
			sign_nonzero(dot(t.v21_cross_nor, p1)) + //
			sign_nonzero(dot(t.v32_cross_nor, p2)) + //
			sign_nonzero(dot(t.v13_cross_nor, p3));

	if (det < 2.f) {
		// 在棱柱外：获取到最近边的距离
		return min(
				min( //
						length_squared(t.v21 * clamp(dot(t.v21, p1) * t.inv_v21_length_squared, 0.f, 1.f) - p1),
						length_squared(t.v32 * clamp(dot(t.v32, p2) * t.inv_v32_length_squared, 0.f, 1.f) - p2)
				),
				length_squared(t.v13 * clamp(dot(t.v13, p3) * t.inv_v13_length_squared, 0.f, 1.f) - p3)
		);
	} else {
		// 在棱柱内：获取到平面的距离
		return squared(dot(t.nor, p1)) * t.inv_nor_length_squared;
	}
}

void precalc_triangles(Span<Triangle> triangles) {
	VOXEL_PROFILE_SCOPE();
	for (size_t i = 0; i < triangles.size(); ++i) {
		Triangle &t = triangles[i];
		t.v21 = t.v2 - t.v1;
		t.v32 = t.v3 - t.v2;
		t.v13 = t.v1 - t.v3;
		t.nor = math::cross(t.v21, t.v13);
		t.v21_cross_nor = math::cross(t.v21, t.nor);
		t.v32_cross_nor = math::cross(t.v32, t.nor);
		t.v13_cross_nor = math::cross(t.v13, t.nor);
		t.inv_v21_length_squared = 1.f / math::length_squared(t.v21);
		t.inv_v32_length_squared = 1.f / math::length_squared(t.v32);
		t.inv_v13_length_squared = 1.f / math::length_squared(t.v13);
		t.inv_nor_length_squared = 1.f / math::length_squared(t.nor);
	}
}

unsigned int get_closest_triangle_precalc(const Vector3f pos, Span<const Triangle> triangles) {
	float min_distance_squared = 9999999.f;
	size_t closest_tri_index = triangles.size();

	for (size_t i = 0; i < triangles.size(); ++i) {
		const Triangle &t = triangles[i];
		// const float sqd = get_distance_to_triangle_squared(t.v1, t.v2, t.v3, pos);
		const float sqd = get_distance_to_triangle_squared_precalc(t, pos);

		if (sqd < min_distance_squared) {
			min_distance_squared = sqd;
			closest_tri_index = i;
		}
	}

	return closest_tri_index;
}

float get_mesh_signed_distance_at(const Vector3f pos, Span<const Triangle> triangles /*, bool debug = false*/) {
	float min_distance_squared = 9999999.f;
	size_t closest_tri_index = 0;

	for (size_t i = 0; i < triangles.size(); ++i) {
		const Triangle &t = triangles[i];
		// const float sqd = get_distance_to_triangle_squared(t.v1, t.v2, t.v3, pos);
		const float sqd = get_distance_to_triangle_squared_precalc(t, pos);

		// TODO 如果两个方向相反的三角形共享同一个点会怎样？
		// 如果距离来自该点，就会使符号的确定变得模糊。
		// 有时两个方向相反的三角形共享一条边或一个点，没有快速的方法
		// 判断必须取哪一个。
		// 目前这通过在后续的洪泛填充阶段来规避。

		/*if (debug) {
			if (sqd < min_distance_squared + 0.01f) {
				const Vector3f plane_normal = t.v13.cross(t.v1 - t.v2).normalized();
				const float plane_d = plane_normal.dot(t.v1);
				float s;
				if (plane_normal.dot(pos) > plane_d) {
					s = 1.f;
				} else {
					s = -1.f;
				}
				println(format("Candidate tri {} with sqd {} sign {}", i, sqd, s));
			}
		}*/

		if (sqd < min_distance_squared) {
			/*if (debug) {
				println(format("Better tri {} with sqd {}", i, sqd));
			}*/
			min_distance_squared = sqd;
			closest_tri_index = i;
		}
	}

	const Triangle &ct = triangles[closest_tri_index];
	const float d = Math::sqrt(min_distance_squared);

	// if (p_dir == CLOCKWISE) {
	// const Vector3f plane_normal = (ct.v1 - ct.v3).cross(ct.v1 - ct.v2).normalized();
	const Vector3f plane_normal = get_normal(ct);
	//} else {
	//	normal = (p_point1 - p_point2).cross(p_point1 - p_point3);
	//}
	const float plane_d = math::dot(plane_normal, ct.v1);

	if (math::dot(plane_normal, pos) > plane_d) {
		return d;
	}
	return -d;
}

float get_mesh_signed_distance_at(const Vector3f pos, const ChunkGrid &chunk_grid) {
	float min_distance_squared = 9999999.f;
	const Triangle *closest_tri = nullptr;

	const Vector3i chunk_pos = to_vec3i(math::floor((pos - chunk_grid.min_pos) / chunk_grid.chunk_size));
	const unsigned chunk_index = Vector3iUtil::get_zxy_index(chunk_pos, chunk_grid.size);
	VOXEL_ASSERT(chunk_index < chunk_grid.chunks.size());
	const Chunk &chunk = chunk_grid.chunks[chunk_index];

	for (auto near_chunk_it = chunk.near_chunks.begin(); near_chunk_it != chunk.near_chunks.end(); ++near_chunk_it) {
		const Chunk &near_chunk = **near_chunk_it;

		for (auto tri_it = near_chunk.triangles.begin(); tri_it != near_chunk.triangles.end(); ++tri_it) {
			const Triangle &t = **tri_it;

			// TODO 如果两个方向相反的三角形共用同一个点，会怎样？
			// 如果距离来自那个点，会使符号判定变得不明确。
			// 有时两个方向相反的三角形会共用一条边或一个顶点，而且没有快速的方法
			// 来确定应该采用哪一个。
			// 目前这一情况会在后续的一遍 floodfill（泛洪填充）中规避。

			// const float sqd = get_distance_to_triangle_squared(t.v1, t.v2, t.v3, pos);
			const float sqd = get_distance_to_triangle_squared_precalc(t, pos);

			if (sqd < min_distance_squared) {
				min_distance_squared = sqd;
				closest_tri = &t;
			}
		}
	}

	const float d = Math::sqrt(min_distance_squared);
	VOXEL_ASSERT(closest_tri != nullptr);

	// if (p_dir == CLOCKWISE) {
	// const Vector3f plane_normal = (ct.v1 - ct.v3).cross(ct.v1 - ct.v2).normalized();
	const Vector3f plane_normal = get_normal(*closest_tri);
	//} else {
	//	normal = (p_point1 - p_point2).cross(p_point1 - p_point3);
	//}
	const float plane_d = math::dot(plane_normal, closest_tri->v1);

	if (math::dot(plane_normal, pos) > plane_d) {
		return d;
	}
	return -d;
}

struct GridToSpaceConverter {
	const Vector3i res;
	const Vector3f min_pos;
	const Vector3f mesh_size;
	const Vector3f half_cell_size;

	// 网格到空间的变换
	const Vector3f translation;
	const Vector3f scale;

	GridToSpaceConverter(Vector3i p_resolution, Vector3f p_min_pos, Vector3f p_mesh_size, Vector3f p_half_cell_size) :
			res(p_resolution),
			min_pos(p_min_pos),
			mesh_size(p_mesh_size),
			half_cell_size(p_half_cell_size),
			translation(min_pos + half_cell_size),
			scale(mesh_size / to_vec3f(res)) {}

	inline Vector3f operator()(const Vector3i grid_pos) const {
		return translation + scale * to_vec3f(grid_pos);
	}
};

struct Evaluator {
	Span<const Triangle> triangles;
	const GridToSpaceConverter grid_to_space;

	inline float operator()(const Vector3i grid_pos) const {
		return get_mesh_signed_distance_at(grid_to_space(grid_pos), triangles);
	}
};

struct EvaluatorCG {
	const ChunkGrid &chunk_grid;
	const GridToSpaceConverter grid_to_space;

	inline float operator()(const Vector3i grid_pos) const {
		return get_mesh_signed_distance_at(grid_to_space(grid_pos), chunk_grid);
	}
};

void generate_mesh_sdf_approx_interp(
		Span<float> sdf_grid,
		const Vector3i res,
		Span<const Triangle> triangles,
		const Vector3f min_pos,
		const Vector3f max_pos
) {
	VOXEL_PROFILE_SCOPE();

	static const float FAR_SD = 9999999.f;

	const Vector3f mesh_size = max_pos - min_pos;
	const Vector3f cell_size = mesh_size / Vector3f(res.x, res.y, res.z);
	const Evaluator eval{ triangles, GridToSpaceConverter(res, min_pos, mesh_size, cell_size * 0.5f) };

	const unsigned int node_size_po2 = 2;
	const unsigned int node_size_cells = 1 << node_size_po2;

	const Vector3i node_grid_size = math::ceildiv(res, node_size_cells) + Vector3i(1, 1, 1);
	const float node_size = node_size_cells * math::length(cell_size);
	const float node_subdiv_threshold = 0.6f * node_size;

	StdVector<float> node_grid;
	node_grid.resize(Vector3iUtil::get_volume_u64(node_grid_size));

	// 用远距离作为“无穷大”填充 SDF 网格，我们将用它来检查是否已计算
	sdf_grid.fill(FAR_SD);

	// 在节点的角点处计算 SDF
	Vector3i node_pos;
	for (node_pos.z = 0; node_pos.z < node_grid_size.z; ++node_pos.z) {
		for (node_pos.x = 0; node_pos.x < node_grid_size.x; ++node_pos.x) {
			for (node_pos.y = 0; node_pos.y < node_grid_size.y; ++node_pos.y) {
				const Vector3i gp000 = node_pos << node_size_po2;

				const size_t ni = Vector3iUtil::get_zxy_index(node_pos, node_grid_size);
				VOXEL_ASSERT(ni < node_grid.size());
				const float sd = eval(gp000);
				node_grid[ni] = sd;

				if (Box3i(Vector3i(), res).contains(gp000)) {
					const size_t i = Vector3iUtil::get_zxy_index(gp000, res);
					VOXEL_ASSERT(i < sdf_grid.size());
					sdf_grid[i] = sd;
				}
			}
		}
	}

	// 预计算扁平网格的邻居偏移
	const unsigned int ni100 = Vector3iUtil::get_zxy_index(Vector3i(1, 0, 0), node_grid_size);
	const unsigned int ni010 = Vector3iUtil::get_zxy_index(Vector3i(0, 1, 0), node_grid_size);
	const unsigned int ni110 = Vector3iUtil::get_zxy_index(Vector3i(1, 1, 0), node_grid_size);
	const unsigned int ni001 = Vector3iUtil::get_zxy_index(Vector3i(0, 0, 1), node_grid_size);
	const unsigned int ni101 = Vector3iUtil::get_zxy_index(Vector3i(1, 0, 1), node_grid_size);
	const unsigned int ni011 = Vector3iUtil::get_zxy_index(Vector3i(0, 1, 1), node_grid_size);
	const unsigned int ni111 = Vector3iUtil::get_zxy_index(Vector3i(1, 1, 1), node_grid_size);

	// 然后对每个节点
	for (node_pos.z = 0; node_pos.z < node_grid_size.z - 1; ++node_pos.z) {
		for (node_pos.x = 0; node_pos.x < node_grid_size.x - 1; ++node_pos.x) {
			for (node_pos.y = 0; node_pos.y < node_grid_size.y - 1; ++node_pos.y) {
				float ud = FAR_SD;

				const unsigned int ni = Vector3iUtil::get_zxy_index(node_pos, node_grid_size);

				// 获取之前在每个角点计算的有符号距离
				const float sd000 = node_grid[ni];
				const float sd100 = node_grid[ni + ni100];
				const float sd010 = node_grid[ni + ni010];
				const float sd110 = node_grid[ni + ni110];
				const float sd001 = node_grid[ni + ni001];
				const float sd101 = node_grid[ni + ni101];
				const float sd011 = node_grid[ni + ni011];
				const float sd111 = node_grid[ni + ni111];

				// 取最小值
				ud = math::min(ud, Math::abs(sd000));
				ud = math::min(ud, Math::abs(sd100));
				ud = math::min(ud, Math::abs(sd010));
				ud = math::min(ud, Math::abs(sd110));
				ud = math::min(ud, Math::abs(sd001));
				ud = math::min(ud, Math::abs(sd101));
				ud = math::min(ud, Math::abs(sd011));
				ud = math::min(ud, Math::abs(sd111));

				const Box3i cell_box = Box3i(node_pos * node_size_cells, Vector3iUtil::create(node_size_cells))
											   .clipped(Box3i(Vector3i(), res));

				// 如果节点角点处的最小距离低于阈值，
				// 则细分该节点。
				if (ud < node_subdiv_threshold) {
					// 全分辨率 SDF
					cell_box.for_each_cell_zxy([&sdf_grid, eval, res](const Vector3i grid_pos) {
						const size_t i = Vector3iUtil::get_zxy_index(grid_pos, res);
						if (sdf_grid[i] != FAR_SD) {
							// 已计算
							return;
						}
						VOXEL_ASSERT(i < sdf_grid.size());
						sdf_grid[i] = eval(grid_pos);
					});
				} else {
					// 距离表面足够远，通过插值角点来近似
					const Vector3i cell_box_end = cell_box.position + cell_box.size;
					Vector3i grid_pos;
					for (grid_pos.z = cell_box.position.z; grid_pos.z < cell_box_end.z; ++grid_pos.z) {
						for (grid_pos.x = cell_box.position.x; grid_pos.x < cell_box_end.x; ++grid_pos.x) {
							for (grid_pos.y = cell_box.position.y; grid_pos.y < cell_box_end.y; ++grid_pos.y) {
								const size_t i = Vector3iUtil::get_zxy_index(grid_pos, res);
								if (sdf_grid[i] != FAR_SD) {
									// 已计算
									continue;
								}
								const Vector3f ipf = to_vec3f(grid_pos - cell_box.position) / float(node_size_cells);
								const float sd = math::interpolate_trilinear(
										sd000, sd100, sd101, sd001, sd010, sd110, sd111, sd011, ipf
								);
								VOXEL_ASSERT(i < sdf_grid.size());
								sdf_grid[i] = sd;
							}
						}
					}
				}
			}
		}
	}
}

void generate_mesh_sdf_naive(
		Span<float> sdf_grid,
		const Vector3i res,
		const Box3i sub_box,
		Span<const Triangle> triangles,
		const Vector3f min_pos,
		const Vector3f max_pos
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(Box3i(Vector3i(), res).contains(sub_box));
	VOXEL_ASSERT(sdf_grid.size() == Vector3iUtil::get_volume_u64(res));

	const Vector3f mesh_size = max_pos - min_pos;
	const Vector3f cell_size = mesh_size / Vector3f(res.x, res.y, res.z);
	const Evaluator eval{ triangles, GridToSpaceConverter(res, min_pos, mesh_size, cell_size * 0.5f) };

	Vector3i grid_pos;

	const Vector3i sub_box_end = sub_box.position + sub_box.size;

	for (grid_pos.z = sub_box.position.z; grid_pos.z < sub_box_end.z; ++grid_pos.z) {
		for (grid_pos.x = sub_box.position.x; grid_pos.x < sub_box_end.x; ++grid_pos.x) {
			grid_pos.y = sub_box.position.y;
			size_t grid_index = Vector3iUtil::get_zxy_index(grid_pos, res);

			for (; grid_pos.y < sub_box_end.y; ++grid_pos.y) {
				const float sd = eval(grid_pos);

				VOXEL_ASSERT(grid_index < sdf_grid.size());
				sdf_grid[grid_index] = sd;

				++grid_index;
			}
		}
	}

	// const uint64_t usec = profiling_clock.restart();
	// println(format("Spent {} usec", usec));
}

void generate_mesh_sdf_partitioned(
		Span<float> sdf_grid,
		const Vector3i res,
		const Box3i sub_box,
		const Vector3f min_pos,
		const Vector3f max_pos,
		const ChunkGrid &chunk_grid
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(Box3i(Vector3i(), res).contains(sub_box));
	VOXEL_ASSERT(sdf_grid.size() == Vector3iUtil::get_volume_u64(res));

	const Vector3f mesh_size = max_pos - min_pos;
	const Vector3f cell_size = mesh_size / Vector3f(res.x, res.y, res.z);
	const EvaluatorCG eval{ chunk_grid, GridToSpaceConverter(res, min_pos, mesh_size, cell_size * 0.5f) };

	const Vector3i sub_box_end = sub_box.position + sub_box.size;

	Vector3i grid_pos;
	for (grid_pos.z = sub_box.position.z; grid_pos.z < sub_box_end.z; ++grid_pos.z) {
		for (grid_pos.x = sub_box.position.x; grid_pos.x < sub_box_end.x; ++grid_pos.x) {
			grid_pos.y = sub_box.position.y;
			size_t grid_index = Vector3iUtil::get_zxy_index(grid_pos, res);

			for (; grid_pos.y < sub_box_end.y; ++grid_pos.y) {
				const float sd = eval(grid_pos);

				VOXEL_ASSERT(grid_index < sdf_grid.size());
				sdf_grid[grid_index] = sd;

				++grid_index;
			}
		}
	}
}

void generate_mesh_sdf_partitioned(
		Span<float> sdf_grid,
		const Vector3i res,
		Span<const Triangle> triangles,
		const Vector3f min_pos,
		const Vector3f max_pos,
		int subdiv
) {
	// TODO 将此设为线程局部？
	ChunkGrid chunk_grid;
	partition_triangles(subdiv, triangles, min_pos, max_pos, chunk_grid);
	compute_near_chunks(chunk_grid);
	generate_mesh_sdf_partitioned(sdf_grid, res, Box3i(Vector3i(), res), min_pos, max_pos, chunk_grid);
}

CheckResult check_sdf(
		Span<const float> sdf_grid,
		Vector3i res,
		Span<const Triangle> triangles,
		Vector3f min_pos,
		Vector3f max_pos
) {
	CheckResult result;
	result.ok = false;

	VOXEL_ASSERT_RETURN_V(math::is_valid_size(res), result);

	if (res.x == 0 || res.y == 0 || res.z == 0) {
		// 为空或无法比较，但可以接受
		result.ok = true;
		return result;
	}

	const float tolerance = 1.1f;
	const float max_variation = tolerance * get_max_sdf_variation(min_pos, max_pos, res);

	FixedArray<Vector3i, 6> dirs;
	dirs[0] = Vector3i(-1, 0, 0);
	dirs[1] = Vector3i(1, 0, 0);
	dirs[2] = Vector3i(0, -1, 0);
	dirs[3] = Vector3i(0, 1, 0);
	dirs[4] = Vector3i(0, 0, -1);
	dirs[5] = Vector3i(0, 0, 1);

	const Vector3f mesh_size = max_pos - min_pos;
	const Vector3f cell_size = mesh_size / Vector3f(res.x, res.y, res.z);
	const Evaluator eval{ triangles, GridToSpaceConverter(res, min_pos, mesh_size, cell_size * 0.5f) };

	Vector3i pos;
	for (pos.z = 0; pos.z < res.z; ++pos.z) {
		for (pos.x = 0; pos.x < res.x; ++pos.x) {
			for (pos.y = 0; pos.y < res.y; ++pos.y) {
				//
				for (unsigned int dir = 0; dir < dirs.size(); ++dir) {
					const Vector3i npos = pos + dirs[dir];

					if (npos.x < 0 || npos.y < 0 || npos.z < 0 || npos.x >= res.x || npos.y >= res.y ||
						npos.z >= res.z) {
						continue;
					}

					const unsigned int loc = Vector3iUtil::get_zxy_index(pos, res);
					const unsigned int nloc = Vector3iUtil::get_zxy_index(npos, res);

					const float v = sdf_grid[loc];
					const float nv = sdf_grid[nloc];

					const float variation = Math::abs(v - nv);

					if (variation > max_variation) {
						VOXEL_PRINT_VERBOSE(
								format("Found variation of {} > {}, {} and {}, at cell {} and {}",
									   variation,
									   max_variation,
									   v,
									   nv,
									   pos,
									   npos)
						);

						const Vector3f posf0 = eval.grid_to_space(pos);
						const Vector3f posf1 = eval.grid_to_space(npos);

						result.ok = false;

						result.cell0.grid_pos = pos;
						result.cell0.mesh_pos = posf0;
						result.cell0.closest_triangle_index = get_closest_triangle_precalc(posf0, triangles);

						result.cell1.grid_pos = npos;
						result.cell1.mesh_pos = posf1;
						result.cell1.closest_triangle_index = get_closest_triangle_precalc(posf1, triangles);

						return result;
					}
				}
			}
		}
	}

	result.ok = true;
	return result;
}

void generate_mesh_sdf_naive(
		Span<float> sdf_grid,
		const Vector3i res,
		Span<const Triangle> triangles,
		const Vector3f min_pos,
		const Vector3f max_pos
) {
	generate_mesh_sdf_naive(sdf_grid, res, Box3i(Vector3i(), res), triangles, min_pos, max_pos);
}

bool prepare_triangles(
		Span<const Vector3> vertices,
		Span<const int> indices,
		StdVector<Triangle> &triangles,
		Vector3f &out_min_pos,
		Vector3f &out_max_pos
) {
	VOXEL_PROFILE_SCOPE();

	// 如果顶点少于 4 个，网格无法闭合
	VOXEL_ASSERT_RETURN_V(vertices.size() >= 4, false);

	if (indices.size() != 0) {
		// 如果三角形少于 4 个，网格无法闭合
		VOXEL_ASSERT_RETURN_V(indices.size() >= 12, false);
		VOXEL_ASSERT_RETURN_V(indices.size() % 3 == 0, false);

		triangles.resize(indices.size() / 3);

		for (size_t ti = 0; ti < triangles.size(); ++ti) {
			const int ii = ti * 3;
			const int i0 = indices[ii];
			const int i1 = indices[ii + 1];
			const int i2 = indices[ii + 2];

			Triangle &t = triangles[ti];
			t.v1 = to_vec3f(vertices[i0]);
			t.v2 = to_vec3f(vertices[i1]);
			t.v3 = to_vec3f(vertices[i2]);

			// 确保所有点互不相同的技巧
			// const Vector3f midp = (t.v1 + t.v2 + t.v3) / 3.f;
			// const float shrink_amount = 0.0001f;
			// t.v1 = math::lerp(t.v1, midp, shrink_amount);
			// t.v2 = math::lerp(t.v2, midp, shrink_amount);
			// t.v3 = math::lerp(t.v3, midp, shrink_amount);
		}

	} else {
		// 非索引网格

		// 如果三角形少于 4 个，网格无法闭合
		VOXEL_ASSERT_RETURN_V(vertices.size() >= 12, false);
		VOXEL_ASSERT_RETURN_V(vertices.size() % 3 == 0, false);

		triangles.resize(vertices.size() / 3);

		for (size_t ti = 0; ti < triangles.size(); ++ti) {
			const int i0 = ti * 3;
			Triangle &t = triangles[ti];
			t.v1 = to_vec3f(vertices[i0 + 0]);
			t.v2 = to_vec3f(vertices[i0 + 1]);
			t.v3 = to_vec3f(vertices[i0 + 2]);
		}
	}

	Vector3f min_pos = to_vec3f(vertices[0]);
	Vector3f max_pos = min_pos;

	for (size_t i = 0; i < vertices.size(); ++i) {
		const Vector3f p = to_vec3f(vertices[i]);
		min_pos = math::min(p, min_pos);
		max_pos = math::max(p, max_pos);
	}

	out_min_pos = min_pos;
	out_max_pos = max_pos;

	precalc_triangles(voxel::to_span(triangles));

	return true;
}

Vector3i auto_compute_grid_resolution(const Vector3f box_size, int cell_count) {
	const float cs = math::max(box_size.x, math::max(box_size.y, box_size.z)) / float(cell_count);
	return Vector3i(box_size.x / cs, box_size.y / cs, box_size.z / cs);
}

// 在线程池内调用
void GenMeshSDFSubBoxTask::run(ThreadedTaskContext &ctx) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(shared_data != nullptr);

	VoxelBuffer &buffer = shared_data->buffer;
	const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
	// VOXEL_ASSERT(!buffer.get_channel_compression(channel) == VoxelBuffer::COMPRESSION_NONE);
	Span<float> sdf_grid;
	VOXEL_ASSERT(buffer.get_channel_data(channel, sdf_grid));

	if (shared_data->use_chunk_grid) {
		generate_mesh_sdf_partitioned(
				sdf_grid, buffer.get_size(), box, shared_data->min_pos, shared_data->max_pos, shared_data->chunk_grid
		);
	} else {
		generate_mesh_sdf_naive(
				sdf_grid,
				buffer.get_size(),
				box,
				to_span(shared_data->triangles),
				shared_data->min_pos,
				shared_data->max_pos
		);
	}

	if (shared_data->pending_jobs.fetch_sub(1, std::memory_order_acq_rel) == 1) {
		if (shared_data->boundary_sign_fix) {
			fix_sdf_sign_from_boundary(sdf_grid, buffer.get_size(), shared_data->min_pos, shared_data->max_pos);
		}
		// 这是最后一个任务
		on_complete();
	}
}

static const float FAR_SD = 9999999.f;

int find_sdf_sign_with_raycast_multi_attempt(
		const ChunkGrid &chunk_grid,
		Span<const Triangle> triangles,
		Vector3f pos,
		unsigned int &ref_triangle_index
) {
	// TODO 优化：以靠近格子的三角形为目标可能会更快？
	int sign_sum = 0;
	// 执行多次射线投射以减少歧义。
	// 因为不幸的是，在处理大量格子上的大量三角形时，float 精度总会以某种方式出问题，
	// 导致某些三角形被遗漏。
	for (int attempt = 0; attempt < 3; ++attempt) {
		int sign_value;
		if (find_sdf_sign_with_raycast(chunk_grid, pos, triangles[ref_triangle_index], sign_value)) {
			sign_sum += sign_value;
		}
		// else {
		// 	// 取消此次尝试
		// 	--attempt;
		// }
		ref_triangle_index = (ref_triangle_index + 1) % triangles.size();
		if (attempt == 1 && sign_sum != 0) {
			// 再尝试一次也不会有区别
			break;
		}
	}

	return sign_sum;
}

void generate_mesh_sdf_hull(
		Span<float> sdf_grid,
		const Vector3i res,
		Span<const Triangle> triangles,
		const Vector3f min_pos,
		const Vector3f max_pos,
		const ChunkGrid &chunk_grid,
		Span<uint8_t> flag_grid,
		uint8_t near_surface_flag_value
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(sdf_grid.size() == flag_grid.size());

	// 用远距离作为“无穷大”填充 SDF 网格，我们将用它来检查是否已计算
	sdf_grid.fill(FAR_SD);

	const Vector3f mesh_size = max_pos - min_pos;
	const Vector3f cell_size = mesh_size / Vector3f(res.x, res.y, res.z);
	const GridToSpaceConverter grid_to_space(res, min_pos, mesh_size, cell_size * 0.5f);

	const Vector3f inv_gts_scale = Vector3f(1.f) / grid_to_space.scale;

	const Box3i grid_box(Vector3i(), res);

	constexpr int pad = 2;

	{
		VOXEL_PROFILE_SCOPE_NAMED("Tri squared distances");

		for (unsigned int tri_index = 0; tri_index < triangles.size(); ++tri_index) {
			const Triangle &t = triangles[tri_index];

			const Vector3f aabb_min = math::min(t.v1, math::min(t.v2, t.v3));
			const Vector3f aabb_max = math::max(t.v1, math::max(t.v2, t.v3));

			// 空间到网格的变换
			const Vector3f aabb_min_g = inv_gts_scale * (aabb_min - grid_to_space.translation);
			const Vector3f aabb_max_g = inv_gts_scale * (aabb_max - grid_to_space.translation);

			// 距离场在这些相交三角形周围约 `pad` 个格子内是精确的。
			// 超出该范围则是近似值，而在包围盒之外则未设置。
			const Box3i tbox = Box3i::from_min_max(to_vec3i(math::floor(aabb_min_g)), to_vec3i(math::ceil(aabb_max_g)))
									   .padded(pad)
									   .clipped(grid_box);

			tbox.for_each_cell_zxy([&sdf_grid, grid_to_space, res, &t](const Vector3i &grid_pos) {
				const size_t i = Vector3iUtil::get_zxy_index(grid_pos, res);
				float &dst = sdf_grid[i];
				const Vector3f pos = grid_to_space(grid_pos);
				const float ud = get_distance_to_triangle_squared_precalc(t, pos);
				dst = math::min(dst, ud);
			});
		}
	}

	const float mv = pad * get_max_sdf_variation(min_pos, max_pos, res);
	// const Triangle &ref_triangle = triangles[0];

	{
		VOXEL_PROFILE_SCOPE_NAMED("Sqrt + Raycast signs");

		unsigned int ref_triangle_index = 0;

		Vector3i grid_pos;
		for (grid_pos.z = 0; grid_pos.z < res.z; ++grid_pos.z) {
			for (grid_pos.x = 0; grid_pos.x < res.x; ++grid_pos.x) {
				grid_pos.y = 0;
				unsigned int loc = Vector3iUtil::get_zxy_index(grid_pos, res);

				for (; grid_pos.y < res.y; ++grid_pos.y, ++loc) {
					float &df = sdf_grid[loc];

					if (df != FAR_SD) {
						// 应用平方根，因为到目前为止我们用的是平方距离
						df = Math::sqrt(df);

						if (df < mv) {
							// 在靠近表面的格子中计算精确的符号
							flag_grid[loc] = near_surface_flag_value;

#ifdef VOXEL_MESH_SDF_DEBUG_BATCH
							if (grid_pos == Vector3i(39, 60, 53)) {
								DDD::begin_batch();

								for (unsigned int ti = 0; ti < triangles.size(); ++ti) {
									const Triangle &t = triangles[ti];
									DDD::draw_triangle(t.v1, t.v2, t.v3, Color(1, 1, 1, 0.5));
								}

								DDD::batch_mark_segment();
							}
#endif

							const int sign_sum = find_sdf_sign_with_raycast_multi_attempt(
									chunk_grid, triangles, grid_to_space(grid_pos), ref_triangle_index
							);

#ifdef VOXEL_MESH_SDF_DEBUG_BATCH
							DDD::save_batch_to_json("ddd_ofs.json");
							DDD::end_batch();
#endif

							if (sign_sum < 0) {
								df = -df;
							}
						}
					}
				}
			}
		}
	}
}

struct Seed {
	Vector3i pos;
};

#ifdef VOXEL_MESH_SDF_DEBUG_SLICES

void debug_print_sdf_image_slice(
		Span<const float> sdf_grid,
		Vector3i res,
		const int y,
		int iteration,
		StdVector<Seed> *seeds
) {
	Ref<Image> im;
	im.instantiate();
	im->create(res.x, res.z, false, Image::FORMAT_RGB8);

	const Color nega_col(0.5f, 0.5f, 1.0f);
	const Color posi_col(1.0f, 0.6f, 0.1f);
	const Color black(0.f, 0.f, 0.f);

	for (int z = 0; z < res.z; ++z) {
		for (int x = 0; x < res.x; ++x) {
			const unsigned int loc = Vector3iUtil::get_zxy_index(Vector3i(x, y, z), res);
			const float sd = sdf_grid[loc];

			const float nega = math::clamp(-sd, 0.0f, 1.0f);
			const float posi = math::clamp(sd, 0.0f, 1.0f);
			const Color col = math::lerp(black, nega_col, nega) + math::lerp(black, posi_col, posi);

			im->set_pixel(x, z, col);
		}
	}

	const int scale = 8;

	Ref<Image> im2 = im->duplicate();
	im2->resize(im->get_width() * scale, im->get_height() * scale, Image::INTERPOLATE_NEAREST);
	im2->save_png(String("debug_voxel_mesh_sdf_slice{0}_iteration{1}.png").format(varray(y, iteration)));

	if (seeds != nullptr) {
		for (auto it = seeds->begin(); it != seeds->end(); ++it) {
			const Seed seed = *it;
			const Vector3i pos = seed.pos;
			if (pos.y == y) {
				im->set_pixel(pos.x, pos.z, Color(0, 1, 0));
			}
		}

		im2 = im->duplicate();
		im2->resize(im->get_width() * scale, im->get_height() * scale, Image::INTERPOLATE_NEAREST);
		im2->save_png(String("debug_voxel_mesh_sdf_slice{0}_iteration{1}_s.png").format(varray(y, iteration)));
	}
}

#endif

void generate_mesh_sdf_approx_floodfill(
		Span<float> sdf_grid,
		const Vector3i res,
		Span<const Triangle> triangles,
		const ChunkGrid &chunk_grid,
		const Vector3f min_pos,
		const Vector3f max_pos,
		bool boundary_sign_fix
) {
	VOXEL_PROFILE_SCOPE();

	StdVector<uint8_t> flag_grid;
	flag_grid.resize(Vector3iUtil::get_volume_u64(res));
	memset(flag_grid.data(), FLAG_NOT_VISITED, sizeof(uint8_t) * flag_grid.size());

	generate_mesh_sdf_hull(sdf_grid, res, triangles, min_pos, max_pos, chunk_grid, to_span(flag_grid), FLAG_FROZEN);

#ifdef VOXEL_MESH_SDF_DEBUG_SLICES
	for (int y = 0; y < res.y; ++y) {
		debug_print_sdf_image_slice(sdf_grid, res, y, 0, nullptr);
	}
#endif

	StdVector<Seed> seeds0;

	{
		VOXEL_PROFILE_SCOPE_NAMED("Place seeds");

		FixedArray<Vector3i, 6> dirs6;
		dirs6[0] = Vector3i(-1, 0, 0);
		dirs6[1] = Vector3i(1, 0, 0);
		dirs6[2] = Vector3i(0, -1, 0);
		dirs6[3] = Vector3i(0, 1, 0);
		dirs6[4] = Vector3i(0, 0, -1);
		dirs6[5] = Vector3i(0, 0, 1);

		Vector3i pos;
		for (pos.z = 0; pos.z < res.z; ++pos.z) {
			for (pos.x = 0; pos.x < res.x; ++pos.x) {
				pos.y = 0;
				unsigned int loc = Vector3iUtil::get_zxy_index(pos, res);

				for (; pos.y < res.y; ++pos.y, ++loc) {
					if (flag_grid[loc] != FLAG_FROZEN) {
						continue;
					}

					for (unsigned int dir = 0; dir < dirs6.size(); ++dir) {
						const Vector3i npos = pos + dirs6[dir];
						if (npos.x < 0 || npos.y < 0 || npos.z < 0 || npos.x >= res.x || npos.y >= res.y ||
							npos.z >= res.z) {
							continue;
						}

						const unsigned int nloc = Vector3iUtil::get_zxy_index(npos, res);
						if (flag_grid[nloc] != FLAG_FROZEN) {
							const float sd = sdf_grid[loc];
							VOXEL_ASSERT(sd != FAR_SD);
							seeds0.push_back({ pos });
							break;
						}
					}
				}
			}
		}
	}

	const Vector3f mesh_size = max_pos - min_pos;
	const Vector3f cell_size = mesh_size / Vector3f(res.x, res.y, res.z);

	FixedArray<float, 8> dds;
	dds[0b000] = 0;
	dds[0b100] = cell_size.x;
	dds[0b010] = cell_size.y;
	dds[0b110] = Math::sqrt(math::squared(cell_size.x) + math::squared(cell_size.y));
	dds[0b001] = cell_size.z;
	dds[0b101] = Math::sqrt(math::squared(cell_size.x) + math::squared(cell_size.z));
	dds[0b011] = Math::sqrt(math::squared(cell_size.y) + math::squared(cell_size.z));
	dds[0b111] = math::length(cell_size);

	StdVector<Seed> seeds1;

	StdVector<Seed> *current_seeds = &seeds0;
	StdVector<Seed> *next_seeds = &seeds1;

	const Vector3i res_minus_one = res - Vector3i(1, 1, 1);

#ifdef VOXEL_MESH_SDF_DEBUG_SLICES
	// unsigned int iteration = 0;
#endif

	while (current_seeds->size() > 0) {
		VOXEL_PROFILE_SCOPE_NAMED("Iteration");

#ifdef VOXEL_MESH_SDF_DEBUG_SLICES
		// 调试
		// debug_print_sdf_image_slice(sdf_grid, res, 23, iteration, current_seeds);
		// ++iteration;
#endif

		// 广度优先，不要遍历本次迭代中创建的种子
		for (auto it = current_seeds->begin(); it != current_seeds->end(); ++it) {
			const Seed seed = *it;
			const Vector3i pos = seed.pos;

			const unsigned int loc = Vector3iUtil::get_zxy_index(pos, res);

			const float src_sd = sdf_grid[loc];
			VOXEL_ASSERT(src_sd != FAR_SD);

			// 确保不越过网格边界
			const int min_dz = pos.z == 0 ? 0 : -1;
			const int min_dy = pos.y == 0 ? 0 : -1;
			const int min_dx = pos.x == 0 ? 0 : -1;
			const int max_dz = pos.z == res_minus_one.z ? 1 : 2;
			const int max_dy = pos.y == res_minus_one.y ? 1 : 2;
			const int max_dx = pos.x == res_minus_one.x ? 1 : 2;

			// 26 个方向
			for (int dz = min_dz; dz < max_dz; ++dz) {
				for (int dy = min_dy; dy < max_dy; ++dy) {
					for (int dx = min_dx; dx < max_dx; ++dx) {
						// 排除中间点
						if (dx == 0 && dy == 0 && dz == 0) {
							continue;
						}

						const Vector3i npos(pos.x + dx, pos.y + dy, pos.z + dz);
						const unsigned int nloc = Vector3iUtil::get_zxy_index(npos, res);

						const uint8_t nflag = flag_grid[nloc];
						if (nflag == FLAG_FROZEN) {
							continue;
						}

						const unsigned int ddi = (Math::abs(dx) << 2) | (Math::abs(dy) << 1) | Math::abs(dz);
						const float dd = dds[ddi];

						float &dst_sd = sdf_grid[nloc];

						if (nflag == FLAG_NOT_VISITED) {
							// 首次访问

							if (dst_sd == FAR_SD) {
								float sd;
								if (src_sd < 0.f) {
									sd = src_sd - dd;
								} else {
									sd = src_sd + dd;
								}

								dst_sd = sd;

							} else {
								// 如果该格子已由外壳预处理阶段设置，我们可以做 min/max，因为外壳
								// 值可能是更好的近似。否则可能导致靠近表面处的近似更差，
								// 进而传播出更差的结果。
								float sd;
								if (src_sd < 0.f) {
									// 未冻结的外壳距离是无符号的
									sd = math::max(-dst_sd, src_sd - dd);
								} else {
									sd = math::min(dst_sd, src_sd + dd);
								}

								dst_sd = sd;
							}

							next_seeds->push_back({ npos });
							flag_grid[nloc] = FLAG_VISITED;

						} else { // FLAG_VISITED
							// 已被洪泛填充访问过，若有更好的有符号距离则采用
							float sd;
							if (src_sd < 0.f) {
								sd = math::max(dst_sd, src_sd - dd);
							} else {
								sd = math::min(dst_sd, src_sd + dd);
							}

							dst_sd = sd;
						}
					}
				}
			}
		}

		current_seeds->clear();

		StdVector<Seed> *temp = current_seeds;
		current_seeds = next_seeds;
		next_seeds = temp;
	}
}

} // namespace voxel::mesh_sdf
