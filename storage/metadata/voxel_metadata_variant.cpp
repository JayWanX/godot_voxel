#include "voxel_metadata_variant.h"

namespace voxel::godot {

size_t VoxelMetadataVariant::get_serialized_size() const {
	return voxel::godot::get_variant_encoded_size(data);
}

size_t VoxelMetadataVariant::serialize(Span<uint8_t> dst) const {
	return voxel::godot::encode_variant(data, dst);
}

bool VoxelMetadataVariant::deserialize(Span<const uint8_t> src, uint64_t &out_read_size) {
	size_t read_size = 0;
	const bool success = voxel::godot::decode_variant(src, data, read_size);
	out_read_size = read_size;
	return success;
}

ICustomVoxelMetadata *VoxelMetadataVariant::duplicate() {
	VoxelMetadataVariant *d = VOXEL_NEW(VoxelMetadataVariant);
	d->data = data.duplicate();
	return d;
}

uint8_t VoxelMetadataVariant::get_type_index() const {
	return METADATA_TYPE_VARIANT;
}

bool VoxelMetadataVariant::equals(const ICustomVoxelMetadata &other) const {
	if (other.get_type_index() != get_type_index()) {
		return false;
	}
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(dynamic_cast<const VoxelMetadataVariant *>(&other) != nullptr);
#endif
	const VoxelMetadataVariant &other_v = static_cast<const VoxelMetadataVariant &>(other);
	// TODO 实现深度比较？
	return data == other_v.data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Variant get_as_variant(const VoxelMetadata &meta) {
	switch (int(meta.get_type())) {
		case METADATA_TYPE_VARIANT: {
			const VoxelMetadataVariant &mv = static_cast<const VoxelMetadataVariant &>(meta.get_custom());
			return mv.data;
		}
		case VoxelMetadata::TYPE_EMPTY: {
			return Variant();
		}
		case VoxelMetadata::TYPE_U64: {
			return Variant(int64_t(meta.get_u64()));
		}
		default:
			VOXEL_PRINT_ERROR("Unknown VoxelMetadata type");
			return Variant();
	}
}

void set_as_variant(VoxelMetadata &meta, const Variant &v) {
	if (v.get_type() == Variant::NIL) {
		meta.clear();
	} else {
		if (int(meta.get_type()) == METADATA_TYPE_VARIANT) {
			VoxelMetadataVariant &mv = static_cast<VoxelMetadataVariant &>(meta.get_custom());
			mv.data = v;
			return;
		} else {
			VoxelMetadataVariant *mv = VOXEL_NEW(VoxelMetadataVariant);
			mv->data = v;
			meta.set_custom(METADATA_TYPE_VARIANT, mv);
		}
	}
}

} // namespace voxel::godot
