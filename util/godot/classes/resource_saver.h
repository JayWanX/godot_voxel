#ifndef VOXEL_GODOT_RESOURCE_SAVER_H
#define VOXEL_GODOT_RESOURCE_SAVER_H

#if defined(VOXEL_GODOT)
#include <core/io/resource_saver.h>
#endif

namespace voxel::godot {

inline Error save_resource(const Ref<Resource> &resource, const String &path = "",
		ResourceSaver::SaverFlags flags = ResourceSaver::FLAG_NONE) {
#if defined(VOXEL_GODOT)
	return ResourceSaver::save(resource, path, flags);
#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_RESOURCE_SAVER_H
