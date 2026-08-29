#ifndef VOXEL_BUFFER_GD_H
#define VOXEL_BUFFER_GD_H

#include "../util/godot/classes/image.h"
#include "../util/godot/classes/ref_counted.h"
#include "../util/godot/core/array.h"
#include "../util/godot/core/typed_array.h"
#include "../util/macros.h"
#include "../util/math/vector3i.h"
#include "voxel_buffer.h"
#include <cstdint>
#include <memory>

// 无法前向声明，因为我们会使用 Image::Format
// VOXEL_GODOT_FORWARD_DECLARE(class Image)
VOXEL_GODOT_FORWARD_DECLARE(class ImageTexture3D)

namespace voxel {

class VoxelTool;

namespace godot {

// 面向脚本的 VoxelBuffer 包装器。
// 单独拆出来是因为作为 Godot 对象需要携带更多包袱，而且这种数据类型
// 会被实例化很多次却很少被脚本直接访问，所以把这一部分单独拿出来会更好
class VoxelBuffer : public RefCounted {
	GDCLASS(VoxelBuffer, RefCounted)

public:
	enum ChannelId {
		CHANNEL_TYPE = voxel::VoxelBuffer::CHANNEL_TYPE,
		CHANNEL_SDF = voxel::VoxelBuffer::CHANNEL_SDF,
		CHANNEL_COLOR = voxel::VoxelBuffer::CHANNEL_COLOR,
		CHANNEL_INDICES = voxel::VoxelBuffer::CHANNEL_INDICES,
		CHANNEL_WEIGHTS = voxel::VoxelBuffer::CHANNEL_WEIGHTS,
		CHANNEL_DATA5 = voxel::VoxelBuffer::CHANNEL_DATA5,
		CHANNEL_DATA6 = voxel::VoxelBuffer::CHANNEL_DATA6,
		CHANNEL_DATA7 = voxel::VoxelBuffer::CHANNEL_DATA7,
		MAX_CHANNELS = voxel::VoxelBuffer::MAX_CHANNELS,
	};

	enum ChannelMask {
		CHANNEL_TYPE_BIT = 1 << CHANNEL_TYPE,
		CHANNEL_SDF_BIT = 1 << CHANNEL_SDF,
		CHANNEL_COLOR_BIT = 1 << CHANNEL_COLOR,
		CHANNEL_INDICES_BIT = 1 << CHANNEL_INDICES,
		CHANNEL_WEIGHTS_BIT = 1 << CHANNEL_WEIGHTS,
		CHANNEL_DATA5_BIT = 1 << CHANNEL_DATA5,
		CHANNEL_DATA6_BIT = 1 << CHANNEL_DATA6,
		CHANNEL_DATA7_BIT = 1 << CHANNEL_DATA7,
		ALL_CHANNELS_MASK = (1 << MAX_CHANNELS) - 1,
	};

	// TODO 使用 C++17 inline 直接在这里初始化...
	static const char *CHANNEL_ID_HINT_STRING;

	enum Compression {
		COMPRESSION_NONE = voxel::VoxelBuffer::COMPRESSION_NONE,
		COMPRESSION_UNIFORM = voxel::VoxelBuffer::COMPRESSION_UNIFORM,
		// COMPRESSION_RLE,
		COMPRESSION_COUNT = voxel::VoxelBuffer::COMPRESSION_COUNT
	};

	enum Depth {
		DEPTH_8_BIT = voxel::VoxelBuffer::DEPTH_8_BIT,
		DEPTH_16_BIT = voxel::VoxelBuffer::DEPTH_16_BIT,
		DEPTH_32_BIT = voxel::VoxelBuffer::DEPTH_32_BIT,
		DEPTH_64_BIT = voxel::VoxelBuffer::DEPTH_64_BIT,
		DEPTH_COUNT = voxel::VoxelBuffer::DEPTH_COUNT
	};

	enum Allocator {
		ALLOCATOR_DEFAULT = voxel::VoxelBuffer::ALLOCATOR_DEFAULT,
		ALLOCATOR_POOL = voxel::VoxelBuffer::ALLOCATOR_POOL,
		ALLOCATOR_COUNT
	};

