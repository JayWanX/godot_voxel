#ifndef VOXEL_GODOT_RESOURCE_LOADER_H
#define VOXEL_GODOT_RESOURCE_LOADER_H

#include <core/io/resource_loader.h>

namespace voxel::godot {

PackedStringArray get_recognized_extensions_for_type(const String &type_name);
Ref<Resource> load_resource(const String &path);

} // namespace voxel::godot

#endif // VOXEL_GODOT_RESOURCE_LOADER_H
