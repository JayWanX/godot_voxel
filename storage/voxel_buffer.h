#ifndef VOXEL_BUFFER_INTERNAL_H
#define VOXEL_BUFFER_INTERNAL_H

#include "../util/containers/fixed_array.h"
#include "../util/containers/flat_map.h"
#include "../util/containers/small_vector.h"
#include "../util/math/box3i.h"
#include "../util/math/ortho_basis.h"
#include "funcs.h"
#include "metadata/voxel_metadata.h"

#include <limits>

namespace voxel {

class DynamicBitset;

struct VoxelFormat;

static_assert(sizeof(uint32_t) == sizeof(float), "uint32_t and float cannot be marshalled back and forth");
static_assert(sizeof(uint64_t) == sizeof(double), "uint64_t and double cannot be marshalled back and forth");

union MarshallFloat {
	float f;
	uint32_t i;
};

union MarshallDouble {
	double d;
	uint64_t l;
};

// 稠密体素数据存储。
// 按可配置位深的通道组织。
// 值既可以解释为无符号整数，也可以解释为归一化浮点数。
class VoxelBuffer {
public:
	enum ChannelId {
		CHANNEL_TYPE = 0,
		CHANNEL_SDF,
		CHANNEL_COLOR,
		CHANNEL_INDICES,
		CHANNEL_WEIGHTS,
		CHANNEL_DATA5,
		CHANNEL_DATA6,
		CHANNEL_DATA7,
		// 任意值，8 应该足够。可根据需要调整。
		MAX_CHANNELS
	};

	static const char *get_channel_name(const ChannelId id);

	static const int ALL_CHANNELS_MASK = 0xff;

	enum Compression : uint8_t {
		COMPRESSION_NONE = 0,
		COMPRESSION_UNIFORM, // 即"未分配任何体素"
		COMPRESSION_COUNT
	};

	enum Depth : uint8_t { //
		DEPTH_8_BIT,
		DEPTH_16_BIT,
		DEPTH_32_BIT,
		DEPTH_64_BIT,
		DEPTH_COUNT
	};

	enum Allocator : uint8_t { //
		// 通用分配器。malloc，Godot 的默认分配器。缓冲销毁时释放。
		ALLOCATOR_DEFAULT,
		// VoxelMemoryPool。应当更快但会一直占用内存。若运行时频繁创建大小相近的缓冲，优先选用。
		// 不要用于大型、低频的分配，也不要在编辑器中用，以免囤积内存。
		ALLOCATOR_POOL,
		ALLOCATOR_COUNT
	};

	static inline uint32_t get_depth_byte_count(VoxelBuffer::Depth d) {
		VOXEL_ASSERT(d >= 0 && d < VoxelBuffer::DEPTH_COUNT);
		return 1 << d;
	}

	static inline uint32_t get_depth_bit_count(Depth d) {
		// CRASH_COND(d < 0 || d >= VoxelBuffer::DEPTH_COUNT);
		return get_depth_byte_count(d) << 3;
	}

	static inline Depth get_depth_from_size(size_t size) {
		switch (size) {
			case 1:
				return DEPTH_8_BIT;
			case 2:
				return DEPTH_16_BIT;
			case 4:
				return DEPTH_32_BIT;
			case 8:
				return DEPTH_64_BIT;
			default:
				VOXEL_CRASH();
		}
		return DEPTH_COUNT;
	}

	static inline uint64_t real_to_raw_voxel(const real_t value, const Depth depth) {
		switch (depth) {
			case DEPTH_8_BIT:
				return snorm_to_s8(value * constants::QUANTIZED_SDF_8_BITS_SCALE);

			case DEPTH_16_BIT:
				return snorm_to_s16(value * constants::QUANTIZED_SDF_16_BITS_SCALE);

			case DEPTH_32_BIT: {
				MarshallFloat m;
				m.f = value;
				return m.i;
			}
			case DEPTH_64_BIT: {
				MarshallDouble m;
				m.d = value;
				return m.l;
			}
			default:
#ifdef DEV_ENABLED
				CRASH_NOW();
#endif
				return 0;
		}
	}

