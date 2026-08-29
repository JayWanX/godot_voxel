#include "object.h"
#include "../../hash_funcs.h"
#include "../../profiling.h"

namespace voxel::godot {

void get_property_list(const Object &obj, StdVector<PropertyInfoWrapper> &out_properties) {
#if defined(VOXEL_GODOT)
	List<PropertyInfo> properties;
	obj.get_property_list(&properties, false);
	// 我本想使用 ConstIterator，因为我只读取该列表，但那是不可能的 :shrug:
	for (List<PropertyInfo>::Iterator it = properties.begin(); it != properties.end(); ++it) {
		const PropertyInfo property = *it;
		PropertyInfoWrapper pi;
		pi.type = property.type;
		pi.name = property.name;
		pi.usage = property.usage;
		out_properties.push_back(pi);
	}
#endif
}

uint64_t get_deep_hash(const Object &obj, uint32_t property_usage, uint64_t hash) {
	VOXEL_PROFILE_SCOPE();

	hash = hash_djb2_one_64(obj.get_class().hash(), hash);

	StdVector<PropertyInfoWrapper> properties;
	get_property_list(obj, properties);

	// 我本想使用 ConstIterator，因为我只读取该列表，但那是不可能的 :shrug:
	for (const PropertyInfoWrapper &property : properties) {
		if ((property.usage & property_usage) != 0) {
			const Variant value = obj.get(property.name);
			uint64_t value_hash = 0;

			if (value.get_type() == Variant::OBJECT) {
				const Object *obj_value = value.operator Object *();
				if (obj_value != nullptr) {
					value_hash = get_deep_hash(*obj_value, property_usage, hash);
				}

			} else {
				value_hash = value.hash();
			}

			hash = hash_djb2_one_64(value_hash, hash);
		}
	}

	return hash;
}

#ifdef TOOLS_ENABLED

void set_object_edited(Object &obj) {
#if defined(VOXEL_GODOT)
	obj.set_edited(true);

#endif
}

#endif // TOOLS_ENABLED

} // namespace voxel::godot
