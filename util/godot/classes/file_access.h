#ifndef VOXEL_GODOT_FILE_ACCESS_H
#define VOXEL_GODOT_FILE_ACCESS_H

#if defined(VOXEL_GODOT)
#include <core/io/file_access.h>

#elif defined(VOXEL_GODOT_EXTENSION)
#include "../core/packed_arrays.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/global_constants.hpp> // For `Error`
using namespace godot;
#endif

#include "../../containers/span.h"

namespace voxel::godot {

inline bool file_exists(const String &path) {
#if defined(VOXEL_GODOT)
	return FileAccess::exists(path);
#elif defined(VOXEL_GODOT_EXTENSION)
	return FileAccess::file_exists(path);
#endif
}

inline Ref<FileAccess> open_file(const String path, FileAccess::ModeFlags mode_flags, Error &out_error) {
#if defined(VOXEL_GODOT)
	return FileAccess::open(path, mode_flags, &out_error);
#elif defined(VOXEL_GODOT_EXTENSION)
	Ref<FileAccess> file = FileAccess::open(path, mode_flags);
	out_error = FileAccess::get_open_error();
	if (out_error != ::godot::OK) {
		return Ref<FileAccess>();
	} else {
		return file;
	}
#endif
}

inline uint64_t get_buffer(FileAccess &f, Span<uint8_t> dst) {
#if defined(VOXEL_GODOT)
	return f.get_buffer(dst.data(), dst.size());
#elif defined(VOXEL_GODOT_EXTENSION)
	PackedByteArray bytes = f.get_buffer(dst.size());
	copy_to(dst, bytes);
	return bytes.size();
#endif
}

inline void store_buffer(FileAccess &f, Span<const uint8_t> src) {
#if defined(VOXEL_GODOT)
	f.store_buffer(src.data(), src.size());
#elif defined(VOXEL_GODOT_EXTENSION)
	PackedByteArray bytes;
	copy_to(bytes, src);
	f.store_buffer(bytes);
#endif
}

inline String get_as_text(FileAccess &f) {
#if defined(VOXEL_GODOT)
	return f.get_as_utf8_string();
#elif defined(VOXEL_GODOT_EXTENSION)
	return f.get_as_text();
#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_FILE_ACCESS_H
