#ifndef VOXEL_CUSTOM_METADATA_H
#define VOXEL_CUSTOM_METADATA_H

#include "../../util/containers/span.h"

namespace voxel {

// 自定义数据类型的基础接口。
class ICustomVoxelMetadata {
public:
	virtual ~ICustomVoxelMetadata() {}

	// 获取此元数据序列化后所占用的字节数。
	virtual size_t get_serialized_size() const = 0;

	// 将此元数据序列化到 `dst` 中。`dst` 的大小将等于或大于
	// `get_serialized_size()` 返回的大小。返回写入的字节数。
	virtual size_t serialize(Span<uint8_t> dst) const = 0;

	// 从给定字节反序列化此元数据。
	// 成功时返回 `true`，否则返回 `false`。`out_read_size` 必须赋值为读取的字节数。
	virtual bool deserialize(Span<const uint8_t> src, uint64_t &out_read_size) = 0;

	virtual ICustomVoxelMetadata *duplicate() = 0;

	// 返回元数据标记中使用的类型索引（主要用于调试检查）
	virtual uint8_t get_type_index() const = 0;

	virtual bool equals(const ICustomVoxelMetadata &other) const = 0;
};

} // namespace voxel

#endif // VOXEL_CUSTOM_METADATA_H