	static inline real_t raw_voxel_to_real(const uint64_t value, const Depth depth) {
		// 低于 32 的位深在 -1 到 1 之间归一化
		switch (depth) {
			case DEPTH_8_BIT:
				return s8_to_snorm(value) * constants::QUANTIZED_SDF_8_BITS_SCALE_INV;

			case DEPTH_16_BIT:
				return s16_to_snorm(value) * constants::QUANTIZED_SDF_16_BITS_SCALE_INV;

			case DEPTH_32_BIT: {
				MarshallFloat m;
				m.i = value;
				return m.f;
			}

			case DEPTH_64_BIT: {
				MarshallDouble m;
				m.l = value;
				return m.d;
			}

			default:
#ifdef DEV_ENABLED
				CRASH_NOW();
#endif
				return 0;
		}
	}

	static const Depth DEFAULT_CHANNEL_DEPTH = DEPTH_8_BIT;
	static const Depth DEFAULT_TYPE_CHANNEL_DEPTH = DEPTH_16_BIT;
	static const Depth DEFAULT_SDF_CHANNEL_DEPTH = DEPTH_16_BIT;
	static const Depth DEFAULT_INDICES_CHANNEL_DEPTH = DEPTH_16_BIT;
	static const Depth DEFAULT_WEIGHTS_CHANNEL_DEPTH = DEPTH_16_BIT;

	// 设置显式上限是出于序列化原因，也因为必须有一个合理的限制
	static const uint32_t MAX_SIZE = 65535;

	struct Channel {
		union {
			// 当通道被填充数据时分配。
			// 一维数组，顺序为 [z][x][y]，因为它允许更快的竖直方向访问（引擎为 Y 轴朝上）。
			uint8_t *data;

			// 当通道未填充数据时的默认值。
			// 这是一个编码后的值，因此可以通过转换获得非整数值。
			uint64_t defval;
		};

		Depth depth = DEFAULT_CHANNEL_DEPTH;
		Compression compression = COMPRESSION_UNIFORM;
		// [...] 2 个未使用字节

		// 在单个缓冲区中存储数 GB 数据既不受支持也不切实际。
		uint32_t size_in_bytes = 0;

		static const size_t MAX_SIZE_IN_BYTES = std::numeric_limits<uint32_t>::max();
	};

	// VoxelBuffer();
	VoxelBuffer(Allocator allocator);
	VoxelBuffer(VoxelBuffer &&src);

	~VoxelBuffer();

	VoxelBuffer &operator=(VoxelBuffer &&src);

	void create(unsigned int sx, unsigned int sy, unsigned int sz, const VoxelFormat *new_format = nullptr);
	void create(const Vector3i size, const VoxelFormat *new_format = nullptr);

	void clear(const VoxelFormat *new_format = nullptr);
	void clear_channel(unsigned int channel_index, uint64_t clear_value);
	void clear_channel_f(unsigned int channel_index, real_t clear_value);

	bool has_format(const VoxelFormat &p_format) const;

	inline Allocator get_allocator() const {
		return _allocator;
	}

	inline const Vector3i &get_size() const {
		return _size;
	}

	static uint64_t get_default_raw_value(const VoxelBuffer::ChannelId channel, const VoxelBuffer::Depth depth);
	static uint64_t get_default_sdf_raw_value(const Depth depth);
	static float get_default_sdf_value(const Depth depth);
	static uint64_t get_default_indices_raw_value(const Depth depth);

	uint64_t get_voxel(int x, int y, int z, unsigned int channel_index) const;
	void set_voxel(uint64_t value, int x, int y, int z, unsigned int channel_index);

