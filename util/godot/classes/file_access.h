#ifndef VOXEL_GODOT_FILE_ACCESS_H
#define VOXEL_GODOT_FILE_ACCESS_H

#include <core/io/file_access.h>


#include "../../containers/span.h"

namespace voxel::godot {

inline bool file_exists(const String &path) {
	return FileAccess::exists(path);
}

inline Ref<FileAccess> open_file(const String path, FileAccess::ModeFlags mode_flags, Error &out_error) {
	return FileAccess::open(path, mode_flags, &out_error);
}

inline uint64_t get_buffer(FileAccess &f, Span<uint8_t> dst) {
	return f.get_buffer(dst.data(), dst.size());
}

inline void store_buffer(FileAccess &f, Span<const uint8_t> src) {
	f.store_buffer(src.data(), src.size());
}

inline String get_as_text(FileAccess &f) {
	return f.get_as_utf8_string();
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_FILE_ACCESS_H
