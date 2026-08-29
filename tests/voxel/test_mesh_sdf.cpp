#include "test_mesh_sdf.h"
#include "../../edition/voxel_mesh_sdf_gd.h"

namespace voxel::tests {

void test_voxel_mesh_sdf_issue463() {
	Ref<VoxelMeshSDF> msdf;
	msdf.instantiate();

	Dictionary d;
	d["roman"] = 22;
	d[22] = 25;
	// TODO 原始报告是创建 BoxShape3D，但出于我无法理解的原因，在 `MODULE_INITIALIZATION_LEVEL_SERVERS` 之后
	// Godot 的 PhysicsServer3D 仍未创建。甚至 SCENE 和 EDITOR
	// 级别也不行。用任何级别都无法对物理做任何操作……自己去琢磨吧。
	//
	// Ref<BoxShape3D> shape1;
	// shape1.instantiate();
	// Ref<BoxShape3D> shape2;
	// shape2.instantiate();
	// d[shape1] = shape2;
	Ref<Resource> res1;
	res1.instantiate();
	Ref<Resource> res2;
	res2.instantiate();
	d[res1] = res2;

	VOXEL_ASSERT(msdf->has_method("_set_data"));
	// 设置无效数据应导致报错，但不崩溃也不泄漏
	msdf->call("_set_data", d);
}

} // namespace voxel::tests
