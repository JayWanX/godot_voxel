#include "detail_rendering.h"
#include "../../edition/funcs.h"
#include "../../generators/voxel_generator.h"
#include "../../storage/voxel_data.h"
#include "../../storage/voxel_data_grid.h"
#include "../../util/godot/classes/image.h"
#include "../../util/godot/classes/image_texture.h"
#include "../../util/math/basis3f.h"
#include "../../util/math/conv.h"
#include "../../util/math/triangle.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"

namespace voxel {

namespace {

void dilate_normalmap(Span<Vector3f> normals, Vector2i size) {
	VOXEL_PROFILE_SCOPE();

	static const int s_dx[4] = { -1, 1, 0, 0 };
	static const int s_dy[4] = { 0, 0, -1, 1 };

	struct NewNormal {
		unsigned int loc;
		Vector3f normal;
	};
	static thread_local StdVector<NewNormal> tls_new_normals;
	tls_new_normals.clear();

	unsigned int loc = 0;
	for (int y = 0; y < size.y; ++y) {
		for (int x = 0; x < size.x; ++x) {
			if (normals[loc] != Vector3f()) {
				++loc;
				continue;
			}
			Vector3f sum;
			int count = 0;
			for (unsigned int di = 0; di < 4; ++di) {
				const int nx = x + s_dx[di];
				const int ny = y + s_dy[di];
				if (nx >= 0 && nx < size.x && ny >= 0 && ny < size.y) {
					const Vector3f nn = normals[nx + ny * size.y];
					if (nn != Vector3f()) {
						sum += nn;
						++count;
					}
				}
			}
			if (count > 0) {
				sum /= float(count);
				tls_new_normals.push_back(NewNormal{ loc, sum });
			}
			++loc;
		}
	}

	for (const NewNormal nn : tls_new_normals) {
		normals[nn.loc] = nn.normal;
	}
}

} // namespace

DetailTextureData::Tile compute_tile_info(
		const CurrentCellInfo &cell_info,
		Span<const Vector3f> mesh_normals,
		Span<const int> mesh_indices
) {
	Vector3f normal_sum;

	for (unsigned int triangle_index = 0; triangle_index < cell_info.triangle_count; ++triangle_index) {
		const unsigned int ii0 = cell_info.triangle_begin_indices[triangle_index];

		const unsigned vi0 = mesh_indices[ii0];
		const unsigned vi1 = mesh_indices[ii0 + 1];
		const unsigned vi2 = mesh_indices[ii0 + 2];

		const Vector3f normal0 = mesh_normals[vi0];
		const Vector3f normal1 = mesh_normals[vi1];
		const Vector3f normal2 = mesh_normals[vi2];

		normal_sum += normal0;
		normal_sum += normal1;
		normal_sum += normal2;
	}

#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(cell_info.position.x >= 0);
	VOXEL_ASSERT(cell_info.position.y >= 0);
	VOXEL_ASSERT(cell_info.position.z >= 0);
	VOXEL_ASSERT(cell_info.position.x < 256);
	VOXEL_ASSERT(cell_info.position.y < 256);
	VOXEL_ASSERT(cell_info.position.z < 256);
#endif
	const DetailTextureData::Tile tile{ //
										uint8_t(cell_info.position.x), //
										uint8_t(cell_info.position.y), //
										uint8_t(cell_info.position.z), //
										uint8_t(math::get_longest_axis(normal_sum))
	};
	return tile;
}

void get_axis_indices(math::Axis axis, unsigned int &ax, unsigned int &ay, unsigned int &az) {
	switch (axis) {
		case math::AXIS_X:
			ax = math::AXIS_Z;
			ay = math::AXIS_Y;
			az = math::AXIS_X;
			break;
		case math::AXIS_Y:
			ax = math::AXIS_X;
			ay = math::AXIS_Z;
			az = math::AXIS_Y;
			break;
		case math::AXIS_Z:
			ax = math::AXIS_X;
			ay = math::AXIS_Y;
			az = math::AXIS_Z;
			break;
		default:
			VOXEL_CRASH();
	}
}

typedef FixedArray<math::BakedIntersectionTriangleForFixedDirection, CurrentCellInfo::MAX_TRIANGLES> CellTriangles;

unsigned int prepare_triangles(
		const CurrentCellInfo &cell_info,
		const Vector3f direction,
		CellTriangles &baked_triangles,
		Span<const Vector3f> mesh_vertices,
		Span<const int> mesh_indices
) {
	unsigned int triangle_count = 0;

	for (unsigned int ti = 0; ti < cell_info.triangle_count; ++ti) {
		const unsigned int ii0 = cell_info.triangle_begin_indices[ti];
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(ii0 + 2 < mesh_indices.size());
#endif
		const unsigned vi0 = mesh_indices[ii0];
		const unsigned vi1 = mesh_indices[ii0 + 1];
		const unsigned vi2 = mesh_indices[ii0 + 2];

		const Vector3f a = mesh_vertices[vi0];
		const Vector3f b = mesh_vertices[vi1];
		const Vector3f c = mesh_vertices[vi2];
		math::BakedIntersectionTriangleForFixedDirection baked_triangle;
		// 三角形可能与方向平行
		if (baked_triangle.bake(a, b, c, direction)) {
			baked_triangles[triangle_count] = baked_triangle;
			++triangle_count;
		}
	}

	return triangle_count;
}

inline uint8_t unorm_to_u8(float x) {
	return math::clamp(255.f * x, 0.f, 255.f);
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
Vector2f octahedron_wrap(Vector2f v) {
	return (Vector2f(1.f) - math::abs(Vector2f(v.y, v.x))) * math::sign_nonzero(v);
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
inline Vector2f encode_normal_octahedron(Vector3f n) {
	const float sum = Math::abs(n.x) + Math::abs(n.y) + Math::abs(n.z);
	n.x /= sum;
	n.y /= sum;
	Vector2f r = n.z >= 0.f ? Vector2f(n.x, n.y) : octahedron_wrap(Vector2f(n.x, n.y));
	r = r * 0.5f + Vector2f(0.5f);
	return r;
}

inline Vector3f encode_normal_xyz(const Vector3f n) {
	return Vector3f(0.5f) + 0.5f * n;
}

void query_sdf_with_edits(
		VoxelGenerator &generator,
#ifdef VOXEL_ENABLE_MODIFIERS
		const VoxelModifierStack &modifiers,
#endif
		const VoxelDataGrid &grid,
		Span<const float> query_x_buffer,
		Span<const float> query_y_buffer,
		Span<const float> query_z_buffer,
		Span<float> query_sdf_buffer,
		Vector3f query_min_pos,
		Vector3f query_max_pos
) {
	VOXEL_PROFILE_SCOPE();

	VoxelDataGrid::LockRead rlock(grid);

	for (unsigned int query_index = 0; query_index < query_sdf_buffer.size(); ++query_index) {
		const Vector3 posf(query_x_buffer[query_index], query_y_buffer[query_index], query_z_buffer[query_index]);

		// TODO 优化：一次处理的采样数量更多，以便更好地受益于批量处理
		// 尽可能一次求多个值：对立方插值取 8 个采样点
		FixedArray<float, 8> sd_samples;
		FixedArray<float, 8> x_gen;
		FixedArray<float, 8> y_gen;
		FixedArray<float, 8> z_gen;
		FixedArray<uint8_t, 8> i_gen;
		unsigned int gen_count = 0;

		const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;

		const Vector3i posi0 = math::floor_to_int(posf);
		unsigned int i = 0;

		// 从已编辑体素收集 8 个采样点
		{
			VOXEL_PROFILE_SCOPE();
			for (int z = 0; z < 2; ++z) {
				for (int y = 0; y < 2; ++y) {
					for (int x = 0; x < 2; ++x) {
						const Vector3i posi = posi0 + Vector3i(x, y, z);
						// TODO 优化：与其为单个采样加锁，不如用空间锁锁定整个区域
						//      （因为否则我们无法同时锁定多个数据块，否则会导致死锁）
						// TODO 优化：可以让网格收集原始通道以便直接访问
						//      （需要先完成前一项优化）
						if (!grid.try_get_voxel_f(posi, sd_samples[i], channel)) {
							// 未被编辑，加入待生成的体素列表
							x_gen[gen_count] = posi.x;
							y_gen[gen_count] = posi.y;
							z_gen[gen_count] = posi.z;
							i_gen[gen_count] = i;
							++gen_count;
						}
						++i;
					}
				}
			}
		}

		// 用生成器补齐采样点。注意，这些采样点未经缩放，因为我们处理的是浮点数
		// 而非编码后的缓冲值。
		if (gen_count > 0) {
			VOXEL_PROFILE_SCOPE();
			FixedArray<float, 8> gen_samples;

			generator.generate_series(
					to_span(x_gen, gen_count),
					to_span(y_gen, gen_count),
					to_span(z_gen, gen_count),
					channel,
					to_span(gen_samples, gen_count),
					query_min_pos,
					query_max_pos
			);

#ifdef VOXEL_ENABLE_MODIFIERS
			modifiers.apply(
					to_span(x_gen, gen_count),
					to_span(y_gen, gen_count),
					to_span(z_gen, gen_count),
					to_span(gen_samples, gen_count),
					query_min_pos,
					query_max_pos
			);
#endif

			for (unsigned int j = 0; j < gen_count; ++j) {
				sd_samples[i_gen[j]] = gen_samples[j];
			}
		}

		// 插值
		const float sd_interp = math::interpolate_trilinear(
				sd_samples[0],
				sd_samples[1],
				sd_samples[5],
				sd_samples[4],
				sd_samples[2],
				sd_samples[3],
				sd_samples[7],
				sd_samples[6],
				math::fract(posf)
		);

		query_sdf_buffer[query_index] = sd_interp;
	}
}

// 可在 tile 内获取已编辑数据块的最大网格尺寸。
// 超过此尺寸后，需要查询的单元格太多，算法将回退到生成器。
static const unsigned int MAX_EDITED_BLOCKS_ACROSS = 8;

bool try_query_edited_blocks(
		VoxelDataGrid &grid,
		const VoxelData &voxel_data,
		Vector3f query_min_pos,
		Vector3f query_max_pos,
		uint32_t &skipped_count_due_to_high_volume
) {
	VOXEL_PROFILE_SCOPE();

	// 向外扩 1 个像素，以防存在相邻的已编辑体素。若不这样做，会在 LOD0 数据块
	// 边界处形成网格状图案，因为插值时这些位置附近的采样点会假设没有已编辑的邻居
	const Vector3i query_min_pos_i = math::floor_to_int(query_min_pos) - Vector3iUtil::create(1);
	const Vector3i query_max_pos_i = math::ceil_to_int(query_max_pos) + Vector3iUtil::create(1);

	{
		const Box3i voxel_box = Box3i::from_min_max(query_min_pos_i, query_max_pos_i);
		const Vector3i block_box_size = voxel_box.size >> constants::DEFAULT_BLOCK_SIZE_PO2;
		const int64_t block_volume = Vector3iUtil::get_volume_u64(block_box_size);
		// TODO 不要硬编码数据块大小（尽管目前我还没有计划让其可配置）
		if (block_volume > math::cubed(MAX_EDITED_BLOCKS_ACROSS)) {
			// 包围盒太大，无法快速稀疏读取，将不处理编辑。回退到生成器。
			// 一种加速方法是使用八叉树存储已编辑数据的位置。
			// 或者我们不得不使用最慢的查询模型，为每个体素遍历数据结构。
			++skipped_count_due_to_high_volume;
			return false;
		}

		// 在可能进行大量查询时，使用 LOD mip 做一次粗略检查。
		if (block_volume <= 8 || voxel_data.has_blocks_with_voxels_in_area_broad_mip_test(voxel_box)) {
			voxel_data.get_blocks_grid(grid, voxel_box, 0);
		}
		// const VoxelDataLodMap::Lod &lod0 = voxel_data.lods[0];
		// RWLockRead rlock(lod0.map_lock);
		// tls_grid.reference_area(lod0.map, voxel_box);
	}

	return grid.has_any_block();
}

struct ClearVoxelDataGridOnExit {
	VoxelDataGrid &grid;
	inline ~ClearVoxelDataGridOnExit() {
		grid.clear();
	}
};

inline void query_sdf(
		VoxelGenerator &generator,
		const VoxelDataGrid *edited_voxel_data,
#ifdef VOXEL_ENABLE_MODIFIERS
		const VoxelModifierStack *modifiers,
#endif
		Span<const float> query_x_buffer,
		Span<const float> query_y_buffer,
		Span<const float> query_z_buffer,
		Span<float> query_sdf_buffer,
		Vector3f query_min_pos,
		Vector3f query_max_pos
) {
	VOXEL_PROFILE_SCOPE();

	if (edited_voxel_data != nullptr) {
#ifdef VOXEL_ENABLE_MODIFIERS
		// 通常如果有编辑，也意味着存在修改器堆栈。它本可以是可选的，但目前
		// 也没有理由让它缺失。
		VOXEL_ASSERT(modifiers != nullptr);
#endif

		query_sdf_with_edits(
				generator,
#ifdef VOXEL_ENABLE_MODIFIERS
				*modifiers,
#endif
				*edited_voxel_data,
				query_x_buffer,
				query_y_buffer,
				query_z_buffer,
				query_sdf_buffer,
				query_min_pos,
				query_max_pos
		);
	} else {
		// 仅使用生成器。

		// 注意，这些采样点未经缩放，因为我们处理的是浮点数而非编码后的缓冲值。
		generator.generate_series(
				query_x_buffer,
				query_y_buffer,
				query_z_buffer,
				VoxelBuffer::CHANNEL_SDF,
				query_sdf_buffer,
				query_min_pos,
				query_max_pos
		);

#ifdef VOXEL_ENABLE_MODIFIERS
		if (modifiers != nullptr) {
			modifiers->apply(
					query_x_buffer, query_y_buffer, query_z_buffer, query_sdf_buffer, query_min_pos, query_max_pos
			);
		}
#endif
	}

#if DEBUG_ENABLED
	for (const float sd : query_sdf_buffer) {
		VOXEL_ASSERT(!(math::is_nan(sd) || math::is_inf(sd)));
	}
#endif
}

// 对于网格的每个非空单元格，根据单元格中的三角形法线选择轴对齐投影。
// 对单元格内的体素进行采样，从 SDF 计算一块世界空间法线 tile。
void compute_detail_texture_data(
		ICellIterator &cell_iterator,
		Span<const Vector3f> mesh_vertices,
		Span<const Vector3f> mesh_normals,
		Span<const int> mesh_indices,
		DetailTextureData &normal_map_data,
		unsigned int tile_resolution,
		VoxelGenerator &generator,
		const VoxelData *voxel_data,
		Vector3i origin_in_voxels,
		Vector3i size_in_voxels,
		unsigned int lod_index,
		bool octahedral_encoding,
		float max_deviation_radians,
		bool edited_tiles_only
) {
	VOXEL_PROFILE_SCOPE();

	VOXEL_ASSERT_RETURN(generator.supports_series_generation());
	VOXEL_ASSERT_RETURN_MSG(max_deviation_radians > 0.001f, "Max deviation angle is too small.");

	const float max_deviation_cosine = Math::cos(max_deviation_radians);
	const float max_deviation_sine = Math::sin(max_deviation_radians);

	const unsigned int encoded_normal_size = octahedral_encoding ? 2 : 3;

	const unsigned int cell_size = 1 << lod_index;
	const float step = float(cell_size) / tile_resolution;

	if (!edited_tiles_only) {
		const unsigned int cell_count = cell_iterator.get_count();
		normal_map_data.tiles.reserve(cell_count);
		normal_map_data.normals.reserve(math::squared(tile_resolution) * cell_count * encoded_normal_size);
	}

	if (voxel_data != nullptr &&
		!voxel_data->has_blocks_with_voxels_in_area_broad_mip_test(Box3i(origin_in_voxels, size_in_voxels))) {
		// 完全忽略编辑
		voxel_data = nullptr;
		if (edited_tiles_only) {
			return;
		}
	}

	uint32_t skipped_count_due_to_high_volume = 0;

	CurrentCellInfo cell_info;
	for (unsigned int cell_index = 0; cell_iterator.next(cell_info); ++cell_index) {
		// 复用内存，因为它会被频繁使用
		static thread_local VoxelDataGrid tls_voxel_data_grid;
		// 确保清理对体素缓冲的引用
		ClearVoxelDataGridOnExit grid_clear_on_exit{ tls_voxel_data_grid };

		const Vector3f cell_origin_world = to_vec3f(origin_in_voxels + cell_info.position * cell_size);

		// 在只想要包含已编辑体素的 tile 时，尽早检查以便跳过该 tile。
		const bool cell_has_edits = voxel_data != nullptr &&
				try_query_edited_blocks(tls_voxel_data_grid,
										*voxel_data,
										cell_origin_world,
										cell_origin_world + Vector3f(cell_size),
										skipped_count_due_to_high_volume);
		if (!cell_has_edits && edited_tiles_only) {
			continue;
		} else if (edited_tiles_only) {
			normal_map_data.tile_indices.push_back(cell_index);
		}

		const DetailTextureData::Tile tile = compute_tile_info(cell_info, mesh_normals, mesh_indices);
		normal_map_data.tiles.push_back(tile);

		unsigned int ax;
		unsigned int ay;
		unsigned int az;
		get_axis_indices(math::Axis(tile.axis), ax, ay, az);

		Vector3f quad_origin_world = cell_origin_world;
		quad_origin_world[az] += cell_size * 0.5f;

		Vector3f direction;
		direction[az] = 1.f;

		static thread_local StdVector<Vector2i> tls_tile_sample_positions;
		tls_tile_sample_positions.clear();
		tls_tile_sample_positions.reserve(math::squared(tile_resolution));

		static thread_local StdVector<uint8_t> tls_tile_sample_triangle_index;
		tls_tile_sample_triangle_index.clear();
		tls_tile_sample_triangle_index.reserve(math::squared(tile_resolution));

		// 每个法线需要 4 个采样点：
		// (x,   y,   z  )
		// (x+s, y,   z  )
		// (x,   y+s, z  )
		// (x,   y,   z+s)
		const unsigned int max_buffer_size = math::squared(tile_resolution) * 4;

		static thread_local StdVector<float> tls_sdf_buffer;
		static thread_local StdVector<float> tls_x_buffer;
		static thread_local StdVector<float> tls_y_buffer;
		static thread_local StdVector<float> tls_z_buffer;
		tls_sdf_buffer.clear();
		tls_x_buffer.clear();
		tls_y_buffer.clear();
		tls_z_buffer.clear();
		tls_sdf_buffer.reserve(max_buffer_size);
		tls_x_buffer.reserve(max_buffer_size);
		tls_y_buffer.reserve(max_buffer_size);
		tls_z_buffer.reserve(max_buffer_size);

		// 优化三角形
		CellTriangles baked_triangles;
		unsigned int triangle_count =
				prepare_triangles(cell_info, direction, baked_triangles, mesh_vertices, mesh_indices);

		// 计算三角形法线
		FixedArray<Vector3f, CurrentCellInfo::MAX_TRIANGLES> triangle_normals;
		for (unsigned int i = 0; i < triangle_count; ++i) {
			const math::BakedIntersectionTriangleForFixedDirection &tri = baked_triangles[i];
			const Vector3f tri_normal = math::normalized(math::cross(tri.e2, tri.e1));
			triangle_normals[i] = tri_normal;
		}

		// 填充查询缓冲
		{
			VOXEL_PROFILE_SCOPE_NAMED("Compute positions");
			for (unsigned int yi = 0; yi < tile_resolution; ++yi) {
				for (unsigned int xi = 0; xi < tile_resolution; ++xi) {
					// TODO 计算法线时是否给中心差分添加偏移？
					Vector3f pos000 = quad_origin_world;
					// 此处转换为 `int`，因为即使目标是 float，临时值也可能是负的 uint
					pos000[ax] += int(xi) * step;
					pos000[ay] += int(yi) * step;

					// 投影到三角形
					const Vector3f ray_origin_world = pos000 - direction * cell_size;
					const Vector3f ray_origin_mesh = ray_origin_world - to_vec3f(origin_in_voxels);
					float nearest_hit_distance = 999999.f;
					unsigned int hit_triangle_index = triangle_count;
					for (unsigned int ti = 0; ti < triangle_count; ++ti) {
						const math::TriangleIntersectionResult result =
								baked_triangles[ti].intersect(ray_origin_mesh, direction);
						if (result.case_id == math::TriangleIntersectionResult::INTERSECTION &&
							result.distance < nearest_hit_distance) {
							nearest_hit_distance = result.distance;
							hit_triangle_index = ti;
						}
					}

					if (hit_triangle_index == triangle_count) {
						// 如果没有三角形则不查询
						continue;
					}

					pos000 = ray_origin_world + direction * nearest_hit_distance;
					tls_tile_sample_positions.push_back(Vector2i(xi, yi));
					tls_tile_sample_triangle_index.push_back(hit_triangle_index);

					tls_x_buffer.push_back(pos000.x);
					tls_y_buffer.push_back(pos000.y);
					tls_z_buffer.push_back(pos000.z);

					Vector3f pos100 = pos000;
					pos100.x += step;
					tls_x_buffer.push_back(pos100.x);
					tls_y_buffer.push_back(pos100.y);
					tls_z_buffer.push_back(pos100.z);

					Vector3f pos010 = pos000;
					pos010.y += step;
					tls_x_buffer.push_back(pos010.x);
					tls_y_buffer.push_back(pos010.y);
					tls_z_buffer.push_back(pos010.z);

					Vector3f pos001 = pos000;
					pos001.z += step;
					tls_x_buffer.push_back(pos001.x);
					tls_y_buffer.push_back(pos001.y);
					tls_z_buffer.push_back(pos001.z);
				}
			}
		}

		tls_sdf_buffer.resize(tls_x_buffer.size());

		{
			const VoxelDataGrid *edits_grid = cell_has_edits ? &tls_voxel_data_grid : nullptr;
#ifdef VOXEL_ENABLE_MODIFIERS
			const VoxelModifierStack *modifiers = voxel_data != nullptr ? &voxel_data->get_modifiers() : nullptr;
#endif

			// 查询体素数据
			query_sdf(
					generator,
					edits_grid,
#ifdef VOXEL_ENABLE_MODIFIERS
					modifiers,
#endif
					to_span(tls_x_buffer),
					to_span(tls_y_buffer),
					to_span(tls_z_buffer),
					to_span(tls_sdf_buffer),
					cell_origin_world,
					cell_origin_world + Vector3f(cell_size)
			);
		}

		static thread_local StdVector<Vector3f> tls_tile_normals;
		tls_tile_normals.clear();
		tls_tile_normals.resize(math::squared(tile_resolution));

		// 从 SDF 结果计算法线
		{
			VOXEL_PROFILE_SCOPE_NAMED("Compute normals");
			VOXEL_ASSERT(tls_tile_sample_positions.size() == tls_tile_sample_triangle_index.size());

			unsigned int bi = 0;

			for (unsigned int si = 0; si < tls_tile_sample_positions.size(); ++si) {
				const Vector2i sample_position = tls_tile_sample_positions[si];
				const uint8_t sample_tri_index = tls_tile_sample_triangle_index[si];

				const unsigned int bi000 = bi;
				const unsigned int bi100 = bi + 1;
				const unsigned int bi010 = bi + 2;
				const unsigned int bi001 = bi + 3;
				bi += 4;
				// TODO 真希望这个问题已解决 https://github.com/godotengine/godot/issues/31608
#ifdef DEBUG_ENABLED
				VOXEL_ASSERT(bi000 < tls_sdf_buffer.size());
				VOXEL_ASSERT(bi100 < tls_sdf_buffer.size());
				VOXEL_ASSERT(bi010 < tls_sdf_buffer.size());
				VOXEL_ASSERT(bi001 < tls_sdf_buffer.size());
#endif
				const float sd000 = tls_sdf_buffer[bi000];
				const float sd100 = tls_sdf_buffer[bi100];
				const float sd010 = tls_sdf_buffer[bi010];
				const float sd001 = tls_sdf_buffer[bi001];

				Vector3f normal = math::normalized(Vector3f(sd100 - sd000, sd010 - sd000, sd001 - sd000));

				// 如果法线与三角形法线的点积超过阈值，则限制法线方向。
				// 这有助于在极低 LOD 下避免法线翻转，因为此时偏差非常大。在
				// SolarSystem 演示中，它可能会从地表捕捉到洞穴，导致出现黑斑。
				const Vector3f &tri_normal = triangle_normals[sample_tri_index];
				const float tdot = math::dot(normal, tri_normal);
				if (tdot < max_deviation_cosine) {
					if (tdot < -0.999) {
						normal = tri_normal;
					} else {
						const Vector3f axis = math::normalized(math::cross(tri_normal, normal));
						normal = math::rotated(tri_normal, axis, max_deviation_cosine, max_deviation_sine);
					}
				}

				const unsigned int normal_index = sample_position.x + sample_position.y * tile_resolution;
#ifdef DEBUG_ENABLED
				VOXEL_ASSERT(normal_index < tls_tile_normals.size());
#endif
				tls_tile_normals[normal_index] = normal;
			}
		}

		for (unsigned int dilation_steps = 0; dilation_steps < 2; ++dilation_steps) {
			// 填充三角形边界周围的一些像素，以便在着色器中靠近边界采样时留出余量
			dilate_normalmap(to_span(tls_tile_normals), Vector2i(tile_resolution, tile_resolution));
		}

		// 边处理边调整大小，因为根据设置我们可能需要跳过某些单元格
		const unsigned int tile_begin = normal_map_data.normals.size();
		normal_map_data.normals.resize(
				normal_map_data.normals.size() + math::squared(tile_resolution) * encoded_normal_size
		);

		// 编码法线
		if (octahedral_encoding) {
			for (unsigned int i = 0; i < tls_tile_normals.size(); ++i) {
				const unsigned int offset = tile_begin + i * encoded_normal_size;
				VOXEL_ASSERT(offset + encoded_normal_size <= normal_map_data.normals.size());
				const Vector2f n = encode_normal_octahedron(tls_tile_normals[i]);
				normal_map_data.normals[offset + 0] = unorm_to_u8(n.x);
				normal_map_data.normals[offset + 1] = unorm_to_u8(n.y);
			}
		} else {
			for (unsigned int i = 0; i < tls_tile_normals.size(); ++i) {
				const unsigned int offset = tile_begin + i * encoded_normal_size; //
				VOXEL_ASSERT(offset + encoded_normal_size <= normal_map_data.normals.size());
				const Vector3f n = encode_normal_xyz(tls_tile_normals[i]);
				normal_map_data.normals[offset + 0] = unorm_to_u8(n.x);
				normal_map_data.normals[offset + 1] = unorm_to_u8(n.y);
				normal_map_data.normals[offset + 2] = unorm_to_u8(n.z);
			}
		}
	}

	if (skipped_count_due_to_high_volume > 0) {
		// 在此记录日志以减少刷屏
		VOXEL_PRINT_VERBOSE(format(
				"Virtual normalmaps: fell back on generator for {} tiles, box too big to render edited voxels (lod {})",
				skipped_count_due_to_high_volume,
				lod_index
		));
	}
}

Ref<Image> store_lookup_to_image(const StdVector<DetailTextureData::Tile> &tiles, Vector3i block_size) {
	VOXEL_PROFILE_SCOPE();

	const unsigned int sqri = get_square_grid_size_from_item_count(Vector3iUtil::get_volume_u64(block_size));

	PackedByteArray bytes;
	{
		const unsigned int pixel_size = 2;
		bytes.resize(math::squared(sqri) * pixel_size);

		uint8_t *bytes_w = bytes.ptrw();
		memset(bytes_w, 0, bytes.size());

		const unsigned int deck_size = block_size.x * block_size.y;
#ifdef DEBUG_ENABLED
		bool tile_index_overflow = false;
#endif

		for (unsigned int tile_index = 0; tile_index < tiles.size(); ++tile_index) {
			const DetailTextureData::Tile tile = tiles[tile_index];
			// RG 位布局：tttttttt aatttttt
			const uint8_t r = tile_index & 0xff;
			const uint8_t g = ((tile_index >> 8) & 0x3f) | (tile.axis << 6);
#ifdef DEBUG_ENABLED
			if (tile_index > 0x3fff && !tile_index_overflow) {
				tile_index_overflow = true;
				VOXEL_PRINT_VERBOSE("Tile index overflow");
			}
#endif
			const unsigned int pi = pixel_size * (tile.x + tile.y * block_size.x + tile.z * deck_size);
			VOXEL_ASSERT(int(pi) < bytes.size());
			bytes_w[pi] = r;
			bytes_w[pi + 1] = g;
		}
	}

	Ref<Image> image = Image::create_from_data(sqri, sqri, false, Image::FORMAT_RG8, bytes);
	return image;
}

#ifdef VOXEL_VIRTUAL_TEXTURE_USE_TEXTURE_ARRAY

Vector<Ref<Image>> store_atlas_to_image_array(
		const StdVector<uint8_t> &normals,
		unsigned int tile_resolution,
		unsigned int tile_count,
		bool octahedral_encoding
) {
	VOXEL_PROFILE_SCOPE();

	const unsigned int pixel_size = octahedral_encoding ? 2 : 4;
	const Image::Format format =
			octahedral_encoding ? Image::FORMAT_RG8 :
								// 我们并不使用 alpha 通道，但 Godot 会就 RGB8 不被 GPU 支持
								// 反复发出警告。所以目前我们浪费一点内存，这对于
								// 需要大量存储的纹理来说很不幸。也许有一天它会派上用场。
			Image::FORMAT_RGBA8;
	const unsigned int tile_size_in_pixels = math::squared(tile_resolution);
	const unsigned int tile_size_in_bytes = tile_size_in_pixels * pixel_size;

	Vector<Ref<Image>> tile_images;
	tile_images.resize(tile_count);

	for (unsigned int tile_index = 0; tile_index < tile_count; ++tile_index) {
		PackedByteArray bytes;
		{
			bytes.resize(tile_size_in_bytes);
			memcpy(bytes.ptrw(), normals.data() + tile_index * tile_size_in_bytes, tile_size_in_bytes);
		}

		Ref<Image> image = Image::create_from_data(tile_resolution, tile_resolution, false, format, bytes);

		tile_images.write[tile_index] = image;
		// image->save_png(String("debug_atlas_{0}.png").format(varray(tile_index)));
	}

	return tile_images;
}

#endif

Ref<Image> store_atlas_to_image(
		const StdVector<uint8_t> &normals,
		unsigned int tile_resolution,
		unsigned int tile_count,
		bool octahedral_encoding
) {
	VOXEL_PROFILE_SCOPE();

	const unsigned int pixel_size = octahedral_encoding ? 2 : 4;
	const Image::Format format =
			octahedral_encoding ? Image::FORMAT_RG8 :
								// 我们并不使用 alpha 通道，但 Godot 会就 RGB8 不被 GPU 支持
								// 反复发出警告。所以目前我们浪费一点内存，这对于
								// 需要大量存储的纹理来说很不幸。也许有一天它会派上用场。
			Image::FORMAT_RGBA8;
	const unsigned int tile_size_in_pixels = math::squared(tile_resolution);
	const unsigned int tile_size_in_bytes = tile_size_in_pixels * pixel_size;

	const unsigned int tiles_across = get_square_grid_size_from_item_count(tile_count);
	const unsigned int pixels_across = tiles_across * tile_resolution;

	PackedByteArray bytes;
	bytes.resize(math::squared(tiles_across) * tile_size_in_bytes);
	Span<uint8_t> bytes_span(bytes.ptrw(), bytes.size());

	for (unsigned int tile_index = 0; tile_index < tile_count; ++tile_index) {
		const Vector2i tile_pos_pixels =
				int(tile_resolution) * Vector2i(tile_index % tiles_across, tile_index / tiles_across);
		Span<const uint8_t> tile =
				to_span_from_position_and_size(normals, tile_index * tile_size_in_bytes, tile_size_in_bytes);
		copy_2d_region_from_packed_to_atlased(
				bytes_span,
				Vector2i(pixels_across, pixels_across),
				tile,
				Vector2i(tile_resolution, tile_resolution),
				tile_pos_pixels,
				pixel_size
		);
	}

	Ref<Image> atlas = Image::create_from_data(pixels_across, pixels_across, false, format, bytes);
	return atlas;
}

DetailImages store_normalmap_data_to_images(
		const DetailTextureData &data,
		unsigned int tile_resolution,
		Vector3i block_size,
		bool octahedral_encoding
) {
	VOXEL_PROFILE_SCOPE();

	DetailImages images;
#ifdef VOXEL_VIRTUAL_TEXTURE_USE_TEXTURE_ARRAY
	images.atlas = store_atlas_to_image_array(data.normals, tile_resolution, data.tiles.size(), octahedral_encoding);
#else
	images.atlas = store_atlas_to_image(data.normals, tile_resolution, data.tiles.size(), octahedral_encoding);
#endif
	images.lookup = store_lookup_to_image(data.tiles, block_size);
	return images;
}

// 将法线贴图数据转换为纹理。它们可在着色器中用于应用法线并获得额外的视觉细节。
DetailTextures store_normalmap_data_to_textures(const DetailImages &data) {
	VOXEL_PROFILE_SCOPE();

	DetailTextures textures;

	{
		VOXEL_PROFILE_SCOPE_NAMED("Atlas texture");
#ifdef VOXEL_VIRTUAL_TEXTURE_USE_TEXTURE_ARRAY
		Ref<Texture2DArray> atlas;
		atlas.instantiate();
		const Error err = atlas->create_from_images(data.atlas);
		VOXEL_ASSERT_RETURN_V(err == OK, textures);
		textures.atlas = atlas;
#else
		textures.atlas = ImageTexture::create_from_image(data.atlas);
#endif
	}

	{
		VOXEL_PROFILE_SCOPE_NAMED("Lookup texture");
		Ref<ImageTexture> lookup = ImageTexture::create_from_image(data.lookup);
		textures.lookup = lookup;
	}

	return textures;
}

unsigned int get_detail_texture_tile_resolution_for_lod(
		const DetailRenderingSettings &settings,
		unsigned int lod_index
) {
	const unsigned int relative_lod_index = lod_index - settings.begin_lod_index;
	const unsigned int tile_resolution = math::clamp(
			int(settings.tile_resolution_min << relative_lod_index),
			int(settings.tile_resolution_min),
			int(settings.tile_resolution_max)
	);
	return tile_resolution;
}

void copy_2d_region_from_packed_to_atlased(
		Span<uint8_t> dst,
		const Vector2i dst_size,
		const Span<const uint8_t> src,
		const Vector2i src_size,
		const Vector2i dst_pos,
		const unsigned int item_size_in_bytes
) {
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(src_size.x >= 0 && src_size.y >= 0);
	VOXEL_ASSERT(dst_size.x >= 0 && dst_size.y >= 0);
	VOXEL_ASSERT(
			dst_pos.x >= 0 && dst_pos.y >= 0 && dst_pos.x + src_size.x <= dst_size.x &&
			dst_pos.y + src_size.y <= dst_size.y
	);
	VOXEL_ASSERT(src.size() == src_size.x * src_size.y * item_size_in_bytes);
	VOXEL_ASSERT(dst.size() == dst_size.x * dst_size.y * item_size_in_bytes);
	VOXEL_ASSERT(!src.overlaps(dst));
#endif
	const unsigned int dst_begin = (dst_pos.x + dst_pos.y * dst_size.x) * item_size_in_bytes;
	const unsigned int src_row_size = src_size.x * item_size_in_bytes;
	const unsigned int dst_row_size = dst_size.x * item_size_in_bytes;
	uint8_t *dst_p = dst.data() + dst_begin;
	const uint8_t *src_p = src.data();
	for (unsigned int src_y = 0; src_y < (unsigned int)src_size.y; ++src_y) {
		memcpy(dst_p, src_p, src_row_size);
		dst_p += dst_row_size;
		src_p += src_row_size;
	}
}

} // namespace voxel
