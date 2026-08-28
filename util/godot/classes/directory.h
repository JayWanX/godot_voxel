#ifndef VOXEL_GODOT_DIRECTORY_H
#define VOXEL_GODOT_DIRECTORY_H

#if defined(VOXEL_GODOT)
#include <core/io/dir_access.h>
#endif

namespace voxel::godot {

inline Ref<DirAccess> open_directory(const String &directory_path, Error *out_err) {
	Ref<DirAccess> dir = DirAccess::open(directory_path);
	if (out_err != nullptr) {
		*out_err = DirAccess::get_open_error();
	}
	return dir;
}

inline bool directory_exists(const String &directory_path) {
	return DirAccess::dir_exists_absolute(directory_path);
}

inline bool directory_exists(DirAccess &dir, const String &relative_directory_path) {
	// Why this function is not `const`, I wonder
	return dir.dir_exists(relative_directory_path);
}

inline Error rename_directory(const String &from, const String &to) {
	return DirAccess::rename_absolute(from, to);
}

Error erase_directory_contents_recursive(DirAccess &da);

} // namespace voxel::godot

#endif // VOXEL_GODOT_DIRECTORY_H
