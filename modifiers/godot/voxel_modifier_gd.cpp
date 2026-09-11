#include "voxel_modifier_gd.h"
#include <core/variant/array.h>
#include "../../util/io/log.h"
#include "../../util/math/conv.h"
#include "../voxel_modifier_sdf.h"
#include <core/variant/array.h>

#ifdef TOOLS_ENABLED
#include "../../util/godot/core/packed_arrays.h"
#include "../../util/godot/core/string.h"
#endif

namespace voxel::godot {

VoxelModifier::VoxelModifier() {
	set_notify_local_transform(true);
}

voxel::VoxelModifier *VoxelModifier::create(voxel::VoxelModifierStack &modifiers, uint32_t id) {
	VOXEL_PRINT_ERROR("Not implemented");
	return nullptr;
}

voxel::VoxelModifierSdf::Operation to_op(VoxelModifier::Operation op) {
	return voxel::VoxelModifierSdf::Operation(op);
}

void post_edit_modifier(VoxelLodTerrain &volume, AABB aabb) {
	volume.post_edit_modifiers(Box3i(math::floor_to_int(aabb.position), math::floor_to_int(aabb.size)));
}

void VoxelModifier::set_operation(Operation op) {
	VOXEL_ASSERT_RETURN(op >= 0 && op < OPERATION_COUNT);
	if (op == _operation) {
		return;
	}
	_operation = op;
	if (_volume == nullptr) {
		return;
	}
	VoxelData &voxel_data = _volume->get_storage();
	VoxelModifierStack &modifiers = voxel_data.get_modifiers();
	voxel::VoxelModifier *modifier = modifiers.get_modifier(_modifier_id);
	VOXEL_ASSERT_RETURN(modifier != nullptr);
	VOXEL_ASSERT_RETURN(modifier->is_sdf());
	voxel::VoxelModifierSdf *sdf_modifier = static_cast<voxel::VoxelModifierSdf *>(modifier);
	sdf_modifier->set_operation(to_op(_operation));
	post_edit_modifier(*_volume, modifier->get_aabb());
}

VoxelModifier::Operation VoxelModifier::get_operation() const {
	return _operation;
}

void VoxelModifier::set_smoothness(float s) {
	if (s == _smoothness) {
		return;
	}
	_smoothness = math::max(s, 0.f);
	if (_volume == nullptr) {
		return;
	}
	VoxelData &voxel_data = _volume->get_storage();
	VoxelModifierStack &modifiers = voxel_data.get_modifiers();
	voxel::VoxelModifier *modifier = modifiers.get_modifier(_modifier_id);
	VOXEL_ASSERT_RETURN(modifier != nullptr);
	VOXEL_ASSERT_RETURN(modifier->is_sdf());
	voxel::VoxelModifierSdf *sdf_modifier = static_cast<voxel::VoxelModifierSdf *>(modifier);
	const AABB prev_aabb = modifier->get_aabb();
	sdf_modifier->set_smoothness(_smoothness);
	const AABB new_aabb = modifier->get_aabb();
	post_edit_modifier(*_volume, prev_aabb);
	post_edit_modifier(*_volume, new_aabb);
}

float VoxelModifier::get_smoothness() const {
	return _smoothness;
}

void VoxelModifier::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node *parent = get_parent();
			VOXEL_ASSERT_RETURN(parent != nullptr);
			VOXEL_ASSERT_RETURN(_volume == nullptr);
			VoxelLodTerrain *volume = Object::cast_to<VoxelLodTerrain>(parent);
			_volume = volume;

			if (_volume != nullptr) {
				VoxelData &voxel_data = _volume->get_storage();
				VoxelModifierStack &modifiers = voxel_data.get_modifiers();
				const uint32_t id = modifiers.allocate_id();
				voxel::VoxelModifier *modifier = create(modifiers, id);

				if (modifier->is_sdf()) {
					voxel::VoxelModifierSdf *sdf_modifier =
							static_cast<voxel::VoxelModifierSdf *>(modifier);
					sdf_modifier->set_operation(to_op(_operation));
					sdf_modifier->set_smoothness(_smoothness);
				}

				modifier->set_transform(get_transform());
				_modifier_id = id;
				// TODO 优化：加载场景时，这可能对性能非常不利，因为地图上可能
				// 有大量修改器，但目前 Godot 中无法区分……
				post_edit_modifier(*_volume, modifier->get_aabb());
			}

			update_configuration_warnings();
		} break;

		case NOTIFICATION_UNPARENTED: {
			if (_volume != nullptr) {
				VoxelData &voxel_data = _volume->get_storage();
				VoxelModifierStack &modifiers = voxel_data.get_modifiers();
				voxel::VoxelModifier *modifier = modifiers.get_modifier(_modifier_id);
				VOXEL_ASSERT_RETURN_MSG(modifier != nullptr, "The modifier node wasn't linked properly");
				post_edit_modifier(*_volume, modifier->get_aabb());
				modifiers.remove_modifier(_modifier_id);
				_volume = nullptr;
				_modifier_id = 0;
			}
		} break;

		case Node3D::NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (_volume != nullptr && is_inside_tree()) {
				VoxelData &voxel_data = _volume->get_storage();
				VoxelModifierStack &modifiers = voxel_data.get_modifiers();
				voxel::VoxelModifier *modifier = modifiers.get_modifier(_modifier_id);
				VOXEL_ASSERT_RETURN(modifier != nullptr);

				const AABB prev_aabb = modifier->get_aabb();
				modifier->set_transform(get_transform());
				const AABB aabb = modifier->get_aabb();
				post_edit_modifier(*_volume, prev_aabb);
				post_edit_modifier(*_volume, aabb);

				// TODO 妥善处理嵌套，虽然这很麻烦
				// 当地形移动时，修改器的局部变换在技术上也会改变。
				// 但它相对于地形并未改变。然而由于我们无法检查这一点，
				// 所有修改器都会同时触发更新……
			}
		} break;
	}
}

#ifdef TOOLS_ENABLED

PackedStringArray VoxelModifier::get_configuration_warnings() const {
	PackedStringArray warnings;
	get_configuration_warnings(warnings);
	return warnings;
}

void VoxelModifier::get_configuration_warnings(PackedStringArray &warnings) const {
	if (_volume == nullptr) {
		warnings.append(VOXEL_TTR("The parent of this node must be of type {0}.")
								.format(varray(VoxelLodTerrain::get_class_static())));
	}
}

#endif

void VoxelModifier::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_operation", "op"), &VoxelModifier::set_operation);
	ClassDB::bind_method(D_METHOD("get_operation"), &VoxelModifier::get_operation);

	ClassDB::bind_method(D_METHOD("set_smoothness", "smoothness"), &VoxelModifier::set_smoothness);
	ClassDB::bind_method(D_METHOD("get_smoothness"), &VoxelModifier::get_smoothness);

	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "operation", PROPERTY_HINT_ENUM, "Add,Remove"), "set_operation", "get_operation"
	);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "smoothness", PROPERTY_HINT_RANGE, "0.0, 100.0, 0.1"),
			"set_smoothness",
			"get_smoothness"
	);

	BIND_ENUM_CONSTANT(OPERATION_ADD);
	BIND_ENUM_CONSTANT(OPERATION_REMOVE);
}

} // namespace voxel::godot
