#ifndef VOXEL_BLOCKY_ATTRIBUTE_AXIS_H
#define VOXEL_BLOCKY_ATTRIBUTE_AXIS_H

#include "voxel_blocky_attribute.h"

namespace voxel {

// `axis`
class VoxelBlockyAttributeAxis : public VoxelBlockyAttribute {
	GDCLASS(VoxelBlockyAttributeAxis, VoxelBlockyAttribute)
public:
	enum Axis { AXIS_X, AXIS_Y, AXIS_Z, AXIS_COUNT };

	VoxelBlockyAttributeAxis();

	// 是否仅限水平方向的轴
	void set_horizontal_only(bool h);
	bool is_horizontal_only() const;

	// 从方向向量获取对应的轴取值
	int from_vec3(Vector3 v) const;

private:
	void update_values();

	static void _bind_methods();

	bool _horizontal_only = false;
	// TODO 对应的正交旋转
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelBlockyAttributeAxis::Axis);

#endif // VOXEL_BLOCKY_ATTRIBUTE_AXIS_H
