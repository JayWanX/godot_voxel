#ifndef VOXEL_BLOCKY_ATTRIBUTE_H
#define VOXEL_BLOCKY_ATTRIBUTE_H

#include "../../../util/containers/span.h"
#include "../../../util/containers/std_vector.h"
#include "../../../util/godot/classes/resource.h"

namespace voxel {

// 可附加到 VoxelBlockyType 上的属性。
// 它有唯一的名称，可以取两个或更多非负值。值可以命名。
class VoxelBlockyAttribute : public Resource {
	GDCLASS(VoxelBlockyAttribute, Resource)
public:
	static const int MAX_VALUES = 256;

	// 属性名称
	virtual StringName get_attribute_name() const;

	// TODO 实际上返回的是最大值 + 1。这并非该属性取值的总数！
	int get_value_count() const;

	// 默认取值
	int get_default_value() const;
	void set_default_value(int value);

	// TODO -1 表示"未找到"，但我不喜欢这样……
	int get_value_from_name(StringName p_name) const;

	// 从取值获取对应的名称
	StringName get_name_from_value(int v) const;
	// 已使用的取值集合
	Span<const uint8_t> get_used_values() const;
	// 该取值是否被使用
	bool is_value_used(int v) const;
	// bool has_named_values() const;

	// 该属性是否表示旋转
	inline bool is_rotation() const {
		return _is_rotation;
	}

	// 如果属性表示旋转，获取与该旋转对应的 OrthoBasis 索引。
	unsigned int get_ortho_rotation_index_from_value(int value) const;

	// 该属性是否与其它属性等价
	bool is_equivalent(const VoxelBlockyAttribute &other) const;

#ifdef TOOLS_ENABLED
	virtual void get_configuration_warnings(PackedStringArray &out_warnings) const;
#endif

	// 按名称排序
	static void sort_by_name(Span<Ref<VoxelBlockyAttribute>> attributes);
	static void sort_by_name(Span<StringName> attributes);
	static void sort_by_name(Span<std::pair<StringName, uint8_t>> attributes);

private:
	static void _bind_methods();

protected:
	StringName _name;
	uint8_t _default_value = 0;
	bool _is_rotation = false;
	StdVector<StringName> _value_names;
	StdVector<uint8_t> _ortho_rotations;

	// 属性不一定使用其范围内的所有值，有些值可能未被使用。但它们不应参与
	// 烘焙过程（不能有"空洞"）。
	StdVector<uint8_t> _used_values;
};

} // namespace voxel

#endif // VOXEL_BLOCKY_ATTRIBUTE_H
