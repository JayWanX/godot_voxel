#ifndef VOXEL_MODIFIER_MESH_H
#define VOXEL_MODIFIER_MESH_H

#include "../edition/voxel_mesh_sdf_gd.h"
#include "voxel_modifier_sdf.h"

namespace voxel {

class VoxelModifierMesh : public VoxelModifierSdf {
public:
	Type get_type() const override {
		return TYPE_MESH;
	};

	void set_mesh_sdf(Ref<VoxelMeshSDF> mesh_sdf);
	void set_isolevel(float isolevel);
	void apply(VoxelModifierContext ctx) const override;

#ifdef VOXEL_ENABLE_GPU
	void get_shader_data(ShaderData &out_shader_data) override;
	void request_shader_data_update();
#endif

protected:
	void update_aabb() override;

private:
	// 最初我想让修改器的核心与 Godot 相关的内容保持分离，但为了支持
	// GPU 资源，把它放在这里更简单。
	Ref<VoxelMeshSDF> _mesh_sdf;
	float _isolevel;
};

} // namespace voxel

#endif // VOXEL_MODIFIER_MESH_H