	real_t get_voxel_f(int x, int y, int z, unsigned int channel_index) const;
	inline real_t get_voxel_f(Vector3i pos, unsigned int channel_index) const {
		return get_voxel_f(pos.x, pos.y, pos.z, channel_index);
	}
	void set_voxel_f(real_t value, int x, int y, int z, unsigned int channel_index);
	inline void set_voxel_f(real_t value, Vector3i pos, unsigned int channel_index) {
		set_voxel_f(value, pos.x, pos.y, pos.z, channel_index);
	}

	inline uint64_t get_voxel(const Vector3i pos, unsigned int channel_index) const {
		return get_voxel(pos.x, pos.y, pos.z, channel_index);
	}
	inline void set_voxel(int value, const Vector3i pos, unsigned int channel_index) {
		set_voxel(value, pos.x, pos.y, pos.z, channel_index);
	}

	void fill(uint64_t defval, unsigned int channel_index);
	void fill_area(uint64_t defval, Vector3i min, Vector3i max, unsigned int channel_index);
	void fill_area_f(float fvalue, Vector3i min, Vector3i max, unsigned int channel_index);
	void fill_f(real_t value, unsigned int channel);

	bool is_uniform(unsigned int channel_index) const;

	void compress_uniform_channels();
	void decompress_channel(unsigned int channel_index);
	Compression get_channel_compression(unsigned int channel_index) const;

	static size_t get_size_in_bytes_for_volume(Vector3i size, Depth depth);

	void copy_format(const VoxelBuffer &other);

	// 专用复制函数。
	// 注意：这些函数刻意不包含元数据。
	// 如果你还想复制元数据，请使用专门的处理函数。
	void copy_channels_from(const VoxelBuffer &other);
	void copy_channel_from(const VoxelBuffer &other, unsigned int channel_index);
	void copy_channel_from(
			const VoxelBuffer &other,
			Vector3i src_min,
			Vector3i src_max,
			Vector3i dst_min,
			unsigned int channel_index
	);

	// 从以原始数组形式传入的值盒中复制一个区域。
	// `src_size` 是源盒的完整三维尺寸。
	// `src_min` 和 `src_max` 是该盒中我们要复制的子区域。
	// `dst_min` 是我们要将数据复制到目标中的较低角。
	template <typename T>
	void copy_channel_from(
			Span<const T> src,
			Vector3i src_size,
			Vector3i src_min,
			Vector3i src_max,
			Vector3i dst_min,
			unsigned int channel_index
	) {
		VOXEL_ASSERT_RETURN(channel_index < MAX_CHANNELS);

		Channel &channel = _channels[channel_index];
#ifdef DEBUG_ENABLED
		// 源和目标值的大小必须匹配
		VOXEL_ASSERT_RETURN(channel.depth == get_depth_from_size(sizeof(T)));
#endif

		// 此函数总是对目标进行解压。
		// 若要保持压缩状态，要么先检查你要复制的数据，
		// 要么计划之后重新压缩。
		decompress_channel(channel_index);

		Span<T> dst = Span<uint8_t>(channel.data, channel.size_in_bytes).reinterpret_cast_to<T>();
		copy_3d_region_zxy<T>(dst, _size, dst_min, src, src_size, src_min, src_max);
	}

	// 将数据的某个区域复制到稠密缓冲区。
	// 若源已压缩，则先解压。
	// `dst` 是存储盒中网格值的原始数组。
	// `dst_size` 是盒的总尺寸。
	// `dst_min` 是我们希望存储源数据的较低角。
	// `src_min` 和 `src_max` 是我们要复制的源子区域。
	template <typename T>
	void copy_channel_to(
			Span<T> dst,
			Vector3i dst_size,
			Vector3i dst_min,
			Vector3i src_min,
			Vector3i src_max,
			unsigned int channel_index
	) const {
		VOXEL_ASSERT_RETURN(channel_index < MAX_CHANNELS);

		const Channel &channel = _channels[channel_index];
#ifdef DEBUG_ENABLED
		// 源和目标值的大小必须匹配
		VOXEL_ASSERT_RETURN(channel.depth == get_depth_from_size(sizeof(T)));
#endif

		if (channel.compression == COMPRESSION_UNIFORM) {
			fill_3d_region_zxy<T>(dst, dst_size, dst_min, dst_min + (src_max - src_min), channel.defval);
		} else {
			Span<const T> src(static_cast<const T *>(channel.data), channel.size_in_bytes / sizeof(T));
			copy_3d_region_zxy<T>(dst, dst_size, dst_min, src, _size, src_min, src_max);
		}
	}

