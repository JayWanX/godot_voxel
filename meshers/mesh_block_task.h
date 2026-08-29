#ifndef VOXEL_MESH_BLOCK_TASK_H
#define VOXEL_MESH_BLOCK_TASK_H

#include "../constants/voxel_constants.h"
#include "../engine/ids.h"
#include "../engine/meshing_dependency.h"
#include "../engine/priority_dependency.h"
#include "../storage/voxel_buffer.h"
#include "../util/containers/std_vector.h"
#include "../util/godot/classes/array_mesh.h"
#include "../util/tasks/cancellation_token.h"
#include "../util/tasks/threaded_task.h"

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
#include "../engine/detail_rendering/detail_rendering.h"
#endif

#ifdef VOXEL_ENABLE_GPU
#include "../generators/generate_block_gpu_task.h"
#endif

namespace voxel {

class VoxelData;

// 在特定体积内，从体素数据块及其邻居生成网格的异步任务
class MeshBlockTask
#ifdef VOXEL_ENABLE_GPU
		: public IGeneratingVoxelsThreadedTask
#else
		: public IThreadedTask
#endif
{
public:
	MeshBlockTask();
	~MeshBlockTask();

	const char *get_debug_name() const override {
		return "MeshBlock";
	}

	void run(ThreadedTaskContext &ctx) override;
	TaskPriority get_priority() override;
	bool is_cancelled() override;
	void apply_result() override;

#ifdef VOXEL_ENABLE_GPU
	void set_gpu_results(StdVector<GenerateBlockGPUTaskResult> &&results) override;
#endif

	static int debug_get_running_count();

	// 3x3x3 或 4x4x4 的体素数据块网格。
	FixedArray<std::shared_ptr<VoxelBuffer>, constants::MAX_BLOCK_COUNT_PER_REQUEST> blocks;
	// TODO 需要提供格式
	// FixedArray<uint8_t, VoxelBuffer::MAX_CHANNELS> channel_depths;
	Vector3i mesh_block_position; // 以指定 LOD 的网格数据块为单位
	VolumeID volume_id;
	uint8_t lod_index = 0;
	uint8_t blocks_count = 0;
	// 如果为 true，则在可能的情况下创建渲染网格资源。
	bool require_visual = true;
	// 如果为 true，则需要碰撞网格（如果可能）
	bool collision_hint = false;
	// 如果为 true，网格将用于带 LOD 的上下文，这可能会在其构建方式上要求一些额外的处理
	bool lod_hint = false;
	// 细节纹理可能已启用，但我们并不想在每次网格更新时都更新它们。
	// 因此也会检查此布尔值以决定是否需要计算它们。
	bool require_detail_texture = false;
	uint8_t detail_texture_generator_override_begin_lod_index = 0;
	bool detail_texture_use_gpu = false;
	bool block_generation_use_gpu = false;
	PriorityDependency priority_dependency;
	std::shared_ptr<MeshingDependency> meshing_dependency;
	std::shared_ptr<VoxelData> data;
#ifdef VOXEL_ENABLE_SMOOTH_MESHING
	DetailRenderingSettings detail_texture_settings;
#endif
	Ref<VoxelGenerator> detail_texture_generator_override;
	TaskCancellationToken cancellation_token;

private:
#ifdef VOXEL_ENABLE_GPU
	void gather_voxels_gpu(voxel::ThreadedTaskContext &ctx);
#endif
	void gather_voxels_cpu();
	void build_mesh();

	bool _has_run = false;
	bool _too_far = false;
	bool _has_mesh_resource = false;
#ifdef VOXEL_ENABLE_GPU
	uint8_t _stage = 0;
#endif
	VoxelBuffer _voxels;
	VoxelMesher::Output _surfaces_output;
	Ref<Mesh> _mesh;
	Ref<Mesh> _shadow_occluder_mesh;
	StdVector<uint16_t> _mesh_material_indices; // 按网格表面索引
#ifdef VOXEL_ENABLE_SMOOTH_MESHING
	std::shared_ptr<DetailTextureOutput> _detail_textures;
#endif
#ifdef VOXEL_ENABLE_GPU
	StdVector<GenerateBlockGPUTaskResult> _gpu_generation_results;
#endif
};

// 根据多组表面数据构建网格资源，并返回输入中指定的材质在返回网格中的映射。
// 空表面不会被加入网格。如果网格完全为空，将返回 null。
Ref<ArrayMesh> build_mesh( //
		Span<const VoxelMesher::Output::Surface> surfaces, //
		Mesh::PrimitiveType primitive, //
		int flags, //
		StdVector<uint16_t> &mesh_material_indices //
);

// 根据单一表面构建三角形网格资源。如果表面为空，则返回 null。
Ref<ArrayMesh> build_mesh(Array surface);

} // namespace voxel

#endif // VOXEL_MESH_BLOCK_TASK_H
