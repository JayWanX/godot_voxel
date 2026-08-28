#ifndef VOXEL_GODOT_OBJECT_H
#define VOXEL_GODOT_OBJECT_H

#if defined(VOXEL_GODOT)
#include <core/object/object.h>
// The `GDCLASS` macro isn't compiling when inheriting `Object`, unless `class_db.h` is also included
#include <core/object/class_db.h>
#endif

#include "../../containers/std_vector.h"


namespace voxel::godot {

// Gets a hash of a given object from its properties. If properties are objects too, they are recursively
// parsed. Note that restricting to editable properties is important to avoid costly properties with objects
// such as textures or meshes.
uint64_t get_deep_hash(
		const Object &obj,
		uint32_t property_usage = PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_EDITOR,
		uint64_t hash = 0
);

// Unified representation of property info.
struct PropertyInfoWrapper {
	Variant::Type type;
	String name;
	uint32_t usage;
};
void get_property_list(const Object &obj, StdVector<PropertyInfoWrapper> &out_properties);

// Turns out these functions are only used in editor for now.
// They are generic, but I have to wrap them, otherwise GCC throws warnings-as-errors for them being unused.
#ifdef TOOLS_ENABLED

void set_object_edited(Object &obj);

#endif

} // namespace voxel::godot

#endif // VOXEL_GODOT_OBJECT_H
