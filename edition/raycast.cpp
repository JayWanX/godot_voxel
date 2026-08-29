#include "raycast.h"
#include "../meshers/blocky/voxel_mesher_blocky.h"
#include "../meshers/cubes/voxel_mesher_cubes.h"
#include "../storage/voxel_buffer.h"
#include "../storage/voxel_data.h"
#include "../terrain/voxel_node.h"
#include "../util/godot/classes/ref_counted.h"
#include "../util/profiling.h"
#include "../util/voxel_raycast.h"
#include "funcs.h"
#include "voxel_raycast_result.h"

namespace voxel {

// 二分搜索可能比线性回归更精确，因为 SDF 本身可能就不准确。
// 另一种方案是将中阶段命中位置周围的一小块区域多边形化。
// `d1` 是二分搜索将沿 `dir` 距 `pos0` 多远进行。
// 若该线段不包含零交叉，则可能会在内部调整。
template <typename Volume_F>
float approximate_distance_to_isosurface_binary_search(
		const Volume_F &f,
		const Vector3 pos0,
		const Vector3 dir,
		float d1,
		const int iterations
) {
	float d0 = 0.f;
	float sdf0 = get_sdf_interpolated(f, pos0);
	// 作为参数给出的位置可能来自中阶段，是一个粗略的近似值，
	// 因此可能略低于表面。我们可以稍作调整使其位于表面之上。
	for (int i = 0; i < 4 && sdf0 < 0.f; ++i) {
		d0 -= 0.5f;
		sdf0 = get_sdf_interpolated(f, pos0 + dir * d0);
	}

	float sdf1 = get_sdf_interpolated(f, pos0 + dir * d1);
	for (int i = 0; i < 4 && sdf1 > 0.f; ++i) {
		d1 += 0.5f;
		sdf1 = get_sdf_interpolated(f, pos0 + dir * d1);
	}

	if ((sdf0 > 0) != (sdf1 > 0)) {
		// 二分搜索
		for (int i = 0; i < iterations; ++i) {
			const float dm = 0.5f * (d0 + d1);
			const float sdf_mid = get_sdf_interpolated(f, pos0 + dir * dm);

			if ((sdf_mid > 0) != (sdf0 > 0)) {
				sdf1 = sdf_mid;
				d1 = dm;
			} else {
				sdf0 = sdf_mid;
				d0 = dm;
			}
		}
	}

	// 选择最接近表面的距离
	if (Math::abs(sdf0) < Math::abs(sdf1)) {
		return d0;
	} else {
		return d1;
	}
}

template <typename TSd>
inline Vector3f get_raw_sdf_gradient(const Span<const TSd> sd_data, const unsigned int i0, const Vector3i jump) {
	const float sd_nx = sd_data[i0 - jump.x];
	const float sd_px = sd_data[i0 + jump.x];
	const float sd_ny = sd_data[i0 - jump.y];
	const float sd_py = sd_data[i0 + jump.y];
	const float sd_nz = sd_data[i0 - jump.z];
	const float sd_pz = sd_data[i0 + jump.z];

	const Vector3f gradient(sd_px - sd_nx, sd_py - sd_ny, sd_pz - sd_nz);
	return gradient;
}

template <typename TSd>
Vector3f get_interpolated_raw_sdf_gradient_4x4x4_p111_t(const VoxelBuffer &vb, const Vector3f pf) {
	const Vector3i block_size(4, 4, 4);
	VOXEL_ASSERT(vb.get_size() == block_size);

	Span<const TSd> sd_data;
	const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
	VOXEL_ASSERT(vb.get_channel_data_read_only(channel, sd_data));

	const Vector3i jump(block_size.y, 1, block_size.y * block_size.x);
	const Vector3i p000(1, 1, 1);
	const unsigned int i0 = p000.x * jump.x + p000.y * jump.y + p000.z * jump.z;

	const Vector3f g000 = get_raw_sdf_gradient<TSd>(sd_data, i0, jump);
	const Vector3f g100 = get_raw_sdf_gradient<TSd>(sd_data, i0 + jump.x, jump);
	const Vector3f g010 = get_raw_sdf_gradient<TSd>(sd_data, i0 + jump.y, jump);
	const Vector3f g110 = get_raw_sdf_gradient<TSd>(sd_data, i0 + jump.x + jump.y, jump);
	const Vector3f g001 = get_raw_sdf_gradient<TSd>(sd_data, i0 + jump.z, jump);
	const Vector3f g101 = get_raw_sdf_gradient<TSd>(sd_data, i0 + jump.x + jump.z, jump);
	const Vector3f g011 = get_raw_sdf_gradient<TSd>(sd_data, i0 + jump.y + jump.z, jump);
	const Vector3f g111 = get_raw_sdf_gradient<TSd>(sd_data, i0 + jump.x + jump.y + jump.z, jump);

	return math::interpolate_trilinear<Vector3f>(g000, g100, g101, g001, g010, g110, g111, g011, pf);
}

// 在 4x4x4 体素的缓冲区中，从由体素 (1,1,1) 到 (2,2,2) 定义的立方体内部获取梯度，
// 不考虑内部缩放。这可用于计算法线。
Vector3f get_interpolated_raw_sdf_gradient_4x4x4_p111(const VoxelBuffer &vb, const Vector3f pf) {
	const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;

	switch (vb.get_channel_compression(channel)) {
		case VoxelBuffer::COMPRESSION_UNIFORM:
			// 无梯度！
			return Vector3f();

		case VoxelBuffer::COMPRESSION_NONE: {
			switch (vb.get_channel_depth(channel)) {
				case VoxelBuffer::DEPTH_8_BIT:
					return get_interpolated_raw_sdf_gradient_4x4x4_p111_t<int8_t>(vb, pf);

				case VoxelBuffer::DEPTH_16_BIT:
					return get_interpolated_raw_sdf_gradient_4x4x4_p111_t<int16_t>(vb, pf);

				case VoxelBuffer::DEPTH_32_BIT:
					return get_interpolated_raw_sdf_gradient_4x4x4_p111_t<float>(vb, pf);

				default:
					VOXEL_PRINT_ERROR("Unhandled depth");
					return Vector3f();
			}
		} break;

		default:
			VOXEL_PRINT_ERROR("Unhandled compression");
			return Vector3f();
	}
}

Vector3f get_interpolated_raw_sdf_gradient(const VoxelData &vd, const Vector3 position) {
	VOXEL_PROFILE_SCOPE();

	const Vector3i p00 = to_vec3i(math::floor(position));
	const Vector3f pf = to_vec3f(position - to_vec3(p00));

	const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
	VoxelBuffer vb(VoxelBuffer::ALLOCATOR_POOL);
	vb.create(Vector3i(4, 4, 4));
	vd.copy(p00 - Vector3i(1, 1, 1), vb, (1 << channel), false);
	return get_interpolated_raw_sdf_gradient_4x4x4_p111(vb, pf);
}

Ref<VoxelRaycastResult> raycast_sdf(
		const VoxelData &voxel_data,
		const Vector3 ray_origin,
		const Vector3 ray_dir,
		const float max_distance,
		const uint8_t binary_search_iterations,
		const bool normal_enabled
) {
	// TODO 实现反向射线投射？（从地面内部射向空气，可能对“挖掘”有用）

	// TODO 优化：体素射线投射使用的是 `get_voxel`，这是最慢的方式，但可以做得更快。
	// 相反，可以对块做粗阶段。如果某个块的体素需要被解析，就获取射线在该块中可能经过的
	// 所有位置，然后一次性全部查询（更适合批量处理，无需再次经过
	// 锁与数据结构，并支持 SIMD）。然后按顺序检查结果。
	// 若未找到命中，则继续处理下一个块。

	struct RaycastPredicate {
		const VoxelData &data;

		bool operator()(const VoxelRaycastState &rs) {
			// 这并不是特别优化，但对于玩家射线投射来说运行速度足够快
			VoxelSingleValue defval;
			defval.f = constants::SDF_FAR_OUTSIDE;
			const VoxelSingleValue v = data.get_voxel(rs.hit_position, VoxelBuffer::CHANNEL_SDF, defval);
			return v.f < 0;
		}
	};

	Ref<VoxelRaycastResult> res;

	// 我们使用网格射线投射作为中阶段，大致检测命中位置在哪里
	RaycastPredicate predicate = { voxel_data };
	Vector3i hit_pos;
	Vector3i prev_pos;
	float hit_distance;
	float hit_distance_prev;
	// 使用 marching cubes 多边形化的体素会影响以其下角为中心的区域，
	// 并在所有方向上延伸最多 0.5 个单位。
	//
	//   o--------o--------o
	//   | A      |     B  |  这里体素 B 是实心的，体素 A、C 和 D 是空的。
	//   |       xxx       |  由于插值，物质会出现在 B 的下角。
	//   |     xxxxxxx     |
	//   o---xxxxxoxxxxx---o
	//   |     xxxxxxx     |
	//   |       xxx       |
	//   | C      |     D  |
	//   o--------o--------o
	//
	// `voxel_raycast` 在离散的立方体体素网格上运行，因此为了考虑平滑插值，
	// 我们可能会偏移射线，使立方体表现得好像以过滤后的结果为中心。
	const Vector3 offset(0.5, 0.5, 0.5);
	if (voxel_raycast(
				ray_origin + offset,
				ray_dir,
				predicate,
				max_distance,
				hit_pos,
				prev_pos,
				hit_distance,
				hit_distance_prev
		)) {
		// 近似表面

		float d = hit_distance;

		if (binary_search_iterations > 0) {
			// 这并不是特别优化，但对于玩家射线投射来说运行速度足够快
			struct VolumeSampler {
				const VoxelData &data;

				inline float operator()(const Vector3i &pos) const {
					VoxelSingleValue defval;
					defval.f = constants::SDF_FAR_OUTSIDE;
					const VoxelSingleValue value = data.get_voxel(pos, VoxelBuffer::CHANNEL_SDF, defval);
					return value.f;
				}
			};

			VolumeSampler sampler{ voxel_data };
			d = hit_distance_prev +
					approximate_distance_to_isosurface_binary_search(
							sampler,
							ray_origin + ray_dir * hit_distance_prev,
							ray_dir,
							hit_distance - hit_distance_prev,
							binary_search_iterations
					);
		}

		res.instantiate();
		res->position = hit_pos;
		res->previous_position = prev_pos;
		res->distance_along_ray = d;
		const Vector3 hit_pos_f = ray_origin + ray_dir * d;

		if (normal_enabled) {
			const Vector3f gradient = get_interpolated_raw_sdf_gradient(voxel_data, hit_pos_f);
			res->normal = to_vec3(math::normalized(gradient));
		}
	}

	return res;
}

Ref<VoxelRaycastResult> raycast_blocky(
		const VoxelData &voxel_data,
		const VoxelMesherBlocky &mesher,
		const Vector3 ray_origin,
		const Vector3 ray_dir,
		const float max_distance,
		const uint32_t p_collision_mask
) {
	struct RaycastPredicateBlocky {
		const VoxelData &data;
		const blocky::BakedLibrary &baked_data;
		const uint32_t collision_mask;
		const Vector3 p_from;
		const Vector3 p_to;
		Vector3 &hit_position;
		Vector3 &hit_normal;

		bool operator()(const VoxelRaycastState &rs) {
			VoxelSingleValue defval;
			defval.i = 0;
			const int v = data.get_voxel(rs.hit_position, VoxelBuffer::CHANNEL_TYPE, defval).i;

			if (baked_data.has_model(v) == false) {
				return false;
			}

			const blocky::BakedModel &model = baked_data.models[v];
			if ((model.box_collision_mask & collision_mask) == 0) {
				return false;
			}

			for (const AABB &aabb : model.box_collision_aabbs) {
				if (AABB(aabb.position + rs.hit_position, aabb.size)
							.intersects_segment(p_from, p_to, &hit_position, &hit_normal)) {
					return true;
				}
			}

			return false;
		}
	};

	Ref<VoxelRaycastResult> res;

	Ref<VoxelBlockyLibraryBase> library_ref = mesher.get_library();
	if (library_ref.is_null()) {
		return res;
	}

	Vector3 hit_pos;
	Vector3 hit_normal;

	RaycastPredicateBlocky predicate{
		voxel_data, //
		library_ref->get_baked_data(), //
		p_collision_mask, //
		ray_origin, //
		ray_origin + ray_dir * max_distance, //
		hit_pos, //
		hit_normal //
	};

	float hit_distance;
	float hit_distance_prev;
	Vector3i hit_voxel_pos;
	Vector3i prev_voxel_pos;

	if (voxel_raycast(
				ray_origin,
				ray_dir,
				predicate,
				max_distance,
				hit_voxel_pos,
				prev_voxel_pos,
				hit_distance,
				hit_distance_prev
		)) {
		// const Vector3 cube_hit_pos = ray_origin + ray_dir * hit_distance;

		res.instantiate();
		res->position = hit_voxel_pos;
		res->previous_position = prev_voxel_pos;
		// TODO 也许某天需要打破兼容性，让 `position` 成为实际的命中位置，
		// 而不是必须使用该距离
		res->distance_along_ray = hit_pos.distance_to(ray_origin);
		res->normal = hit_normal;
	}

	return res;
}

Ref<VoxelRaycastResult> raycast_nonzero(
		const VoxelData &voxel_data,
		const Vector3 ray_origin,
		const Vector3 ray_dir,
		const float max_distance,
		const uint8_t p_channel
) {
	struct RaycastPredicateColor {
		const VoxelData &data;
		const uint8_t channel;

		bool operator()(const VoxelRaycastState &rs) const {
			VoxelSingleValue defval;
			defval.i = 0;
			const uint64_t v = data.get_voxel(rs.hit_position, channel, defval).i;
			return v != 0;
		}
	};

	Ref<VoxelRaycastResult> res;

	RaycastPredicateColor predicate{ voxel_data, p_channel };

	float hit_distance;
	float hit_distance_prev;
	Vector3i hit_pos;
	Vector3i prev_pos;

	if (voxel_raycast(
				ray_origin, ray_dir, predicate, max_distance, hit_pos, prev_pos, hit_distance, hit_distance_prev
		)) {
		res.instantiate();
		res->position = hit_pos;
		res->previous_position = prev_pos;
		res->distance_along_ray = hit_distance;
		res->normal = to_vec3(hit_pos - prev_pos);
	}

	return res;
}

Ref<VoxelRaycastResult> raycast_generic(
		const VoxelData &voxel_data,
		const Ref<VoxelMesher> mesher,
		const Vector3 ray_origin,
		const Vector3 ray_dir,
		const float max_distance,
		const uint32_t p_collision_mask,
		const uint8_t binary_search_iterations,
		const bool normal_enabled
) {
	using namespace voxel::godot;

	Ref<VoxelRaycastResult> res;

	Ref<VoxelMesherBlocky> mesher_blocky;
	Ref<VoxelMesherCubes> mesher_cubes;

	if (try_get_as(mesher, mesher_blocky)) {
		res = raycast_blocky(voxel_data, **mesher_blocky, ray_origin, ray_dir, max_distance, p_collision_mask);

	} else if (try_get_as(mesher, mesher_cubes)) {
		res = raycast_nonzero(voxel_data, ray_origin, ray_dir, max_distance, voxel::VoxelBuffer::CHANNEL_COLOR);

	} else {
		res = raycast_sdf(voxel_data, ray_origin, ray_dir, max_distance, binary_search_iterations, normal_enabled);
	}

	return res;
}

Ref<VoxelRaycastResult> raycast_generic_world(
		const VoxelData &voxel_data,
		const Ref<VoxelMesher> mesher,
		const Transform3D &to_world,
		const Vector3 ray_origin_world,
		const Vector3 ray_dir_world,
		const float max_distance_world,
		const uint32_t p_collision_mask,
		const uint8_t binary_search_iterations,
		const bool normal_enabled
) {
	VOXEL_PROFILE_SCOPE();

	// TODO 在块上实现粗阶段以尽量减少锁定并提高性能

	// TODO 优化：体素射线投射使用的是 `get_voxel`，这是最慢的方式，但可以做得更快。
	// 有关如何实现改进的信息，请参阅 `VoxelToolLodTerrain`。

	// TODO 改用 “from/to” 参数，而不是 “from/dir/distance”

	const Vector3 ray_end_world = ray_origin_world + ray_dir_world * max_distance_world;

	const Transform3D to_local = to_world.affine_inverse();

	const Vector3 pos0_local = to_local.xform(ray_origin_world);
	const Vector3 pos1_local = to_local.xform(ray_end_world);

	const float max_distance_local_sq = pos0_local.distance_squared_to(pos1_local);
	if (max_distance_local_sq < 0.000001f) {
		return Ref<VoxelRaycastResult>();
	}
	const float max_distance_local = Math::sqrt(max_distance_local_sq);
	const Vector3 dir_local = (pos1_local - pos0_local) / max_distance_local;

	Ref<VoxelRaycastResult> res = raycast_generic(
			voxel_data,
			mesher,
			pos0_local,
			dir_local,
			max_distance_local,
			p_collision_mask,
			binary_search_iterations,
			normal_enabled
	);

	if (res.is_valid()) {
		const float max_distance_world_sq = ray_origin_world.distance_squared_to(ray_end_world);
		const float to_world_scale = max_distance_world_sq / max_distance_local_sq;

		res->distance_along_ray = res->distance_along_ray * to_world_scale;

		if (normal_enabled) {
			res->normal = to_world.basis.xform(res->normal);
		} else {
			res->normal = Vector3();
		}
	}

	return res;
}

} // namespace voxel
