#ifndef VOXEL_SHADER_MATERIAL_POOL_VLT_H
#define VOXEL_SHADER_MATERIAL_POOL_VLT_H

#include "../../util/godot/shader_material_pool.h"

namespace voxel {

class ShaderMaterialPoolVLT : public voxel::godot::ShaderMaterialPool {
public:
	void recycle(Ref<ShaderMaterial> material);
};

} // namespace voxel

#endif // VOXEL_SHADER_MATERIAL_POOL_VLT_H
