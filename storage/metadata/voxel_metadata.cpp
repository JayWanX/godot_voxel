#include "voxel_metadata.h"
#include "../../util/errors.h"
#include "custom_voxel_metadata.h"

namespace voxel {

void VoxelMetadata::clear() {
	if (_type >= TYPE_CUSTOM_BEGIN) {
		VOXEL_DELETE(_data.custom_data);
		_data.custom_data = nullptr;
	}
	_type = TYPE_EMPTY;
}

void VoxelMetadata::set_u64(const uint64_t &v) {
	if (_type != TYPE_U64) {
		clear();
		_type = TYPE_U64;
	}
	_data.u64_data = v;
}

uint64_t VoxelMetadata::get_u64() const {
	VOXEL_ASSERT(_type == TYPE_U64);
	return _data.u64_data;
}

void VoxelMetadata::set_custom(uint8_t type, ICustomVoxelMetadata *custom_data) {
	VOXEL_ASSERT(type >= TYPE_CUSTOM_BEGIN);
	clear();
	_type = type;
	_data.custom_data = custom_data;
}

ICustomVoxelMetadata &VoxelMetadata::get_custom() {
	VOXEL_ASSERT(_type >= TYPE_CUSTOM_BEGIN);
	VOXEL_ASSERT(_data.custom_data != nullptr);
	return *_data.custom_data;
}

const ICustomVoxelMetadata &VoxelMetadata::get_custom() const {
	VOXEL_ASSERT(_type >= TYPE_CUSTOM_BEGIN);
	VOXEL_ASSERT(_data.custom_data != nullptr);
	return *_data.custom_data;
}

void VoxelMetadata::copy_from(const VoxelMetadata &src) {
	clear();
	if (src._type >= TYPE_CUSTOM_BEGIN) {
		VOXEL_ASSERT(src._data.custom_data != nullptr);
		_data.custom_data = src._data.custom_data->duplicate();
	} else {
		_data = src._data;
	}
	_type = src._type;
}

bool VoxelMetadata::equals(const VoxelMetadata &other) const {
	if (_type != other._type) {
		return false;
	}
	switch (_type) {
		case TYPE_EMPTY:
			return true;
		case TYPE_U64:
			return _data.u64_data == other._data.u64_data;
		default:
			if (_type >= TYPE_CUSTOM_BEGIN) {
				VOXEL_ASSERT(_data.custom_data != nullptr);
				VOXEL_ASSERT(other._data.custom_data != nullptr);
				return _data.custom_data->equals(*other._data.custom_data);
			} else {
				VOXEL_PRINT_ERROR("Non-implemented comparison");
				return false;
			}
	}
}

} // namespace voxel
