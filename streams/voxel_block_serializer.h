#ifndef VOXEL_BLOCK_SERIALIZER_H
#define VOXEL_BLOCK_SERIALIZER_H

#include "../util/containers/span.h"
#include "../util/containers/std_vector.h"
#include "../util/godot/macros.h"
#include "compressed_data.h"

#include <cstdint>

class FileAccess;

namespace voxel {

class VoxelBuffer;

namespace BlockSerializer {

// 序列化时使用的最新版本
static const uint8_t BLOCK_FORMAT_VERSION = 4;

struct SerializeResult {
	// 所指向对象的生命周期仅在调用线程内有效，
	// 直到进行下一次序列化或反序列化调用为止。
	// TODO 最终要整理分配器，以便调用方决定
	const StdVector<uint8_t> &data;
	bool success;

	inline SerializeResult(const StdVector<uint8_t> &p_data, bool p_success) : data(p_data), success(p_success) {}
};

SerializeResult serialize(const VoxelBuffer &voxel_buffer);
bool deserialize(Span<const uint8_t> p_data, VoxelBuffer &out_voxel_buffer);

SerializeResult serialize_and_compress(
		const VoxelBuffer &voxel_buffer,
		const CompressedData::Compression compression_mode
);
bool decompress_and_deserialize(Span<const uint8_t> p_data, VoxelBuffer &out_voxel_buffer);
bool decompress_and_deserialize(FileAccess &f, unsigned int size_to_read, VoxelBuffer &out_voxel_buffer);

// 供内部使用的临时线程本地缓冲区
StdVector<uint8_t> &get_tls_data();
StdVector<uint8_t> &get_tls_compressed_data();

} // namespace BlockSerializer
} // namespace voxel

#endif // VOXEL_BLOCK_SERIALIZER_H
