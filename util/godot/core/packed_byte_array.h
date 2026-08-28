#ifndef VOXEL_GODOT_PACKED_BYTE_ARRAY_H
#define VOXEL_GODOT_PACKED_BYTE_ARRAY_H

#if defined(VOXEL_GODOT)
#include <core/io/file_access.h>
#include <core/variant/variant.h>

#endif

namespace voxel::godot {
namespace PackedByteArrayUtility {

PackedByteArray compress(const PackedByteArray &self, const FileAccess::CompressionMode p_mode);

PackedByteArray decompress(
		const PackedByteArray &self,
		const int64_t buffer_size,
		const FileAccess::CompressionMode p_mode
);

} // namespace PackedByteArrayUtility
} // namespace voxel::godot

#endif // VOXEL_GODOT_PACKED_BYTE_ARRAY_H
