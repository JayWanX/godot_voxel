#ifndef VOXEL_GODOT_RESOURCE_LOADER_H
#define VOXEL_GODOT_RESOURCE_LOADER_H

#if defined(VOXEL_GODOT)
#include <core/io/resource_loader.h>
#endif

namespace voxel::godot {

PackedStringArray get_recognized_extensions_for_type(const String &type_name);
Ref<Resource> load_resource(const String &path);

} // namespace voxel::godot

#endif // VOXEL_GODOT_RESOURCE_LOADER_H
