#ifndef VOXEL_RENDER_DETAIL_TEXTURE_TASK_H
#define VOXEL_RENDER_DETAIL_TEXTURE_TASK_H

#include "../../generators/voxel_generator.h"
#include "../../meshers/voxel_mesher.h"
#include "../../util/containers/std_vector.h"
#include "../../util/memory/memory.h"
#include "../../util/tasks/threaded_task.h"
#include "../ids.h"
#include "../priority_dependency.h"
#include "detail_rendering.h"

namespace voxel {

class RenderDetailTextureGPUTask;

// 为远处的体素网格渲染提供额外细节的纹理。
// 该任务与网格化任务分开，因为它耗时明显更长。它有单独的优先级，
// 因此大多数情况下我们可以先拿到网格，稍后再用结果进行修正。
class RenderDetailTextureTask : public IThreadedTask {
public:
	// 输入

	UniquePtr<ICellIterator> cell_iterator;
	// TODO 优化：也许能想办法不复制网格数据？唯一的原因是 Godot 需要
	// 稍微不同的数据结构，可能因为使用 `Vector3` 而产生不必要的重复占用……
	StdVector<Vector3f> mesh_vertices;
	StdVector<Vector3f> mesh_normals;
	StdVector<int> mesh_indices;
	Ref<VoxelGenerator> generator;
	std::shared_ptr<VoxelData> voxel_data;
	Vector3i mesh_block_size;
	uint8_t lod_index;
	bool use_gpu = false;
	DetailRenderingSettings detail_texture_settings;

	// 输出（待赋值，以便填充内容）
	std::shared_ptr<DetailTextureOutput> output_textures;

	// 标识
	Vector3i mesh_block_position;
	VolumeID volume_id;
	PriorityDependency priority_dependency;

	const char *get_debug_name() const override {
		return "RenderDetailTexture";
	}

	void run(ThreadedTaskContext &ctx) override;
	void apply_result() override;
	TaskPriority get_priority() override;
	bool is_cancelled() override;

	// 暴露给测试使用
	RenderDetailTextureGPUTask *make_gpu_task();

private:
	void run_on_cpu();
#ifdef VOXEL_ENABLE_GPU
	void run_on_gpu();
#endif
};

#ifdef VOXEL_ENABLE_GPU

// 在 GPU 工作完成后，在 CPU 上执行最终操作
class RenderDetailTexturePass2Task : public IThreadedTask {
public:
	PackedByteArray atlas_data;
	DetailTextureData edited_tiles_texture_data;
	StdVector<DetailTextureData::Tile> tile_data;
	std::shared_ptr<DetailTextureOutput> output_textures;
	VolumeID volume_id;
	Vector3i mesh_block_position;
	Vector3i mesh_block_size;
	uint16_t atlas_width;
	uint16_t atlas_height;
	uint8_t lod_index;
	uint8_t tile_size_pixels;

	const char *get_debug_name() const override {
		return "RenderDetailTexturePass2";
	}

	void run(ThreadedTaskContext &ctx) override;
	void apply_result() override;
};

#endif

} // namespace voxel

#endif // VOXEL_RENDER_DETAIL_TEXTURE_TASK_H
