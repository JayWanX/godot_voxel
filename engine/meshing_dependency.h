#ifndef VOXEL_MESHING_DEPENDENCY_H
#define VOXEL_MESHING_DEPENDENCY_H

#include "../generators/voxel_generator.h"
#include "../meshers/voxel_mesher.h"
#include "../util/memory/memory.h"

namespace voxel {

// 某些异步任务所需的共享依赖。
// 可通过 shared_ptr 传递。
// 内部的指针不应改变。如果发生变化，将创建新实例并将旧实例标记为无效，
// 而不是冒读坏指针的风险，或不得不使用（大量）互斥锁。
struct MeshingDependency {
	Ref<VoxelMesher> mesher;
	Ref<VoxelGenerator> generator;
	bool valid = true;

	static void reset(std::shared_ptr<MeshingDependency> &ref, Ref<VoxelMesher> mesher, Ref<VoxelGenerator> generator) {
		if (ref != nullptr) {
			ref->valid = false;
		}
		ref = make_shared_instance<MeshingDependency>();
		ref->mesher = mesher;
		ref->generator = generator;
		ref->valid = true;
	}
};

} // namespace voxel

#endif // VOXEL_MESHING_DEPENDENCY_H
