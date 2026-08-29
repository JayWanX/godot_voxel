#ifndef VOXEL_COMPRESSED_DATA_H
#define VOXEL_COMPRESSED_DATA_H

#include "../util/containers/span.h"
#include "../util/containers/std_vector.h"
#include <cstdint>

namespace voxel::CompressedData {

// 压缩数据的开头是一个字节，用于指明所使用的压缩格式。
// 其后的内容取决于该格式。

enum Compression {
	// 不压缩。其后所有字节均按原样读取。
	// 可用于调试。
	COMPRESSION_NONE = 0,
	// [已弃用]
	// 紧随其后的 uint32_t 为以大端格式存储的解压后数据大小。
	// 其后所有字节均为使用 LZ4 默认参数的压缩数据。
	// 这是速度最快的压缩格式。
	COMPRESSION_LZ4_BE = 1,
	// 紧随其后的 uint32_t 为以小端格式存储的解压后数据大小。
	// 其后所有字节均为使用 LZ4 默认参数的压缩数据。
	// 这是速度最快的压缩格式。
	COMPRESSION_LZ4 = 2,
	COMPRESSION_ZSTD = 3,
	COMPRESSION_COUNT = 4
};

bool compress(Span<const uint8_t> src, StdVector<uint8_t> &dst, const Compression comp);
bool decompress(Span<const uint8_t> src, StdVector<uint8_t> &dst);

} // namespace voxel::CompressedData

#endif // VOXEL_COMPRESSED_DATA_H