	// TODO 已弃用？
	// 对与缓冲区相交的给定盒中的所有单元格执行读写操作。
	// `action_func` 从通道接收体素值，并返回修改后的值。
	// 如果返回的值不同，它将被应用到缓冲区。
	// 可用于混合体素。
	template <typename F>
	inline void read_write_action(Box3i box, unsigned int channel_index, F action_func) {
		VOXEL_ASSERT_RETURN(channel_index < MAX_CHANNELS);

		box.clip(Box3i(Vector3i(), _size));
		const Vector3i min_pos = box.position;
		const Vector3i max_pos = box.position + box.size;
		Vector3i pos;
		for (pos.z = min_pos.z; pos.z < max_pos.z; ++pos.z) {
			for (pos.x = min_pos.x; pos.x < max_pos.x; ++pos.x) {
				for (pos.y = min_pos.y; pos.y < max_pos.y; ++pos.y) {
					// TODO 优化：可以跳过一堆检查和分支
					const uint64_t v0 = get_voxel(pos, channel_index);
					const uint64_t v1 = action_func(pos, v0);
					if (v0 != v1) {
						set_voxel(v1, pos, channel_index);
					}
				}
			}
		}
	}

	static inline size_t get_index(const Vector3i pos, const Vector3i size) {
		return Vector3iUtil::get_zxy_index(pos, size);
	}

	inline size_t get_index(unsigned int x, unsigned int y, unsigned int z) const {
		return y + _size.y * (x + _size.x * z); // ZXY 索引
	}

	template <typename F>
	inline void for_each_index_and_pos(const Box3i &box, F f) {
		const Vector3i min_pos = box.position;
		const Vector3i max_pos = box.position + box.size;
		Vector3i pos;
		for (pos.z = min_pos.z; pos.z < max_pos.z; ++pos.z) {
			for (pos.x = min_pos.x; pos.x < max_pos.x; ++pos.x) {
				pos.y = min_pos.y;
				size_t i = get_index(pos.x, pos.y, pos.z);
				for (; pos.y < max_pos.y; ++pos.y) {
					f(i, pos);
					++i;
				}
			}
		}
	}

	// Data_T action_func(Vector3i pos, Data_T in_v)
	template <typename F, typename Data_T>
	void write_box_template(const Box3i &box, unsigned int channel_index, F action_func, Vector3i offset) {
		decompress_channel(channel_index);
		Channel &channel = _channels[channel_index];
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT_RETURN(Box3i(Vector3i(), _size).contains(box));
		VOXEL_ASSERT_RETURN(get_depth_byte_count(channel.depth) == sizeof(Data_T));
#endif
		Span<Data_T> data = Span<uint8_t>(channel.data, channel.size_in_bytes).reinterpret_cast_to<Data_T>();
		// 需要 `&` 是因为 lambda 捕获默认为 `const`，而 `mutable` 只能从 C++23 开始使用
		for_each_index_and_pos(box, [&data, action_func, offset](size_t i, Vector3i pos) {
			// 这不需要操作使用完全相同的类型，此处可以发生转换。
			data.set(i, action_func(pos + offset, data[i]));
		});
		compress_if_uniform(channel);
	}

