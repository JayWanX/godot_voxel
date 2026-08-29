#ifndef VOXEL_MESH_SDF_H
#define VOXEL_MESH_SDF_H

#include "../storage/voxel_buffer.h"
#include "../util/containers/span.h"
#include "../util/containers/std_vector.h"
#include "../util/math/vector3f.h"
#include "../util/math/vector3i.h"
#include "../util/tasks/threaded_task.h"

#include <atomic>
#include <memory>

namespace voxel::mesh_sdf {

// 用于从三维三角形网格生成有符号距离场的工具函数。

struct Triangle {
	// 需要从网格提供的顶点。
	Vector3f v1;
	Vector3f v2;
	Vector3f v3;

	// 使用 `prepare_triangles()` 预计算的值。
	Vector3f v21;
	Vector3f v32;
	Vector3f v13;
	Vector3f nor;
	Vector3f v21_cross_nor;
	Vector3f v32_cross_nor;
	Vector3f v13_cross_nor;
	float inv_v21_length_squared;
	float inv_v32_length_squared;
	float inv_v13_length_squared;
	float inv_nor_length_squared;
};

struct Chunk {
	Vector3i pos;
	StdVector<const Chunk *> near_chunks;
	StdVector<const Triangle *> triangles;
};

struct ChunkGrid {
	StdVector<Chunk> chunks;
	Vector3i size; // 网格的尺寸（以格子为单位）
	Vector3f min_pos; // 网格下角在空间单位中的位置
	float chunk_size; // 一个立方体格子的大小（以空间单位计）
};

class GenMeshSDFSubBoxTask : public IThreadedTask {
public:
	struct SharedData {
		StdVector<Triangle> triangles;
		std::atomic_int pending_jobs = { 0 };
		VoxelBuffer buffer;
		Vector3f min_pos;
		Vector3f max_pos;
		ChunkGrid chunk_grid;
		bool use_chunk_grid = false;
		bool boundary_sign_fix = false;

		SharedData() : buffer(VoxelBuffer::ALLOCATOR_DEFAULT) {}
	};

	std::shared_ptr<SharedData> shared_data;
	Box3i box;

	void run(ThreadedTaskContext &ctx) override;

	// 当 `pending_jobs` 归零时被调用。
	virtual void on_complete() {}

	virtual const char *get_debug_name() const override {
		return "GenMeshSDFSubBox";
	}
};

// 计算网格的一种表示形式，以便更高效地计算到三角形的距离。
bool prepare_triangles(
		Span<const Vector3> vertices,
		Span<const int> indices,
		StdVector<Triangle> &triangles,
		Vector3f &out_min_pos,
		Vector3f &out_max_pos
);

// 对网格的三角形进行分区，以便在计算 SDF 时减少需要检查的三角形数量。
// 空间被细分为一个由块组成的网格。与块重叠的三角形会被列出。
void partition_triangles(
		int subdiv,
		Span<const Triangle> triangles,
		Vector3f min_pos,
		Vector3f max_pos,
		ChunkGrid &chunk_grid
);

// 对于每个块，找出哪些其它非空块与它相邻。细分的数量需要仔细选择：
// 太低会导致被跳过的三角形变少，太高会使分区变慢。
// 这对使用 ChunkGrid 的函数是必要的。
void compute_near_chunks(ChunkGrid &chunk_grid);

// 一种从网格获取采样 SDF 的朴素方法，对每个格子检查每个三角形。它很精确，但比
// 其它技术慢得多，可作为基于 CPU 的替代方案，用于实时性要求较低的
// 任务。网格必须是闭合的，否则 SDF 会包含错误。
void generate_mesh_sdf_naive(
		Span<float> sdf_grid,
		const Vector3i res,
		Span<const Triangle> triangles,
		const Vector3f min_pos,
		const Vector3f max_pos
);

// 通过对三角形进行分区来更快地计算 SDF，同时保持与检查所有三角形相同的精度。
// 对于 Suzanne 网格（一次细分后约 3900 个三角形）且 `subdiv = 32` 时，比在
// 每个格子上检查每个三角形快约 8 倍。
void generate_mesh_sdf_partitioned(
		Span<float> sdf_grid,
		const Vector3i res,
		Span<const Triangle> triangles,
		const Vector3f min_pos,
		const Vector3f max_pos,
		int subdiv
);

// 生成一个近似结果。
// 将网格细分为每个覆盖 4*4*4 个格子的节点。
// 如果某个节点角点的距离值接近表面，则完全计算 SDF；否则进行插值。
// 对 Suzanne 的测试表明，它比基本的朴素方法快 2 到 3 倍，且质量只有轻微下降。
// 不过它仍然相当慢。
void generate_mesh_sdf_approx_interp(
		Span<float> sdf_grid,
		const Vector3i res,
		Span<const Triangle> triangles,
		const Vector3f min_pos,
		const Vector3f max_pos
);

Vector3i auto_compute_grid_resolution(const Vector3f box_size, int cell_count);

struct CheckResult {
	bool ok;
	struct BadCell {
		Vector3i grid_pos;
		Vector3f mesh_pos;
		unsigned int closest_triangle_index;
	};
	BadCell cell0;
	BadCell cell1;
};

// 检查 SDF 的变化是否合法。相邻两个格子之间的差值不能高于
// 这两个格子之间的距离。这适用于真正的 SDF，而非近似值或缩放后的值。
CheckResult check_sdf(
		Span<const float> sdf_grid,
		Vector3i res,
		Span<const Triangle> triangles,
		Vector3f min_pos,
		Vector3f max_pos
);

// 当前方法提供的符号不完美。由于存在歧义，有时成片的格子会得到错误的符号。
// 此函数尝试纠正这些错误。
// 假定盒子边缘的符号为正，并使用洪泛填充。
// 如果从已有的粗略 SDF 出发，可以进行一种洪泛填充，把意外的符号变化视为可填充的，
// 而预期的符号变化则会正确地阻止填充。
// 不过，这种变通办法无法修复体积内部的符号。
// 我曾考虑将其与完全无符号的距离场配合使用，但我不确定是否能够
// 准确判断符号应该在何时翻转（即何时穿过表面）。
void fix_sdf_sign_from_boundary(Span<float> sdf_grid, Vector3i res, Vector3f min_pos, Vector3f max_pos);

// 生成一个近似结果。
// 计算一层准确的 SDF 值的薄壳，然后用 26 方向的洪泛填充进行传播。
void generate_mesh_sdf_approx_floodfill(
		Span<float> sdf_grid,
		const Vector3i res,
		Span<const Triangle> triangles,
		const ChunkGrid &chunk_grid,
		const Vector3f min_pos,
		const Vector3f max_pos,
		bool boundary_sign_fix
);

} // namespace voxel::mesh_sdf

#endif // VOXEL_MESH_SDF_H