	// 设置显式上限是出于序列化原因，也因为必须有一个合理的限制
	static const uint32_t MAX_SIZE = 65535;

	// 构造一个新的缓冲区
	VoxelBuffer();
	VoxelBuffer(VoxelBuffer::Allocator allocator);
	// 引用现有的缓冲区
	VoxelBuffer(std::shared_ptr<voxel::VoxelBuffer> &other);

	~VoxelBuffer();

	// 由于 Godot 的限制，带参数的构造函数并不总能使用，因此采用变通方法
	static Ref<VoxelBuffer> create_shared(std::shared_ptr<voxel::VoxelBuffer> &other);

	inline const voxel::VoxelBuffer &get_buffer() const {
#ifdef DEBUG_ENABLED
		CRASH_COND(_buffer == nullptr);
#endif
		return *_buffer;
	}

	inline voxel::VoxelBuffer &get_buffer() {
#ifdef DEBUG_ENABLED
		CRASH_COND(_buffer == nullptr);
#endif
		return *_buffer;
	}

	inline std::shared_ptr<voxel::VoxelBuffer> get_buffer_shared() {
#ifdef DEBUG_ENABLED
		CRASH_COND(_buffer == nullptr);
#endif
		return _buffer;
	}

	// inline std::shared_ptr<voxel::VoxelBuffer> get_buffer_shared() { return _buffer; }

	Vector3i get_size() const {
		return _buffer->get_size();
	}

	void create(int x, int y, int z);
	void clear();

	uint64_t get_voxel(int x, int y, int z, unsigned int channel) const {
		return _buffer->get_voxel(x, y, z, channel);
	}
	void set_voxel(uint64_t value, int x, int y, int z, unsigned int channel) {
		_buffer->set_voxel(value, x, y, z, channel);
	}
	real_t get_voxel_f(int x, int y, int z, unsigned int channel_index) const;
	void set_voxel_f(real_t value, int x, int y, int z, unsigned int channel_index);

	uint64_t get_voxel_v(Vector3i pos, unsigned int channel) const {
		return _buffer->get_voxel(pos.x, pos.y, pos.z, channel);
	}
	void set_voxel_v(uint64_t value, Vector3i pos, unsigned int channel_index) {
		_buffer->set_voxel(value, pos.x, pos.y, pos.z, channel_index);
	}

	void copy_channel_from(Ref<VoxelBuffer> other, unsigned int channel);
	void copy_channel_from_area(
			Ref<VoxelBuffer> other,
			Vector3i src_min,
			Vector3i src_max,
			Vector3i dst_min,
			unsigned int channel
	);

	void fill(uint64_t defval, int channel_index = 0);
	void fill_f(real_t value, int channel = 0);
	void fill_area(uint64_t defval, Vector3i min, Vector3i max, unsigned int channel_index) {
		_buffer->fill_area(defval, min, max, channel_index);
	}
	void fill_area_f(real_t value, Vector3i min, Vector3i max, unsigned int channel_index) {
		_buffer->fill_area_f(value, min, max, channel_index);
	}

	bool is_uniform(int channel_index) const;

	void compress_uniform_channels();
	Compression get_channel_compression(int channel_index) const;
	void decompress_channel(int channel_index);

	void downscale_to(Ref<VoxelBuffer> dst, Vector3i src_min, Vector3i src_max, Vector3i dst_min) const;

	void rotate_90(Vector3i::Axis axis, int turns);
	void mirror(Vector3i::Axis axis);

	Ref<VoxelBuffer> duplicate(bool include_metadata) const;

	Ref<VoxelTool> get_voxel_tool();

	void set_channel_depth(unsigned int channel_index, Depth new_depth);
	Depth get_channel_depth(unsigned int channel_index) const;

	void remap_values(unsigned int channel_index, PackedInt32Array map);

	// 当使用低于 32 位的分辨率表示地形有符号距离场时，
	// 应对其进行缩放以更好地适配所表示值的范围，因为存储被归一化到 -1..1。
	// 该函数返回给定位深配置下的缩放比例。
	static float get_sdf_quantization_scale(Depth d);

