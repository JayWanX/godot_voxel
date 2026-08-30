#ifndef VOXEL_STREAM_MEMORY_H
#define VOXEL_STREAM_MEMORY_H

#include "../constants/voxel_constants.h"
#include "../storage/voxel_buffer.h"
#include "../util/containers/fixed_array.h"
#include "../util/containers/span.h"
#include "../util/containers/std_unordered_map.h"
#include "../util/math/vector3i.h"
#include "../util/memory/memory.h"
#include "../util/thread/mutex.h"
#include "instance_data.h"
#include "voxel_stream.h"

namespace voxel {

// "fake" 数据流，只在内存中保存数据副本，而非保存到文件系统。可用于
// 测试。
class VoxelStreamMemory : public VoxelStream {
	GDCLASS(VoxelStreamMemory, VoxelStream)
public:
	// 批量加载体素数据块
	void load_voxel_blocks(Span<VoxelQueryData> p_blocks) override;
	// 批量保存体素数据块
	void save_voxel_blocks(Span<VoxelQueryData> p_blocks) override;
	// 加载单个体素数据块
	void load_voxel_block(VoxelQueryData &query_data) override;
	// 保存单个体素数据块
	void save_voxel_block(VoxelQueryData &query_data) override;

#ifdef VOXEL_ENABLE_INSTANCER
	// 是否支持实例块数据
	bool supports_instance_blocks() const override;
	// 批量加载实例块数据
	void load_instance_blocks(Span<InstancesQueryData> out_blocks) override;
	// 批量保存实例块数据
	void save_instance_blocks(Span<InstancesQueryData> p_blocks) override;
#endif

	// 是否支持一次性加载所有数据块
	bool supports_loading_all_blocks() const override;
	// 一次性加载所有数据块
	void load_all_blocks(FullLoadingResult &result) override;

	// 获取此数据流中可用的通道掩码
	int get_used_channels_mask() const override;

	// 获取数据块的细节层级（LOD）数量
	int get_lod_count() const override;

	// 设置人为保存延迟（微秒），用于模拟慢速存储以进行测试
	void set_artificial_save_latency_usec(int usec);
	// 获取人为保存延迟（微秒）
	int get_artificial_save_latency_usec() const;

private:
	static void _bind_methods();

	struct VoxelChunk {
		VoxelBuffer voxels;
		VoxelChunk() : voxels(VoxelBuffer::ALLOCATOR_POOL) {}
	};

	struct Lod {
		StdUnorderedMap<Vector3i, VoxelChunk> voxel_blocks;
		StdUnorderedMap<Vector3i, InstanceBlockData> instance_blocks;
		Mutex mutex;
	};

	FixedArray<Lod, constants::MAX_LOD> _lods;
	unsigned int _artificial_save_latency_usec = 0;
};

} // namespace voxel

#endif // VOXEL_STREAM_MEMORY_H
