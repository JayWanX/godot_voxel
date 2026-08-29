#[compute]
#version 450

// 接收一个网格和一组 tile，每个 tile 对应网格的一个立方体单元。
// 每个单元可能包含网格的几个三角形，
// 并且 tile 会沿某个轴定向，使其尽可能朝向更多的三角形。
// 我们从每个 tile 的每个像素向三角形发射射线，以找到世界空间位置。
// 命中位置和三角形索引将用于在这些位置求值体素数据，
// 进而用于烘焙纹理。

layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout (set = 0, binding = 0, std430) restrict readonly buffer MeshVertices {
	vec3 data[];
} u_vertices;

layout (set = 0, binding = 1, std430) restrict readonly buffer MeshIndices {
	int data[];
} u_indices;

layout (set = 0, binding = 2, std430) restrict readonly buffer CellTris {
	// 三角形索引列表。
	// 按块分组，对应 tile 内的三角形。
	// 每个块最多可有 5 个三角形索引。
	int data[];
} u_cell_tris;

layout (set = 0, binding = 3, std430) restrict readonly buffer AtlasInfo {
	// [tile index] => 单元格信息
	// X：
	// 单元格的打包 8 位坐标。
	// Y：
	// aaaaaaaa aaaaaaaa aaaaaaaa 0bbb00cc
	// a：`u_cell_tris.data` 数组中的 24 位索引。
	// b：3 位三角形数量。
	// c：2 位投影方向（0:X，1:Y，2:Z）
	// 全局调用 X 和 Y 表示我们在哪个像素中。
	ivec2 data[];
} u_tile_data;

layout (set = 0, binding = 4, std430) restrict readonly buffer Params {
	vec3 block_origin_world;
	// 图集中一个像素在世界空间中的大小
	float pixel_world_step;
	int tile_size_pixels;
} u_params;

layout (set = 0, binding = 5, std430) restrict writeonly buffer HitBuffer {
	// X、Y、Z 为命中位置
	// W 为整数三角形索引
	// 索引为 `pixel_pos_in_tile.x + pixel_pos_in_tile.y * tile_resolution + tile_index * (tile_resolution ^ 2)`
	vec4 positions[];
} u_hits;

const int TRI_NO_INTERSECTION = 0;
const int TRI_PARALLEL = 1;
const int TRI_INTERSECTION = 2;

int ray_intersects_triangle(vec3 p_from, vec3 p_dir, vec3 p_v0, vec3 p_v1, vec3 p_v2, out float out_distance) {
	const vec3 e1 = p_v1 - p_v0;
	const vec3 e2 = p_v2 - p_v0;
	const vec3 h = cross(p_dir, e2);
	const float a = dot(e1, h);

	if (abs(a) < 0.00001) {
		out_distance = -1.0;
		return TRI_PARALLEL;
	}

	const float f = 1.0f / a;

	const vec3 s = p_from - p_v0;
	const float u = f * dot(s, h);

	if ((u < 0.0) || (u > 1.0)) {
		out_distance = -1.0;
		return TRI_NO_INTERSECTION;
	}

	const vec3 q = cross(s, e1);

	const float v = f * dot(p_dir, q);

	if ((v < 0.0) || (u + v > 1.0)) {
		out_distance = -1.0;
		return TRI_NO_INTERSECTION;
	}

	// 在此阶段我们可以计算 t 以确定
	// 交点在线上的位置。
	const float t = f * dot(e2, q);

	if (t > 0.00001) { // 射线相交
		//r_res = p_from + p_dir * t;
		out_distance = t;
		return TRI_INTERSECTION;

	} else { // 这意味着存在直线相交，但不存在射线相交。
		out_distance = -1.0;
		return TRI_NO_INTERSECTION;
	}
}

void main() {
	const int tile_index = int(gl_GlobalInvocationID.z);

	const ivec2 tile_data = u_tile_data.data[tile_index];
	const int tri_count = (tile_data.y >> 4) & 0x7;

	if (tri_count == 0) {
		return;
	}

	const ivec2 pixel_pos_in_tile = ivec2(gl_GlobalInvocationID.xy);

	const int tri_info_begin = tile_data.y >> 8;
	const int projection = tile_data.y & 0x3;

	const int packed_cell_pos = tile_data.x;//u_tile_cell_positions.data[tile_index];
	const vec3 cell_pos_cells = vec3(
		packed_cell_pos & 0xff,
		(packed_cell_pos >> 8) & 0xff,
		(packed_cell_pos >> 16) & 0xff
	);
	const float cell_size_world = u_params.pixel_world_step * float(u_params.tile_size_pixels);
	const vec3 cell_origin_mesh = cell_size_world * cell_pos_cells;

	// 选择一个基，使 Z 为发射射线的轴。X 和 Y 为 tile 的横向轴。
	const vec3 ray_dir = vec3(float(projection == 0), float(projection == 1), float(projection == 2));
	const vec3 dx = vec3(float(projection == 1 || projection == 2), 0.0, float(projection == 0));
	const vec3 dy = vec3(0.0, float(projection == 0 || projection == 2), float(projection == 1));

	const vec2 pos_in_tile = u_params.pixel_world_step * vec2(pixel_pos_in_tile);
	const vec3 ray_origin_mesh = cell_origin_mesh
		 - 1.01 * ray_dir * cell_size_world
		 + pos_in_tile.x * dx + pos_in_tile.y * dy;

	// 查找最近的命中三角形
	const float no_hit_distance = 999999.0;
	float nearest_hit_distance = no_hit_distance;
	int nearest_hit_tri_index = -1;
	for (int i = 0; i < tri_count; ++i) {
		const int tri_index = u_cell_tris.data[tri_info_begin + i];
		const int ii0 = tri_index * 3;
		
		const int i0 = u_indices.data[ii0];
		const int i1 = u_indices.data[ii0 + 1];
		const int i2 = u_indices.data[ii0 + 2];

		const vec3 v0 = u_vertices.data[i0];
		const vec3 v1 = u_vertices.data[i1];
		const vec3 v2 = u_vertices.data[i2];

		float hit_distance;
		const int intersection_result = ray_intersects_triangle(ray_origin_mesh, ray_dir, v0, v1, v2, hit_distance);

		if (intersection_result == TRI_INTERSECTION && hit_distance < nearest_hit_distance) {
			nearest_hit_distance = hit_distance;
			nearest_hit_tri_index = tri_index;
		}
	}

	const int index = pixel_pos_in_tile.x + pixel_pos_in_tile.y * u_params.tile_size_pixels 
		+ tile_index * u_params.tile_size_pixels * u_params.tile_size_pixels;

	if (nearest_hit_tri_index != -1) {
		const vec3 hit_pos_world = ray_origin_mesh + ray_dir * nearest_hit_distance + u_params.block_origin_world;
		u_hits.positions[index] = vec4(hit_pos_world, nearest_hit_tri_index);
	} else {
		u_hits.positions[index] = vec4(0.0, 0.0, 0.0, -1.0);
	}
}
