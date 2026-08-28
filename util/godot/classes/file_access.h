#ifndef VOXEL_GODOT_FILE_ACCESS_H
#define VOXEL_GODOT_FILE_ACCESS_H

#if defined(VOXEL_GODOT)
#include <core/io/file_access.h>

#endif

#include "../../containers/span.h"

namespace voxel::godot {

inline bool file_exists(const String &path) {
#if defined(VOXEL_GODOT)
	return FileAccess::exists(path);
#endif
}

inline Ref<FileAccess> open_file(const String path, FileAccess::ModeFlags mode_flags, Error &out_error) {
#if defined(VOXEL_GODOT)
	return FileAccess::open(path, mode_flags, &out_error);
#endif
}

inline uint64_t get_buffer(FileAccess &f, Span<uint8_t> dst) {
#if defined(VOXEL_GODOT)
	return f.get_buffer(dst.data(), dst.size());
#endif
}

inline void store_buffer(FileAccess &f, Span<const uint8_t> src) {
#if defined(VOXEL_GODOT)
	f.store_buffer(src.data(), src.size());
#endif
}

inline String get_as_text(FileAccess &f) {
#if defined(VOXEL_GODOT)
	return f.get_as_utf8_string();
#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_FILE_ACCESS_H
