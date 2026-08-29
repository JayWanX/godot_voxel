#ifndef VOXEL_VOXEL_GENERATE_INSTANCES_BLOCK_TASK_H
#define VOXEL_VOXEL_GENERATE_INSTANCES_BLOCK_TASK_H

#include "../../generators/voxel_generator.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/core/array.h"
#include "../../util/tasks/threaded_task.h"
#include "instancer_task_output_queue.h"
#include "up_mode.h"
#include "voxel_instance_generator.h"

#include <cstdint>
#include <memory>

namespace voxel {

// TODO 优化：最终这应移到更靠近网格化任务的位置，包括已编辑的实例
class GenerateInstancesBlockTask : public IThreadedTask {
public:
	Vector3i mesh_block_grid_position;
	uint16_t layer_id;
	uint8_t lod_index;
	uint8_t edited_mask;
	UpMode up_mode;
	float mesh_block_size;
	Array surface_arrays;
	int32_t vertex_range_end = -1;
	int32_t index_range_end = -1;
	Ref<VoxelInstanceGenerator> generator;
	Ref<VoxelGenerator> voxel_generator;
	// 可由已编辑的变换预先填充
	StdVector<Transform3f> transforms;
	std::shared_ptr<InstancerTaskOutputQueue> output_queue;

	const char *get_debug_name() const override {
		return "GenerateInstancesBlock";
	}

	void run(ThreadedTaskContext &ctx) override;
};

} // namespace voxel

#endif // VOXEL_VOXEL_GENERATE_INSTANCES_BLOCK_TASK_H
