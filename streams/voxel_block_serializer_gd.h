#ifndef VOXEL_BLOCK_SERIALIZER_GD_H
#define VOXEL_BLOCK_SERIALIZER_GD_H

#include "../storage/voxel_buffer_gd.h"
#include "compressed_data.h"

class StreamPeer;

namespace voxel::godot {

class VoxelBuffer;

// 面向 Godot 的 BlockSerializer API
// TODO 可以是单例吗？或做成 VoxelBuffer 的方法？该对象没有状态。
class VoxelBlockSerializer : public RefCounted {
	GDCLASS(VoxelBlockSerializer, RefCounted)
public:
	enum Compression {
		COMPRESSION_NONE,
		COMPRESSION_LZ4,
		COMPRESSION_ZSTD,
	};

	// 必须使用它，因为内部枚举没有连续的 ID（因保存数据而无法更改），
	// 而 Godot 需要连续的 ID……
	// 将内部压缩枚举转为 Godot 枚举
	static Compression compression_to_gd(const CompressedData::Compression src);
	// 将 Godot 压缩枚举转为内部枚举
	static CompressedData::Compression compression_from_gd(const Compression src);

	static const char *COMPRESSION_MODE_HINT_STRING;

	// 将缓冲区序列化后写入数据流
	static int serialize_to_stream_peer(
			Ref<StreamPeer> peer,
			Ref<VoxelBuffer> voxel_buffer,
			const Compression compress_mode
	);

	// 从数据流读取并反序列化到缓冲区
	static void deserialize_from_stream_peer(
			Ref<StreamPeer> peer,
			Ref<VoxelBuffer> voxel_buffer,
			const int64_t size,
			const bool compress_mode
	);

	// 将缓冲区序列化为字节数组
	static PackedByteArray serialize_to_byte_array(Ref<VoxelBuffer> voxel_buffer, const Compression compress_mode);

	// 从字节数组反序列化到缓冲区
	static void deserialize_from_byte_array(
			PackedByteArray bytes,
			Ref<VoxelBuffer> voxel_buffer,
			const bool decompress
	);

	static void _bind_methods();
};

} // namespace voxel::godot

VARIANT_ENUM_CAST(voxel::godot::VoxelBlockSerializer::Compression);

#endif // VOXEL_BLOCK_SERIALIZER_GD_H