	Allocator get_allocator() const;

	PackedByteArray get_channel_as_byte_array(const ChannelId channel) const;
	void set_channel_from_byte_array(const ChannelId channel, const PackedByteArray &pba);

	Ref<ImageTexture3D> create_3d_texture_from_sdf_zxy(const Image::Format output_format) const;
	void update_3d_texture_from_sdf_zxy(Ref<ImageTexture3D> texture) const;

	// 操作

	void op_add_buffer_f(Ref<VoxelBuffer> other, VoxelBuffer::ChannelId channel);
	void op_sub_buffer_f(Ref<VoxelBuffer> other, VoxelBuffer::ChannelId channel);
	void op_mul_buffer_f(Ref<VoxelBuffer> other, VoxelBuffer::ChannelId channel);
	void op_mul_value_f(float scale, VoxelBuffer::ChannelId channel);
	void op_min_buffer_f(Ref<VoxelBuffer> other, VoxelBuffer::ChannelId channel);
	void op_max_buffer_f(Ref<VoxelBuffer> other, VoxelBuffer::ChannelId channel);

	// 检查源缓冲区某通道的 float/SDF 值是否低于阈值，并根据该比较的结果
	// 将整数值写入目标缓冲区。
	void op_select_less_src_f_dst_i_values(
			Ref<VoxelBuffer> src_ref,
			const VoxelBuffer::ChannelId src_channel,
			const float threshold,
			const int value_if_less,
			const int value_if_more,
			const VoxelBuffer::ChannelId dst_channel
	);

	// 元数据

	Variant get_block_metadata() const;
	void set_block_metadata(Variant meta);

	Variant get_voxel_metadata(Vector3i pos) const;
	void set_voxel_metadata(Vector3i pos, Variant meta);

	void for_each_voxel_metadata(const Callable &callback) const;
	void for_each_voxel_metadata_in_area(const Callable &callback, Vector3i min_pos, Vector3i max_pos);
	void copy_voxel_metadata_in_area(
			Ref<VoxelBuffer> src_buffer,
			Vector3i src_min_pos,
			Vector3i src_max_pos,
			Vector3i dst_pos
	);

	void clear_voxel_metadata();
	void clear_voxel_metadata_in_area(Vector3i min_pos, Vector3i max_pos);

	// 调试

	Ref<Image> debug_print_sdf_to_image_top_down();
	static Ref<Image> debug_print_sdf_to_image_top_down(const voxel::VoxelBuffer &vb);
	TypedArray<Image> debug_print_sdf_y_slices(float scale) const;
	Ref<Image> debug_print_sdf_y_slice(float scale, int y) const;
	static Ref<Image> debug_print_sdf_y_slice(const voxel::VoxelBuffer &buffer, float scale, int y);
	static Ref<Image> debug_print_sdf_z_slice(const voxel::VoxelBuffer &buffer, float scale, int z);

private:
	// `create` 由 `GDCLASS` 定义，从而阻止直接绑定名为 `create` 的函数
	void _b_create(int x, int y, int z) {
		create(x, y, z);
	}

	static void _bind_methods();

	std::shared_ptr<voxel::VoxelBuffer> _buffer;
};

Variant get_voxel_metadata(const voxel::VoxelBuffer &vb, const Vector3i pos);
void set_voxel_metadata(voxel::VoxelBuffer &vb, const Vector3i pos, const Variant &meta);

} // namespace godot
} // namespace voxel

VARIANT_ENUM_CAST(voxel::godot::VoxelBuffer::ChannelId)
VARIANT_ENUM_CAST(voxel::godot::VoxelBuffer::ChannelMask)
VARIANT_ENUM_CAST(voxel::godot::VoxelBuffer::Depth)
VARIANT_ENUM_CAST(voxel::godot::VoxelBuffer::Compression)
VARIANT_ENUM_CAST(voxel::godot::VoxelBuffer::Allocator)

#endif // VOXEL_BUFFER_GD_H