	// void action_func(Vector3i pos, Data0_T &inout_v0, Data1_T &inout_v1)
	template <typename F, typename Data0_T, typename Data1_T>
	void write_box_2_template(
			const Box3i &box,
			unsigned int channel_index0,
			unsigned int channel_index1,
			F action_func,
			Vector3i offset
	) {
		decompress_channel(channel_index0);
		decompress_channel(channel_index1);
		Channel &channel0 = _channels[channel_index0];
		Channel &channel1 = _channels[channel_index1];
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT_RETURN(Box3i(Vector3i(), _size).contains(box));
		VOXEL_ASSERT_RETURN(get_depth_byte_count(channel0.depth) == sizeof(Data0_T));
		VOXEL_ASSERT_RETURN(get_depth_byte_count(channel1.depth) == sizeof(Data1_T));
#endif
		Span<Data0_T> data0 = Span<uint8_t>(channel0.data, channel0.size_in_bytes).reinterpret_cast_to<Data0_T>();
		Span<Data1_T> data1 = Span<uint8_t>(channel1.data, channel1.size_in_bytes).reinterpret_cast_to<Data1_T>();
		for_each_index_and_pos(box, [action_func, offset, &data0, &data1](size_t i, Vector3i pos) {
			// TODO 调用方仍必须指定完全正确的类型，也许可以使用某种转换
			action_func(pos + offset, data0[i], data1[i]);
		});
		compress_if_uniform(channel0);
		compress_if_uniform(channel1);
	}

	template <typename F>
	void write_box(const Box3i &box, unsigned int channel_index, F action_func, Vector3i offset) {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT_RETURN(channel_index < MAX_CHANNELS);
#endif
		const Channel &channel = _channels[channel_index];
		switch (channel.depth) {
			case DEPTH_8_BIT:
				write_box_template<F, uint8_t>(box, channel_index, action_func, offset);
				break;
			case DEPTH_16_BIT:
				write_box_template<F, uint16_t>(box, channel_index, action_func, offset);
				break;
			case DEPTH_32_BIT:
				write_box_template<F, uint32_t>(box, channel_index, action_func, offset);
				break;
			case DEPTH_64_BIT:
				write_box_template<F, uint64_t>(box, channel_index, action_func, offset);
				break;
			default:
				VOXEL_PRINT_ERROR("Unknown channel");
				break;
		}
	}

	/*template <typename F>
	void write_box_2(const Box3i &box, unsigned int channel_index0, unsigned int channel_index1, F action_func,
			Vector3i offset) {
#ifdef DEBUG_ENABLED
		ERR_FAIL_INDEX(channel_index0, MAX_CHANNELS);
		ERR_FAIL_INDEX(channel_index1, MAX_CHANNELS);
#endif
		const Channel &channel0 = _channels[channel_index0];
		const Channel &channel1 = _channels[channel_index1];
#ifdef DEBUG_ENABLED
		// TODO 想办法更好地处理组合爆炸。目前只允许实际用到的组合。
		ERR_FAIL_COND_MSG(channel1.depth != DEPTH_16_BIT, "Second channel depth is hardcoded to 16 for now");
#endif
		switch (channel.depth) {
			case DEPTH_8_BIT:
				write_box_2_template<F, uint8_t, uint16_t>(box, channel_index0, channel_index1, action_func, offset);
				break;
			case DEPTH_16_BIT:
				write_box_2_template<F, uint16_t, uint16_t>(box, channel_index0, channel_index1, action_func, offset);
				break;
			case DEPTH_32_BIT:
				write_box_2_template<F, uint32_t, uint16_t>(box, channel_index0, channel_index1, action_func, offset);
				break;
			case DEPTH_64_BIT:
				write_box_2_template<F, uint64_t, uint16_t>(box, channel_index0, channel_index1, action_func, offset);
				break;
			default:
				ERR_FAIL();
				break;
		}
	}*/

	static inline SmallVector<uint8_t, MAX_CHANNELS> mask_to_channels_list(uint8_t channels_mask) {
		SmallVector<uint8_t, MAX_CHANNELS> channels;
		for (unsigned int channel_index = 0; channel_index < VoxelBuffer::MAX_CHANNELS; ++channel_index) {
			if (((1 << channel_index) & channels_mask) != 0) {
				channels.push_back(channel_index);
			}
		}
		return channels;
	}

	void copy_to(VoxelBuffer &dst, bool include_metadata) const;
	void move_to(VoxelBuffer &dst);

