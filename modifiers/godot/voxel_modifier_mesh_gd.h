#ifndef VOXEL_MODIFIER_MESH_GD_H
#define VOXEL_MODIFIER_MESH_GD_H

#include "../../edition/voxel_mesh_sdf_gd.h"
#include "voxel_modifier_gd.h"

namespace voxel::godot {

class VoxelModifierMesh : public VoxelModifier {
	GDCLASS(VoxelModifierMesh, VoxelModifier);

public:
	// 网格的 SDF 数据
	void set_mesh_sdf(Ref<VoxelMeshSDF> mesh_sdf);
	Ref<VoxelMeshSDF> get_mesh_sdf() const;

	// 等值面（isolevel）
	void set_isolevel(float isolevel);
	float get_isolevel() const;

#ifdef TOOLS_ENABLED
	// 获取编辑器中显示的操作警告
	void get_configuration_warnings(PackedStringArray &warnings) const override;
#endif

protected:
	voxel::VoxelModifier *create(voxel::VoxelModifierStack &modifiers, uint32_t id) override;

private:
	void _on_mesh_sdf_baked();

	static void _bind_methods();

	Ref<VoxelMeshSDF> _mesh_sdf;
	float _isolevel = 0.0f;
};

} // namespace voxel::godot

#endif // VOXEL_MODIFIER_MESH_GD_H