#ifndef VOXEL_BLOCKY_ATTRIBUTE_ROTATION_H
#define VOXEL_BLOCKY_ATTRIBUTE_ROTATION_H

#include "voxel_blocky_attribute.h"

namespace voxel {

// `rotation`
class VoxelBlockyAttributeRotation : public VoxelBlockyAttribute {
	GDCLASS(VoxelBlockyAttributeRotation, VoxelBlockyAttribute)
public:
	VoxelBlockyAttributeRotation();

	// 是否启用水平滚动旋转
	void set_horizontal_roll_enabled(bool enable);
	bool is_horizontal_roll_enabled() const;

private:
	void update_values();

	static void _bind_methods();

	bool _horizontal_roll_enabled = false;
};

} // namespace voxel

#endif // VOXEL_BLOCKY_ATTRIBUTE_ROTATION_H