	inline bool is_position_valid(unsigned int x, unsigned int y, unsigned int z) const {
		return x < (unsigned)_size.x && y < (unsigned)_size.y && z < (unsigned)_size.z;
	}

	inline bool is_position_valid(const Vector3i pos) const {
		return is_position_valid(pos.x, pos.y, pos.z);
	}

	inline bool is_box_valid(const Box3i box) const {
		return Box3i(Vector3i(), _size).contains(box);
	}

	inline uint64_t get_volume() const {
		return Vector3iUtil::get_volume_u64(_size);
	}

	// 获取一个别名到通道数据的切片
	bool get_channel_as_bytes(unsigned int channel_index, Span<uint8_t> &slice);

	// 获取一个只读别名到通道数据的切片
	bool get_channel_as_bytes_read_only(unsigned int channel_index, Span<const uint8_t> &slice) const;

	// 获取一个别名到通道数据的切片，并将其重新解释为特定类型
	template <typename T>
	bool get_channel_data(unsigned int channel_index, Span<T> &dst) {
		Span<uint8_t> dst8;
		VOXEL_ASSERT_RETURN_V(get_channel_as_bytes(channel_index, dst8), false);
		dst = dst8.reinterpret_cast_to<T>();
		return true;
	}

	// 获取一个只读别名到通道数据的切片，并将其重新解释为特定类型
	template <typename T>
	bool get_channel_data_read_only(unsigned int channel_index, Span<const T> &dst) const {
		Span<const uint8_t> dst8;
		VOXEL_ASSERT_RETURN_V(get_channel_as_bytes_read_only(channel_index, dst8), false);
		dst = dst8.reinterpret_cast_to<const T>();
		return true;
	}

	// 用原始数据覆盖通道内容。这会跳过通道的默认初始化，因此
	// 可能比使用 `decompress_channel` 稍快一些。输入数据必须具有正确的大小。
	void set_channel_from_bytes(const unsigned int channel_index, Span<const uint8_t> src);

	void downscale_to(VoxelBuffer &dst, Vector3i src_min, Vector3i src_max, Vector3i dst_min) const;

	bool equals(const VoxelBuffer &p_other) const;

	void set_channel_depth(unsigned int channel_index, Depth new_depth);
	Depth get_channel_depth(unsigned int channel_index) const;

	// 当使用低于 32 位的分辨率表示地形有符号距离场时，
	// 应对其进行缩放以更好地适配所表示值的范围，因为存储被归一化到 -1..1。
	// 该函数返回给定位深配置下的缩放比例。
	static float get_sdf_quantization_scale(Depth d);

	void get_range_f(float &out_min, float &out_max, ChannelId channel_index) const;

	void transform(const math::OrthoBasis &basis);

	// 元数据

	VoxelMetadata &get_block_metadata() {
		return _block_metadata;
	}
	const VoxelMetadata &get_block_metadata() const {
		return _block_metadata;
	}

	const VoxelMetadata *get_voxel_metadata(Vector3i pos) const;
	VoxelMetadata *get_voxel_metadata(Vector3i pos);
	VoxelMetadata *get_or_create_voxel_metadata(Vector3i pos);
	void erase_voxel_metadata(Vector3i pos);

	void clear_and_set_voxel_metadata(Span<FlatMapMoveOnly<Vector3i, VoxelMetadata>::Pair> pairs);

	template <typename F>
	void for_each_voxel_metadata_in_area(Box3i box, F callback) const {
		// TODO 对于 `find` 和这类迭代，我们可能想把 FlatMap 内部存储中的键和值分开，以减少缓存未命中
		for (FlatMapMoveOnly<Vector3i, VoxelMetadata>::ConstIterator it = _voxel_metadata.begin();
			 it != _voxel_metadata.end();
			 ++it) {
			if (box.contains(it->key)) {
				callback(it->key, it->value);
			}
		}
	}

	template <typename F>
	inline void erase_voxel_metadata_if(F predicate) {
		_voxel_metadata.remove_if(predicate);
	}

