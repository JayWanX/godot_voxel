#ifndef VOXEL_STREAMING_DEPENDENCY_H
#define VOXEL_STREAMING_DEPENDENCY_H

#include "../generators/voxel_generator.h"
#include "../streams/voxel_stream.h"

namespace voxel {

// 某些异步任务所需的共享依赖。
// 可通过 shared_ptr 传递。
// 内部的指针不应改变。如果发生变化，将创建新实例并将旧实例标记为无效，
// 而不是冒读坏指针的风险，或不得不使用（大量）互斥锁。
struct StreamingDependency {
	Ref<VoxelStream> stream;
	Ref<VoxelGenerator> generator;
	bool valid = true;

	static void reset(
			std::shared_ptr<StreamingDependency> &ref,
			Ref<VoxelStream> stream,
			Ref<VoxelGenerator> generator
	) {
		if (ref != nullptr) {
			ref->valid = false;
		}
		ref = make_shared_instance<StreamingDependency>();
		ref->stream = stream;
		ref->generator = generator;
		ref->valid = true;
	}
};

} // namespace voxel

#endif // VOXEL_STREAMING_DEPENDENCY_H
