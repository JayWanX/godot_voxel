#include "variant.h"

#if defined(VOXEL_GODOT)
#include <core/io/marshalls.h>
#endif

namespace voxel::godot {

size_t get_variant_encoded_size(const Variant &src) {
#if defined(VOXEL_GODOT)
	int len;
	const Error err = encode_variant(src, nullptr, len, false);
	VOXEL_ASSERT_RETURN_V_MSG(err == OK, 0, "Error when trying to encode Variant metadata.");
	return len;
#endif
}

size_t encode_variant(const Variant &src, Span<uint8_t> dst) {
#if defined(VOXEL_GODOT)
	int written_length;
	const Error err = encode_variant(src, dst.data(), written_length, false);
	VOXEL_ASSERT_RETURN_V(err == OK, 0);
	return written_length;

#endif
}

bool decode_variant(Span<const uint8_t> src, Variant &dst, size_t &out_read_size) {
#if defined(VOXEL_GODOT)
	int read_length;
	const Error err = decode_variant(dst, src.data(), src.size(), &read_length, false);
	VOXEL_ASSERT_RETURN_V_MSG(err == OK, false, "Failed to deserialize Variant");
	out_read_size = read_length;
	return true;

#endif
}

} // namespace voxel::godot
