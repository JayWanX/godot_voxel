#ifndef VOXEL_METADATA_VARIANT_H
#define VOXEL_METADATA_VARIANT_H

#include "../../util/godot/core/variant.h"
#include "custom_voxel_metadata.h"
#include "voxel_metadata.h"

namespace voxel::godot {

// TODO 不确定这是否应该作为自定义类型。自定义类型本应针对特定游戏？
enum GodotMetadataTypes { //
	METADATA_TYPE_VARIANT = VoxelMetadata::TYPE_CUSTOM_BEGIN
};

// 持有 Godot Variant 的自定义元数据（基本上是 Godot 引擎认识的任何东西）。
// 可序列化性遵循与 Godot 的 `encode_variant` 相同的规则：无无效对象、无循环引用。
class VoxelMetadataVariant : public ICustomVoxelMetadata {
public:
	Variant data;

	size_t get_serialized_size() const override;
	size_t serialize(Span<uint8_t> dst) const override;
	bool deserialize(Span<const uint8_t> src, uint64_t &out_read_size) override;
	ICustomVoxelMetadata *duplicate() override;
	uint8_t get_type_index() const override;
	bool equals(const ICustomVoxelMetadata &other) const override;
};

Variant get_as_variant(const VoxelMetadata &meta);
void set_as_variant(VoxelMetadata &meta, const Variant &v);

} // namespace voxel::godot

#endif // VOXEL_METADATA_VARIANT_H
