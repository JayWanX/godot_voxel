#ifndef VOXEL_INSTANCER_TASK_OUTPUT_QUEUE_H
#define VOXEL_INSTANCER_TASK_OUTPUT_QUEUE_H

#include "../../util/containers/std_vector.h"
#include "../../util/math/transform3f.h"
#include "../../util/math/vector3i.h"
#include "../../util/thread/mutex.h"
#include <cstdint>

namespace voxel {

struct InstanceLoadingTaskOutput {
	Vector3i render_block_position;
	uint8_t layer_id;
	// 表示数据块中哪些部分包含已编辑的数据（非生成数据）。
	// 当数据块大小是渲染数据块的一半时，按 XYZ 顺序占 8 位。
	uint8_t edited_mask;
	StdVector<Transform3f> transforms;
};

struct InstancerTaskOutputQueue {
	StdVector<InstanceLoadingTaskOutput> results;
	Mutex mutex;
};

} // namespace voxel

#endif // VOXEL_INSTANCER_TASK_OUTPUT_QUEUE_H
