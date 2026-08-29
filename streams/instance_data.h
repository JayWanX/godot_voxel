#ifndef VOXEL_INSTANCE_DATA_H
#define VOXEL_INSTANCE_DATA_H

#include "../util/containers/span.h"
#include "../util/containers/std_vector.h"
#include "../util/math/transform3f.h"

namespace voxel {

// 存储用于传递的数据，直到它被保存或转换为实际的实例
struct InstanceBlockData {
	struct InstanceData {
		// 实例的变换，相对于数据块的原点。
		Transform3f transform;
	};

	enum VoxelInstanceFormat {
		// 位置为有损压缩，基于区块的大小
		// - uint16_t x;
		// - uint16_t y;
		// - uint16_t z;
		//
		// 缩放是均匀的，被有损压缩为 256 个值
		// - uint8_t scale;
		//
		// 旋转为一个压缩的四元数，其各分量被朴素地量化为 256 个值
		// - uint8_t x;
		// - uint8_t y;
		// - uint8_t z;
		// - uint8_t w;
		FORMAT_SIMPLE_11B_V1 = 0
	};

	static const int POSITION_RESOLUTION = 65536;
	// 因为位置被量化，所以我们需要它的范围；但该范围不能为零，因此可能被钳制到此值。
	static const float POSITION_RANGE_MINIMUM;

	static const int SIMPLE_11B_V1_SCALE_RESOLUTION = 256;
	static const int SIMPLE_11B_V1_QUAT_RESOLUTION = 256;
	// 因为缩放被量化，所以我们需要它的范围；但该范围不能为零，因此可能被钳制到此值。
	static const float SIMPLE_11B_V1_SCALE_RANGE_MINIMUM;

	struct LayerData {
		uint16_t id;
		float scale_min;
		float scale_max;
		StdVector<InstanceData> instances;
	};

	float position_range;
	StdVector<LayerData> layers;

	void copy_to(InstanceBlockData &dst) const {
		// 它们都是 POD 类型，所以目前这样应该可行
		dst = *this;
	}
};

bool serialize_instance_block_data(const InstanceBlockData &src, StdVector<uint8_t> &dst);
bool deserialize_instance_block_data(InstanceBlockData &dst, Span<const uint8_t> src);

} // namespace voxel

#endif // VOXEL_INSTANCE_DATA_H
