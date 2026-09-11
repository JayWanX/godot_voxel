#include "voxel_blocky_attribute_rotation.h"
#include "../../../constants/voxel_string_names.h"
#include <core/object/class_db.h>


namespace voxel {

// 完整的正交旋转实际上很难直接作为单个属性使用……所有 24 个值以及原始整数值
// 都无法有直接明了的唯一名称。它可能总是需要一个中间工具或辅助
// 函数来获取/设置旋转，通常从现有状态开始并围绕一个或两个轴旋转。

VoxelBlockyAttributeRotation::VoxelBlockyAttributeRotation() {
	_name = VoxelStringNames::get_singleton().rotation;
	_is_rotation = true;

	_value_names.resize(math::ORTHOGONAL_BASIS_COUNT);
	for (unsigned int i = 0; i < _value_names.size(); ++i) {
		_value_names[i] = VoxelStringNames::get_singleton().ortho_rotation_names[i];
	}

	update_values();

	_ortho_rotations.resize(math::ORTHOGONAL_BASIS_COUNT);
	for (unsigned int i = 0; i < _ortho_rotations.size(); ++i) {
		_ortho_rotations[i] = i;
	}
}

void VoxelBlockyAttributeRotation::set_horizontal_roll_enabled(bool enable) {
	if (enable != _horizontal_roll_enabled) {
		_horizontal_roll_enabled = enable;
		update_values();
		emit_changed();
	}
}

bool VoxelBlockyAttributeRotation::is_horizontal_roll_enabled() const {
	return _horizontal_roll_enabled;
}

void VoxelBlockyAttributeRotation::update_values() {
	_used_values.clear();
	_used_values.reserve(math::ORTHOGONAL_BASIS_COUNT);
	if (_horizontal_roll_enabled) {
		for (unsigned int ortho_index = 0; ortho_index < math::ORTHOGONAL_BASIS_COUNT; ++ortho_index) {
			_used_values.push_back(ortho_index);
		}
	} else {
		for (unsigned int ortho_index = 0; ortho_index < math::ORTHOGONAL_BASIS_COUNT; ++ortho_index) {
			const math::OrthoBasis &basis = math::get_ortho_basis_from_index(ortho_index);
			if (basis.y != Vector3i(0, 1, 0) && basis.z.y == 0) {
				// 跳过 Y 不朝上且 Z 不水平的旋转
				continue;
			}
			_used_values.push_back(ortho_index);
		}
	}
}

void VoxelBlockyAttributeRotation::_bind_methods() {
	ClassDB::bind_method(
			D_METHOD("is_horizontal_roll_enabled"), &VoxelBlockyAttributeRotation::is_horizontal_roll_enabled
	);
	ClassDB::bind_method(
			D_METHOD("set_horizontal_roll_enabled", "enabled"),
			&VoxelBlockyAttributeRotation::set_horizontal_roll_enabled
	);

	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "horizontal_only"), "set_horizontal_roll_enabled", "is_horizontal_roll_enabled"
	);
}

} // namespace voxel
