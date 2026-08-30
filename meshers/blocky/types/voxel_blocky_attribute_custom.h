#ifndef VOXEL_BLOCKY_ATTRIBUTE_CUSTOM_H
#define VOXEL_BLOCKY_ATTRIBUTE_CUSTOM_H

#include "voxel_blocky_attribute.h"

namespace voxel {

class VoxelBlockyAttributeCustom : public VoxelBlockyAttribute {
	GDCLASS(VoxelBlockyAttributeCustom, VoxelBlockyAttribute)
public:
	VoxelBlockyAttributeCustom();

	// 属性名称
	void set_attribute_name(StringName p_name);
	// 属性可取值的数量
	void set_value_count(int count);
	// 设置指定取值的名称
	void set_value_name(int index, StringName p_name);
	// 默认取值
	void set_default_value(int v);

	// 不那样暴露旋转，因为目前我们还无法自动化处理（以轨道为例，
	// 存在旋转，但它是不均匀的：多个值具有相同的旋转却对应不同模型）。用户
	// 将不得不使用编辑器工具手动旋转模型，这能提供更多控制。
	// 更简单的做法是将旋转与轨道形状分离为两个属性，但会为直线形状
	// 浪费一些模型 ID。
	//
	// void set_is_rotation(bool is_rotation);
	// void set_value_ortho_rotation(int index, int ortho_rotation_index);

private:
	void update_values();

	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	static void _bind_methods();
};

} // namespace voxel

#endif // VOXEL_BLOCKY_ATTRIBUTE_CUSTOM_H
