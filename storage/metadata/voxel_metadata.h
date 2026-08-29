#ifndef VOXEL_METADATA_H
#define VOXEL_METADATA_H

#include "../../util/memory/memory.h"
// #include "../../util/non_copyable.h"
#include "../../util/containers/span.h"
#include <cstdint>

namespace voxel {

// 体素元数据是任意的、稀疏的数据，可以附加到特定体素上。
// 它并不是一种高效或快速的存储方式，而是一种针对特殊情况的通用方式。
// 例如，它可以用来存储文本、标签、物品栏内容，或附加到体素的其他复杂状态。
// 如果需要更频繁地存储较小数据，可以依靠数据通道。

class ICustomVoxelMetadata;

// 单个元数据实例的容器。它拥有数据。
class VoxelMetadata {
public:
	enum Type : uint8_t { //
		TYPE_EMPTY = 0,
		TYPE_U64 = 1,
		// 保留的预定义类型。

		TYPE_CUSTOM_BEGIN = 32,
		// 等于或大于此索引的类型将实现 `ICustomVoxelMetadata`。

		TYPE_APP_SPECIFIC_BEGIN = 40
		// 没有什么阻止注册低于此索引的自定义类型，但为了方便，它应被用于
		// 应用特定类型（即游戏特定类型）。较低的索引可用于引擎特定集成。
	};

	static const unsigned int CUSTOM_TYPES_MAX_COUNT = 256 - TYPE_CUSTOM_BEGIN;

	VoxelMetadata() {}

	VoxelMetadata(VoxelMetadata &&other) {
		_type = other._type;
		_data = other._data;
		other._type = TYPE_EMPTY;
	}

	~VoxelMetadata() {
		clear();
	}

	inline void operator=(VoxelMetadata &&other) {
		clear();
		_type = other._type;
		_data = other._data;
		other._type = TYPE_EMPTY;
	}

	void clear();

	inline uint8_t get_type() const {
		return _type;
	}

	void set_u64(const uint64_t &v);
	uint64_t get_u64() const;

	void set_custom(uint8_t type, ICustomVoxelMetadata *custom_data);
	ICustomVoxelMetadata &get_custom();
	const ICustomVoxelMetadata &get_custom() const;

	// 清除此元数据，并使其成为给定元数据的副本。
	void copy_from(const VoxelMetadata &src);

	bool equals(const VoxelMetadata &other) const;

private:
	union Data {
		uint64_t u64_data;
		ICustomVoxelMetadata *custom_data;
	};

	uint8_t _type = TYPE_EMPTY;
	Data _data;
};

} // namespace voxel

#endif // VOXEL_METADATA_H
