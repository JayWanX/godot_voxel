#include "resource_loader.h"
#include "resource.h"

namespace voxel::godot {

PackedStringArray get_recognized_extensions_for_type(const String &type_name) {
	List<String> extensions_list;
	ResourceLoader::get_recognized_extensions_for_type(type_name, &extensions_list);
	PackedStringArray extensions_array;
	for (const String &extension : extensions_list) {
		extensions_array.push_back(extension);
	}
	return extensions_array;

}

Ref<Resource> load_resource(const String &path) {
	return ResourceLoader::load(path);
}

} // namespace voxel::godot
