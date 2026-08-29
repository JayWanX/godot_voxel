#ifndef VOXEL_METADATA_FACTORY_H
#define VOXEL_METADATA_FACTORY_H

#include "../../util/containers/fixed_array.h"
#include "voxel_metadata.h"
#include <cstdint>

namespace voxel {

// 自定义元数据类型的注册表，用于从已保存数据反序列化它们。
class VoxelMetadataFactory {
public:
	typedef ICustomVoxelMetadata *(*ConstructorFunc)();

	static VoxelMetadataFactory &get_singleton();

	VoxelMetadataFactory();

	// 注册一个自定义元数据类型。
	// 你选择的 `type` 应长期保持不变。
	// 它会用于存档文件中，因此更改它可能会破坏旧的存档。
	// `type` 必须大于或等于 `VoxelMetadata::TYPE_CUSTOM_BEGIN`。
	void add_constructor(uint8_t type, ConstructorFunc ctor);

	template <typename T>
	void add_constructor_by_type(uint8_t type) {
		add_constructor(type, []() { //
			// 如果我直接返回新建的实例，就无法编译
			ICustomVoxelMetadata *c = VOXEL_NEW(T);
			return c;
		});
	}

	void remove_constructor(uint8_t type);

	// 从给定类型 ID 构造一个自定义元数据类型。
	// `type` 必须大于或等于 `VoxelMetadata::TYPE_CUSTOM_BEGIN`。
	// 如果无法构造该类型，则返回 `nullptr`。
	ICustomVoxelMetadata *try_construct(uint8_t type) const;

private:
	FixedArray<ConstructorFunc, VoxelMetadata::CUSTOM_TYPES_MAX_COUNT> _constructors;
};

} // namespace voxel

#endif // VOXEL_METADATA_FACTORY_H