	// #ifdef VOXEL_GODOT
	// 	// TODO 移到别处
	// 	void for_each_voxel_metadata(const Callable &callback) const;
	// 	void for_each_voxel_metadata_in_area(const Callable &callback, Box3i box) const;
	// #endif

	void clear_voxel_metadata();
	void clear_voxel_metadata_in_area(const Box3i box);
	void copy_voxel_metadata_in_area(const VoxelBuffer &src_buffer, const Box3i src_box, const Vector3i dst_origin);
	void copy_voxel_metadata(const VoxelBuffer &src_buffer);

	const FlatMapMoveOnly<Vector3i, VoxelMetadata> &get_voxel_metadata() const {
		return _voxel_metadata;
	}

#ifdef VOXEL_TESTS
	void check_voxel_metadata_integrity() const;
#endif

private:
	void init_channel_defaults();
	bool create_channel_noinit(int i, Vector3i size);
	bool create_channel(int i, uint64_t defval);
	void delete_channel(int i);
	void compress_if_uniform(Channel &channel);
	static void delete_channel(Channel &channel, Allocator allocator);
	static void clear_channel(Channel &channel, uint64_t clear_value, Allocator allocator);
	static bool is_uniform(const Channel &channel);

private:
	// 每个通道都可以存储任意数据。
	// 例如，你可以决定存储颜色（R、G、B、A）、玩法类型（类型、状态、光照）或两者兼有。
	FixedArray<Channel, MAX_CHANNELS> _channels;

	// 三个方向上各有多少体素。所有已填充的通道大小相同。
	Vector3i _size;

	// 需要存储单个体素时使用哪个分配器。
	// 默认分配器最不容易被误用，但不一定是最快的。
	Allocator _allocator = ALLOCATOR_DEFAULT;

	// TODO 能否将元数据从 VoxelBuffer 中分离出来？
	VoxelMetadata _block_metadata;
	// 该元数据预期是稀疏的，条目数量很少。
	FlatMapMoveOnly<Vector3i, VoxelMetadata> _voxel_metadata;
};

void get_unscaled_sdf(const VoxelBuffer &voxels, Span<float> sdf);
void scale_and_store_sdf(VoxelBuffer &voxels, Span<float> sdf);
void scale_and_store_sdf_if_modified(VoxelBuffer &voxels, Span<float> sdf, Span<const float> comparand);

void paste(
		Span<const uint8_t> channels,
		const VoxelBuffer &src_buffer,
		VoxelBuffer &dst_buffer,
		const Vector3i dst_base_pos,
		bool with_metadata
);

// 当源不为某个特定值时才粘贴
void paste_src_masked(
		Span<const uint8_t> channels,
		const VoxelBuffer &src_buffer,
		unsigned int src_mask_channel,
		uint64_t src_mask_value,
		VoxelBuffer &dst_buffer,
		const Vector3i dst_base_pos,
		bool with_metadata
);

// 当源不为某个特定值、且目标为某个特定值时才粘贴
void paste_src_masked_dst_writable_value(
		Span<const uint8_t> channels,
		const VoxelBuffer &src_buffer,
		unsigned int src_mask_channel,
		uint64_t src_mask_value,
		VoxelBuffer &dst_buffer,
		const Vector3i dst_base_pos,
		unsigned int dst_mask_channel,
		uint64_t dst_mask_value,
		bool with_metadata
);

// 当源不为某个特定值、且指定的位集包含目标值时才粘贴
void paste_src_masked_dst_writable_bitarray(
		Span<const uint8_t> channels,
		const VoxelBuffer &src_buffer,
		unsigned int src_mask_channel,
		uint64_t src_mask_value,
		VoxelBuffer &dst_buffer,
		const Vector3i dst_base_pos,
		unsigned int dst_mask_channel,
		const DynamicBitset &bitarray,
		bool with_metadata
);

} // namespace voxel

#endif // VOXEL_BUFFER_INTERNAL_H
