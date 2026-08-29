#ifndef VOXEL_BLOCKY_TYPE_H
#define VOXEL_BLOCKY_TYPE_H

#include "../../../util/containers/std_vector.h"
#include "../../../util/godot/classes/resource.h"
#include "../voxel_blocky_fluid.h"
#include "../voxel_blocky_model.h"
#include "voxel_blocky_attribute.h"

namespace voxel {

// 一种体素"类型"的高层表示，用于 `VoxelMesherBlocky` 和 `VoxelBlockyTypeLibrary`。
// 一种类型可以在体素数据中表示多个可能的值，对应于该类型的状态（例如
// 旋转、连接、开/关等）。
class VoxelBlockyType : public Resource {
	GDCLASS(VoxelBlockyType, Resource)
public:
	// 应尽可能保持较低。需要时再增加。
	static const int MAX_ATTRIBUTES = 4;
	// 可作为组合列表编辑的最大变体数量。
	// 技术上限是 65536，但达到数百就表明存在设计问题，且编辑起来会不堪重负。不过
	// 这种数量的变体是有可能的，此时我们应该实现条件逻辑。
	static const int MAX_EDITING_VARIANTS = 256;

	VoxelBlockyType();

	void set_unique_name(StringName p_name);
	StringName get_unique_name() const;

	void set_base_model(Ref<VoxelBlockyModel> model);
	Ref<VoxelBlockyModel> get_base_model() const;

	Span<const Ref<VoxelBlockyAttribute>> get_attributes() const;
	Ref<VoxelBlockyAttribute> get_attribute_by_name(const StringName &attrib_name) const;
	Ref<VoxelBlockyAttribute> get_rotation_attribute() const;

	void get_checked_attributes(StdVector<Ref<VoxelBlockyAttribute>> &out_attribs) const;

	// 标识类型的一个模型变体，即它拥有的属性及其值。
	struct VariantKey {
		// 名称必须按字符串排序（不是 StringName 指针比较）。
		// 在设计上这并非必需，用户和配置文件可以任意顺序指定属性，
		// 但我们在运行时这样做以提升查找性能。
		// 名称和值必须紧排在数组开头。未使用的值必须为默认值。
		FixedArray<StringName, MAX_ATTRIBUTES> attribute_names;
		FixedArray<uint8_t, MAX_ATTRIBUTES> attribute_values;

		VariantKey() {
			fill(attribute_values, uint8_t(0));
		}

		bool operator==(const VariantKey &other) const {
			return attribute_names == other.attribute_names && attribute_values == other.attribute_values;
		}

		String to_string() const;
		String to_string(Span<const Ref<VoxelBlockyAttribute>> context_attributes) const;
		bool parse_from_array(const Array &array);
		Array to_array() const;
		void sort();
	};

	// 获取或设置与特定变体关联的模型（它们不一定是最终结果）
	void set_variant(const VariantKey &key, Ref<VoxelBlockyModel> model);
	Ref<VoxelBlockyModel> get_variant(const VariantKey &key) const;

	void bake(
			StdVector<blocky::BakedModel> &out_models,
			StdVector<VariantKey> &out_keys,
			blocky::MaterialIndexer &material_indexer,
			const VariantKey *specific_key,
			bool bake_tangents,
			StdVector<Ref<VoxelBlockyFluid>> &indexed_fluids,
			StdVector<blocky::BakedFluid> &baked_fluids
	) const;

#ifdef TOOLS_ENABLED
	void get_configuration_warnings(PackedStringArray &out_warnings) const;
#endif

	Ref<Mesh> get_preview_mesh(const VariantKey &key) const;

	void generate_keys(StdVector<VariantKey> &out_keys, bool include_rotations) const;

private:
	// 在属性可用于处理之前，过滤空项、移除重复项并对属性排序
	static void gather_and_sort_attributes(
			const StdVector<Ref<VoxelBlockyAttribute>> &attributes_with_maybe_nulls,
			StdVector<Ref<VoxelBlockyAttribute>> &out_attributes
	);

	// 从已排序的属性生成所有组合。
	static void generate_keys(
			const StdVector<Ref<VoxelBlockyAttribute>> &attributes,
			StdVector<VariantKey> &out_keys,
			bool include_rotations
	);

	void _on_attribute_changed();
	void _on_base_model_changed();

	TypedArray<VoxelBlockyAttribute> _b_get_attributes() const;
	void _b_set_attributes(TypedArray<VoxelBlockyAttribute> attributes);

	// bool _set(const StringName &p_name, const Variant &p_value);
	// bool _get(const StringName &p_name, Variant &r_ret) const;
	// void _get_property_list(List<PropertyInfo> *p_list) const;

	void _b_set_variant_model(Array p_key, Ref<VoxelBlockyModel> model);
	void _b_set_variant_models_data(Array data);
	Array _b_get_variant_models_data() const;

	static void _bind_methods();

	// 类型的名称，用于开发、配置文件、存档文件或命令中。它必须是唯一的，
	// 如果你的游戏支持 modding，可能还需要加前缀。要在游戏中显示它，
	// 可能更推荐使用翻译字典而不是直接使用它。
	StringName _name;

	Ref<VoxelBlockyModel> _base_model;

	// 编辑器中指定的未检查属性列表。可能包含空值、重复项，并且未排序。
	// 以这种方式存储是为了方便在 Godot 编辑器中编辑……
	// TODO 重命名为 `_unchecked_attributes`？
	StdVector<Ref<VoxelBlockyAttribute>> _attributes;

	// TODO 自动旋转并不总是可行。
	// 例如，如果一个 3 轴数据块不对称，并且在其 Y 轴配置中需要始终朝下，
	// 就没有办法做到这一点（除非浪费一个 6 方向属性）

	// 如果为 true，旋转属性将不需要用户为每次旋转指定模型。它们将
	// 自动生成，以默认旋转为参照。
	bool _automatic_rotations = true;

	struct VariantData {
		VariantKey key;
		Ref<VoxelBlockyModel> model;
	};

	// 这里只包含用户在编辑器中显式定义的变体。并非所有运行时变体都在其中。
	// 也可能包含与任何属性都无关的变体，但这些变体不会被保存。它们保留在内存中，
	// 以便用户在编辑器中做更改时可以在不同配置之间来回切换。
	// 已保存的变体由当前有效属性的组合决定。
	StdVector<VariantData> _variants;

	// TODO 条件模型
};

String to_string(const VoxelBlockyType::VariantKey &key);

} // namespace voxel

#endif // VOXEL_BLOCKY_TYPE_H
