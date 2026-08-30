#ifndef VOXEL_BLOCKY_ATTRIBUTE_DIRECTION_H
#define VOXEL_BLOCKY_ATTRIBUTE_DIRECTION_H

#include "voxel_blocky_attribute.h"

namespace voxel {

// `direction`
class VoxelBlockyAttributeDirection : public VoxelBlockyAttribute {
	GDCLASS(VoxelBlockyAttributeDirection, VoxelBlockyAttribute)
public:
	enum Direction { //
		DIR_NEGATIVE_X,
		DIR_POSITIVE_X,
		DIR_NEGATIVE_Y,
		DIR_POSITIVE_Y,
		DIR_NEGATIVE_Z,
		DIR_POSITIVE_Z,
		DIR_COUNT
	};

	VoxelBlockyAttributeDirection();

	// 是否仅限水平方向
	void set_horizontal_only(bool h);
	bool is_horizontal_only() const;

	// 从方向向量获取对应的方向取值
	int from_vec3(Vector3 v) const;

private:
	void update_values();

	static void _bind_methods();

	bool _horizontal_only = false;
	// TODO 对应的正交旋转
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelBlockyAttributeDirection::Direction);

#endif // VOXEL_BLOCKY_ATTRIBUTE_DIRECTION_H
