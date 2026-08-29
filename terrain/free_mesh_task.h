#ifndef VOXEL_FREE_MESH_TASK_H
#define VOXEL_FREE_MESH_TASK_H

#include "../engine/voxel_engine.h"
#include "../util/godot/direct_mesh_instance.h"
#include "../util/profiling.h"
#include "../util/tasks/progressive_task_runner.h"

namespace voxel {

// 在 Godot4 中不得不这样做，因为删除网格尤其昂贵，
// 这归因于渲染器使用的 Vulkan 分配器。
// 这是一个延迟成本（它不是在 Mesh 对象被销毁的确切时刻消耗的，而是稍后发生），所以必须
// 使用不同类型的任务来进行负载均衡。该任务实际上只是将网格的引用多保留
// 一小段时间，假定该网格不再被使用。然后执行该任务会释放该引用。
class FreeMeshTask : public IProgressiveTask {
public:
	static inline void try_add_and_destroy(voxel::godot::DirectMeshInstance &mi) {
		const Mesh *mesh = mi.get_mesh_ptr();
		if (mesh != nullptr && mesh->get_reference_count() == 1) {
			// 该实例持有此网格的最后一个引用
			add(mi.get_mesh());
		}
		mi.destroy();
	}

	void run() override {
		VOXEL_PROFILE_SCOPE();
		if (_mesh->get_reference_count() > 1) {
			VOXEL_PRINT_WARNING("Mesh has more than one ref left, task spreading will not be effective at smoothing "
							 "destruction cost");
		}
		_mesh.unref();
	}

private:
	static void add(Ref<Mesh> mesh) {
		VOXEL_ASSERT(mesh.is_valid());
		FreeMeshTask *task = VOXEL_NEW(FreeMeshTask(mesh));
		VoxelEngine::get_singleton().push_main_thread_progressive_task(task);
	}

	FreeMeshTask(Ref<Mesh> p_mesh) : _mesh(p_mesh) {}

	Ref<Mesh> _mesh;
};

} // namespace voxel

#endif // VOXEL_FREE_MESH_TASK_H
