#ifndef VOXEL_BLOCKY_MODEL_EMPTY_H
#define VOXEL_BLOCKY_MODEL_EMPTY_H

#include "voxel_blocky_model.h"

namespace voxel {

// 默认没有视觉效果也没有碰撞的模型。
class VoxelBlockyModelEmpty : public VoxelBlockyModel {
	GDCLASS(VoxelBlockyModelEmpty, VoxelBlockyModel)
public:
	VoxelBlockyModelEmpty();

	void bake(blocky::ModelBakingContext &ctx) const override;

	Ref<Mesh> get_preview_mesh() const override;
	bool is_empty() const override;

private:
	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_BLOCKY_MODEL_EMPTY_H
