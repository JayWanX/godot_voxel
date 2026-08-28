#include "resource_loader.h"
#include "resource.h"

namespace voxel::godot {

PackedStringArray get_recognized_extensions_for_type(const String &type_name) {
#if defined(VOXEL_GODOT)
	List<String> extensions_list;
	ResourceLoader::get_recognized_extensions_for_type(type_name, &extensions_list);
	PackedStringArray extensions_array;
	for (const String &extension : extensions_list) {
		extensions_array.push_back(extension);
	}
	return extensions_array;

#elif defined(VOXEL_GODOT_EXTENSION)
	return ResourceLoader::get_singleton()->get_recognized_extensions_for_type(type_name);
#endif
}

Ref<Resource> load_resource(const String &path) {
#if defined(VOXEL_GODOT)
	return ResourceLoader::load(path);
#elif defined(VOXEL_GODOT_EXTENSION)
	return ResourceLoader::get_singleton()->load(path);
#endif
}

} // namespace voxel::godot
