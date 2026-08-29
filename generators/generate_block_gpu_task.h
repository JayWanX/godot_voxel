#ifndef VOXEL_GENERATE_BLOCK_GPU_TASK_H
#define VOXEL_GENERATE_BLOCK_GPU_TASK_H

#include "../engine/gpu/gpu_storage_buffer_pool.h"
#include "../engine/gpu/gpu_task_runner.h"
#include "../generators/voxel_generator.h"
#include "../util/containers/std_vector.h"
#include "../util/godot/classes/rd_uniform.h"
#include "../util/math/box3i.h"
#include "../util/memory/memory.h"
#include "../util/tasks/threaded_task.h"

#ifdef VOXEL_ENABLE_MODIFIERS
#include "../modifiers/voxel_modifier.h"
#endif

namespace voxel {

class GenerateBlockGPUTaskResult {
public:
	GenerateBlockGPUTaskResult(
			Box3i p_box,
			VoxelGenerator::ShaderOutput::Type p_type,
			Span<const uint8_t> p_bytes,
			PackedByteArray p_shared_bytes
	) :
			_box(p_box), _type(p_type), _bytes(p_bytes), _shared_bytes(p_shared_bytes) {}

	static void convert_to_voxel_buffer(Span<GenerateBlockGPUTaskResult> boxes_data, VoxelBuffer &dst);

private:
	void convert_to_voxel_buffer(VoxelBuffer &dst);

	Box3i _box;
	VoxelGenerator::ShaderOutput::Type _type;
	// 此数据块对应的共享输出缓冲区的 span。
	Span<const uint8_t> _bytes;
	// 这是直接从 GPU 下载的缓冲区。它在多个消费者之间共享，须在它们全部用完之前一直保留在内存中，
	// 因此在每个结果中都持有它的引用。它是只读的以避免写时复制（CoW）。这样做是为了避免分配单独的缓冲区来传递。
	PackedByteArray _shared_bytes;
};

// 用于可派生 `GenerateBlockGPUTask` 的任务的接口。它必须返回其结果。
class IGeneratingVoxelsThreadedTask : public IThreadedTask {
public:
	// GPU 任务完成时调用。
	virtual void set_gpu_results(StdVector<GenerateBlockGPUTaskResult> &&results) = 0;
};

// 在 GPU 上生成一个体素块。必须从某个线程任务中调度，当本任务完成后该线程任务会被恢复。
class GenerateBlockGPUTask : public IGPUTask {
public:
	~GenerateBlockGPUTask();

	unsigned int get_required_shared_output_buffer_size() const override;

	void prepare(GPUTaskContext &ctx) override;
	void collect(GPUTaskContext &ctx) override;

	// TODO 不确定是否有必要处理子数据块。只有在部分编辑过的网格生成数据块时才会用到……
	// 这种情况似乎并不常见。

	// 相对于 VoxelBuffer 的数据块（不是世界体素坐标）。它们不能相交。
	StdVector<Box3i> boxes_to_generate;
	// VoxelBuffer 左下角在世界体素坐标中的位置
	Vector3i origin_in_voxels;
	uint8_t lod_index = 0;
	// 该体素数据所服务的任务。
	IGeneratingVoxelsThreadedTask *consumer_task = nullptr;

	// 基础生成器
	std::shared_ptr<ComputeShader> generator_shader;
	std::shared_ptr<ComputeShaderParameters> generator_shader_params;
	std::shared_ptr<VoxelGenerator::ShaderOutputs> generator_shader_outputs;

#ifdef VOXEL_ENABLE_MODIFIERS
	StdVector<VoxelModifier::ShaderData> modifiers;
#endif

private:
	struct BoxData {
		GPUStorageBuffer params_sb;
		Ref<RDUniform> params_uniform;
		Ref<RDUniform> output_uniform;
	};

	StdVector<BoxData> _boxes_data;
	RID _generator_pipeline_rid;
	StdVector<RID> _modifier_pipelines;
	StdVector<RID> _uniform_sets_to_free;
};

} // namespace voxel

#endif // VOXEL_GENERATE_BLOCK_GPU_TASK_H
