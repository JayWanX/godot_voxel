#include "shader_material_pool_vlt.h"
#include "../../constants/voxel_string_names.h"
#include <scene/resources/texture.h>
#include "../../util/profiling.h"

namespace voxel {

void ShaderMaterialPoolVLT::recycle(Ref<ShaderMaterial> material) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(material.is_valid());

	const VoxelStringNames &sn = VoxelStringNames::get_singleton();

	// 重置纹理，避免它们在池中被囤积
	material->set_shader_parameter(sn.u_voxel_normalmap_atlas, Ref<Texture2D>());
	material->set_shader_parameter(sn.u_voxel_cell_lookup, Ref<Texture2D>());
	material->set_shader_parameter(sn.u_voxel_virtual_texture_offset_scale, Vector4(0, 0, 0, 1));
	// TODO 若能重新利用 `u_transition_mask` 存储额外标志就好了。
	// 这里我们利用 cell_size==0 表示“该数据块上没有虚拟法线贴图”
	material->set_shader_parameter(sn.u_voxel_cell_size, 0.f);
	material->set_shader_parameter(sn.u_voxel_virtual_texture_fade, 0.f);

	material->set_shader_parameter(sn.u_transition_mask, 0);
	material->set_shader_parameter(sn.u_lod_fade, Vector2(0.0, 0.0));

	voxel::godot::ShaderMaterialPool::recycle(material);
}

} // namespace voxel
