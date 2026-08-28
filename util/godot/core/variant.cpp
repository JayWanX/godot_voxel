#include "variant.h"

#if defined(VOXEL_GODOT)
#include <core/io/marshalls.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/utility_functions.hpp>
#endif

namespace voxel::godot {

size_t get_variant_encoded_size(const Variant &src) {
#if defined(VOXEL_GODOT)
	int len;
	const Error err = encode_variant(src, nullptr, len, false);
	VOXEL_ASSERT_RETURN_V_MSG(err == OK, 0, "Error when trying to encode Variant metadata.");
	return len;
#elif defined(VOXEL_GODOT_EXTENSION)
	// TODO Optimization: this is a waste. Godot would have to expose support for getting the size up-front.
	// but that likely won't happen soon given how niche that is
	::godot::PackedByteArray dst = ::godot::UtilityFunctions::var_to_bytes(src);
	return dst.size();
#endif
}

size_t encode_variant(const Variant &src, Span<uint8_t> dst) {
#if defined(VOXEL_GODOT)
	int written_length;
	const Error err = encode_variant(src, dst.data(), written_length, false);
	VOXEL_ASSERT_RETURN_V(err == OK, 0);
	return written_length;

#elif defined(VOXEL_GODOT_EXTENSION)
	// TODO Optimization: this is a waste. Godot would have to expose support for writing directly to a raw buffer.
	// but that likely won't happen soon given how niche that is
	::godot::PackedByteArray pba = ::godot::UtilityFunctions::var_to_bytes(src);

	const size_t written_length = pba.size();
	VOXEL_ASSERT_RETURN_V(written_length <= dst.size(), 0);

	const uint8_t *pba_data = pba.ptr();
	VOXEL_ASSERT_RETURN_V(pba_data != nullptr, 0);

	memcpy(dst.data(), pba_data, written_length);

	return pba.size();
#endif
}

bool decode_variant(Span<const uint8_t> src, Variant &dst, size_t &out_read_size) {
#if defined(VOXEL_GODOT)
	int read_length;
	const Error err = decode_variant(dst, src.data(), src.size(), &read_length, false);
	VOXEL_ASSERT_RETURN_V_MSG(err == OK, false, "Failed to deserialize Variant");
	out_read_size = read_length;
	return true;

#elif defined(VOXEL_GODOT_EXTENSION)
	// TODO Optimization: this is a waste. Godot would have to expose support for reading directly from a raw buffer.
	// but that likely won't happen soon given how niche that is
	::godot::PackedByteArray pba;
	pba.resize(src.size());
	uint8_t *pba_data = pba.ptrw();
	VOXEL_ASSERT_RETURN_V(pba_data != nullptr, false);
	memcpy(pba_data, src.data(), src.size());
	dst = ::godot::UtilityFunctions::bytes_to_var(pba);
	// TODO Can't tell how many bytes were actually read!
	out_read_size = src.size();
	return true;
#endif
}

} // namespace voxel::godot
