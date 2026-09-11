#ifndef VOXEL_GODOT_RESOURCE_SAVER_H
#define VOXEL_GODOT_RESOURCE_SAVER_H

#include <core/io/resource_saver.h>

namespace voxel::godot {

inline Error save_resource(const Ref<Resource> &resource, const String &path = "",
		ResourceSaver::SaverFlags flags = ResourceSaver::FLAG_NONE) {
	return ResourceSaver::save(resource, path, flags);
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_RESOURCE_SAVER_H
