#ifndef VOXEL_BLOCKY_TYPE_LIBRARY_H
#define VOXEL_BLOCKY_TYPE_LIBRARY_H

#include "../../../util/containers/std_vector.h"
#include "../voxel_blocky_library_base.h"
#include "voxel_blocky_type.h"

namespace voxel {

// 暴露类型数组的库，用于类似 Minecraft 数据块的高层系统。此数组中的索引
// 无关紧要，但名称很重要。模型、旋转和体素 ID 会根据每种类型的属性自动生成。
class VoxelBlockyTypeLibrary : public VoxelBlockyLibraryBase {
	GDCLASS(VoxelBlockyTypeLibrary, VoxelBlockyLibraryBase)
public:
	static constexpr unsigned int MAX_TYPES = 65536;

	void clear() override;
	void load_default() override;
	void bake() override;
#ifdef TOOLS_ENABLED
	void get_configuration_warnings(PackedStringArray &out_warnings) const override;
#endif

	int get_model_index_default(StringName type_name) const;

	// 快捷方式，无需指定属性名称。
	int get_model_index_single_attribute(StringName type_name, Variant p_attrib_value) const;

	// 从类型名称及其所有属性的值获取模型索引。值可以指定
	// 为整数、布尔值，或者如果有名称则使用其名称。
	//
	// 参数示例：
	//
	// (&"mygame:button", {
	//     "direction": VoxelBlockyAttributeDirection.DIR_POSITIVE_Z,
	//     "active": &"on",
	//     "powered": false
	// })
	//
	// 警告：此方法较慢。请考虑在非密集代码中使用（例如一次只编辑少量体素？），或者将
	// 结果缓存在变量中。
	// 它很慢是因为：
	// - Dictionary 及其每个键都必须在堆上分配
	// - 按设计 Dictionary 键不能是 `StringName`（Godot 会将它们转换为 `String`，但
	//   函数内部必须将它们转回 `StringName`）
	// - 函数必须遍历字典，而这正是字典缓慢的另一个原因。
	//   不过使用 Dictionary 是为了方便，并与 setter 函数保持一致。
	// - 必须对属性进行排序并在内部数据结构中查找，以获取实际的体素 ID。
	//
	int get_model_index_with_attributes(StringName type_name, Dictionary attribs_dict) const;

	Ref<VoxelBlockyType> get_type_from_name(StringName p_name) const;

	// 返回的数组有两个元素：
	// - 类型名称（StringName）
	// - 一个字典，其中键是属性名称（很遗憾是 String，因为 Godot 开发者决定强制
	//   在 Dictionary 中将 StringName 转换为 String），值是该属性当前的整数值。
	Array get_type_name_and_attributes_from_model_index(int i) const;

	bool load_id_map_from_string_array(PackedStringArray array);
	PackedStringArray serialize_id_map_to_string_array() const;

	bool load_id_map_from_json(String array);
	String serialize_id_map_to_json() const;

	void get_id_map_preview(PackedStringArray &out_ids, StdVector<uint16_t> &used_ids) const;

private:
	// 标识特定模型的完整限定名称，由类型和每个属性的状态组成。
	struct VoxelID {
		StringName type_name;
		VoxelBlockyType::VariantKey variant_key;

		bool operator==(const VoxelID &other) const {
			return type_name == other.type_name && variant_key == other.variant_key;
		}

		String to_string() const;
	};

	void update_id_map();
	void update_id_map(StdVector<VoxelID> &id_map, StdVector<uint16_t> *used_ids) const;
	static PackedStringArray serialize_id_map_to_string_array(const StdVector<VoxelID> &id_map);

	static bool parse_voxel_id(const String &str, VoxelID &out_id);

	int get_model_index(const VoxelID queried_id) const;

	PackedStringArray _b_get_id_map();
	void _b_set_id_map(PackedStringArray sarray);
	TypedArray<VoxelBlockyType> _b_get_types() const;
	void _b_set_types(TypedArray<VoxelBlockyType> types);
	PackedStringArray _b_serialize_id_map_to_string_array() const;

	static void _bind_methods();

	// 无序。可能包含 null。
	StdVector<Ref<VoxelBlockyType>> _types;

	// 将体素数据索引映射到完整限定的模型名称。这用于确保模型 ID 保持不变，
	// 只要其类型具有相同的名称且属性值相同。
	// 可能引用已不存在的类型。
	// 索引和大小与 `_baked_data.models` 匹配。
	StdVector<VoxelID> _id_map;
};

} // namespace voxel

#endif // VOXEL_BLOCKY_TYPE_LIBRARY_H
