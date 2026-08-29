#include "priority_dependency.h"
#include "../constants/voxel_constants.h"
#include "../util/math/funcs.h"

namespace voxel {

TaskPriority PriorityDependency::evaluate(uint8_t lod_index, uint8_t band2_priority, float *out_closest_distance_sq) {
	TaskPriority priority;
	VOXEL_ASSERT_RETURN_V(shared != nullptr, priority);

	const StdVector<Vector3f> &viewer_positions = shared->viewers;
	const unsigned int viewer_count = shared->viewers_count;

	const Vector3f block_position = world_position;

	float closest_distance_sq = 99999.f;
	if (viewer_positions.size() == 0) {
		// 假设为原点
		closest_distance_sq = math::length_squared(block_position);
	} else {
		for (unsigned int i = 0; i < viewer_count; ++i) {
			const float d = math::distance_squared(viewer_positions[i], block_position);
			if (d < closest_distance_sq) {
				closest_distance_sq = d;
			}
		}
	}

	if (out_closest_distance_sq != nullptr) {
		*out_closest_distance_sq = closest_distance_sq;
	}

	// TODO 有什么办法可以省去开方运算吗？或许可以用快速的整数版本？
	// 我加上它是因为 LOD 修正在使用平方距离时不起作用，
	// 这会导致数据块相比相邻数据块过度细分，从而更容易出现裂缝
	const int distance = static_cast<int>(Math::sqrt(closest_distance_sq));

	// TODO 优先处理 LOD 会拖慢生成速度……但不优先处理又更容易出现裂缝……
	// 这个问题是否可以通过允许体预先请求下一级 LOD 的数据块来解决？
	//
	// 更高的 lod 索引排在前面，以便八叉树能够细分。
	// 接着是距离，它会根据数据块在视野中的程度进行调整
	// priority += (constants::MAX_LOD - lod_index) * 10000;

	// 越近优先级越高。随距离增加而降低。
	// 按 LOD 缩放，因为在 band 1 中也按 LOD 划分优先级。
	priority.band0 = math::max(TaskPriority::BAND_MAX - math::arithmetic_rshift(distance, 4 + lod_index), 0);
	// 注意：过去让更低的 LOD 索引（即更精细的）拥有更高优先级，似乎会更容易导致网格之间
	// 出现裂缝，所以有一段时间我把它反了过来。但那种优先级更合理，所以我改回来了。
	// 之后再观察这是否真的会引发问题。
	priority.band1 = constants::MAX_LOD - lod_index;
	priority.band2 = band2_priority;
	priority.band3 = constants::TASK_PRIORITY_BAND3_DEFAULT;

	return priority;
}

} // namespace voxel
