#ifndef VOXEL_GODOT_VARIANT_H
#define VOXEL_GODOT_VARIANT_H

#if defined(VOXEL_GODOT)
#include <core/variant/variant.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#endif

#include "../../containers/span.h"

namespace voxel::godot {

size_t get_variant_encoded_size(const Variant &src);
size_t encode_variant(const Variant &src, Span<uint8_t> dst);
bool decode_variant(Span<const uint8_t> src, Variant &dst, size_t &out_read_size);

} // namespace voxel::godot

#endif // VOXEL_GODOT_VARIANT_H
