#ifndef VOXEL_STREAM_H
#define VOXEL_STREAM_H

#include "../constants/voxel_constants.h"
#include "../util/containers/span.h"
#include "../util/containers/std_vector.h"
#include "../util/godot/classes/resource.h"
#include "../util/math/box3i.h"
#include "../util/math/vector3.h"
#include "../util/math/vector3i.h"
#include "../util/memory/memory.h"
#include "../util/thread/rw_lock.h"
#include "compressed_data.h"
#include "voxel_block_serializer_gd.h"

#include <cstdint>

namespace voxel {

class VoxelBuffer;
#ifdef VOXEL_ENABLE_INSTANCER
struct InstanceBlockData;
#endif

namespace godot {
class VoxelBuffer;
}

// 提供对分页体素数据源的访问，可加载和保存。
// 该设计针对文件，因此可在单个后台线程中运行，并批量接收请求。
// 必须以线程安全的方式实现。
//
// 若你需要更专业的 API 以使用更多线程生成体素，请使用 VoxelGenerator。
//
class VoxelStream : public Resource {
	GDCLASS(VoxelStream, Resource)
public:
	static const int32_t DEFAULT_MIN_SUPPORTED_BLOCK_COORDINATE =
			-math::arithmetic_rshift(constants::MAX_VOLUME_EXTENT, constants::DEFAULT_BLOCK_SIZE_PO2);
	static const int32_t DEFAULT_MAX_SUPPORTED_BLOCK_COORDINATE =
			math::arithmetic_rshift(constants::MAX_VOLUME_EXTENT, constants::DEFAULT_BLOCK_SIZE_PO2);

	VoxelStream();
	~VoxelStream();

	enum ResultCode : uint8_t {
		// 出现错误，应中止该请求
		RESULT_ERROR,
		// 在数据流中找不到该数据块。请求方可以回退到生成器。
		RESULT_BLOCK_NOT_FOUND,
		// 已找到该数据块，因此请求方不会使用生成器。
		RESULT_BLOCK_FOUND,

		_RESULT_COUNT
	};

	struct VoxelQueryData {
		VoxelBuffer &voxel_buffer;
		Vector3i position_in_blocks;
		uint8_t lod_index;
		// 目前在保存查询中未使用该字段。也许应该使用？
		ResultCode result;
	};

#ifdef VOXEL_ENABLE_INSTANCER
	struct InstancesQueryData {
		UniquePtr<InstanceBlockData> data;
		Vector3i position_in_blocks;
		uint8_t lod_index;
		ResultCode result;
	};
#endif

	// TODO 弃用
	// 查询从给定世界坐标体素位置和 LOD 开始的一块体素。
	// 若使用 LOD，给定坐标处的结果必须始终保持一致，无论 LOD 如何。
	// 换言之，体素值必须仅取决于其坐标或固定参数。
	virtual void load_voxel_block(VoxelQueryData &query_data);

	// TODO 弃用
	virtual void save_voxel_block(VoxelQueryData &query_data);

	// 注意：不要修改 `p_blocks` 的顺序。
	virtual void load_voxel_blocks(Span<VoxelQueryData> p_blocks);

	// 向数据流返回多个体素数据块。
	// 若保存到文件，推荐使用此函数，因为可以批量访问。
	virtual void save_voxel_blocks(Span<VoxelQueryData> p_blocks);

#ifdef VOXEL_ENABLE_INSTANCER
	// TODO 将支持函数合并为带功能位掩码的单个 getter
	// 是否支持实例块数据
	virtual bool supports_instance_blocks() const;

	// 加载多个实例块数据
	virtual void load_instance_blocks(Span<InstancesQueryData> out_blocks);
	// 保存多个实例块数据
	virtual void save_instance_blocks(Span<InstancesQueryData> p_blocks);
#endif

	struct FullLoadingResult {
		// TODO 也许这需要解耦。并非所有体素数据块都有实例，反之亦然
		struct Block {
			std::shared_ptr<VoxelBuffer> voxels;
#ifdef VOXEL_ENABLE_INSTANCER
			UniquePtr<InstanceBlockData> instances_data;
#endif
			Vector3i position;
			unsigned int lod;
		};
		StdVector<Block> blocks;
	};

	// 是否支持一次性加载所有数据块
	virtual bool supports_loading_all_blocks() const {
		return false;
	}

	// 一次性加载所有数据块
	virtual void load_all_blocks(FullLoadingResult &result);

	// 告知此数据流中可找到哪些通道。
	// 最简单的实现是全部返回。
	// 指定可用通道的一个原因是帮助编辑器检测配置问题，
	// 并且若只想保存特定通道，可避免保存其他通道。
	virtual int get_used_channels_mask() const;

	// 获取此数据流将提供的数据块大小，以 2 的幂表示。
	// 文件数据流很可能强制规定特定的数据块大小，
	// 更改它可能代价高昂，因此 API 通常也是特定的
	virtual int get_block_size_po2() const;

	// 获取数据块可被查询的细节层级（LOD）数量。
	virtual int get_lod_count() const;

	// 获取此数据流支持的数据块坐标范围
	virtual Box3i get_supported_block_range() const;

	// 生成的数据块是否应立即保存？若不，它们将仅在修改后被保存。
	// 若启用，生成的数据块将立即被视为已编辑并保存到数据流。
	// 警告：这与修改器等非破坏性工作流不兼容。
	void set_save_generator_output(bool enabled);
	bool get_save_generator_output() const;

	// 若数据流不立即将数据写入文件系统（例如使用缓存批量处理 I/O），则强制
	// 写入所有待处理数据。
	// 若关心性能，则不应频繁调用此方法，因为会需要更多文件 I/O。可
	// 在需要立即写入全部数据时使用。注意，实现应当已在资源销毁或其配置改变时自动执行此操作。
	// 某些实现若无缓存，可能什么都不做。
	virtual void flush();

	// 设置压缩方式
	void set_compression_mode(const godot::VoxelBlockSerializer::Compression mode);
	// 获取压缩方式
	godot::VoxelBlockSerializer::Compression get_compression_mode() const;

	// 提示数据流的函数是否可以调用。主要用于脚本实现，以避免错误
	// 刷屏。
	virtual bool is_runnable() const;

protected:
	CompressedData::Compression _compression_mode = CompressedData::COMPRESSION_LZ4;

private:
	static void _bind_methods();

	ResultCode _b_load_voxel_block(Ref<godot::VoxelBuffer> out_buffer, Vector3i block_position, int lod_index);
	void _b_save_voxel_block(Ref<godot::VoxelBuffer> buffer, Vector3i block_position, int lod_index);
	int _b_get_used_channels_mask() const;
	Vector3 _b_get_block_size() const;

	struct Parameters {
		bool save_generator_output = false;
	};

	Parameters _parameters;
	RWLock _parameters_lock;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelStream::ResultCode);

#endif // VOXEL_STREAM_H
