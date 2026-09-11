#ifndef REGION_FILE_H
#define REGION_FILE_H

#include "../../storage/voxel_buffer.h"
#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/file_access.h"
#include "../../util/math/color8.h"
#include <core/math/vector3i.h>
#include "../compressed_data.h"

namespace voxel {

struct RegionFormat {
	static const char *FILE_EXTENSION;
	static const uint32_t MAX_BLOCKS_ACROSS = 255;
	static const uint32_t CHANNEL_COUNT = 8;

	static_assert(CHANNEL_COUNT == VoxelBuffer::MAX_CHANNELS, "This format doesn't support variable channel count");

	// 一个立方体区块中包含多少个体素，以 2 的幂表示
	uint8_t block_size_po2 = 0;
	// 各维度上区块的数量（以 3 个字节存储）
	Vector3i region_size;
	FixedArray<VoxelBuffer::Depth, CHANNEL_COUNT> channel_depths;
	// 区块以该大小的整数倍偏移量存储
	uint32_t sector_size = 0;
	FixedArray<Color8, 256> palette;
	bool has_palette = false;

	bool validate() const;
	bool verify_block(const VoxelBuffer &block) const;
};

struct RegionBlockInfo {
	static const unsigned int MAX_SECTOR_INDEX = 0xffffff;
	static const unsigned int MAX_SECTOR_COUNT = 0xff;

	// AAAB
	// A: 3 字节为扇区索引
	// B: 1 字节为区块大小，以扇区为单位
	uint32_t data = 0;

	inline uint32_t get_sector_index() const {
		return data >> 8;
	}

	inline void set_sector_index(uint32_t i) {
		CRASH_COND(i > MAX_SECTOR_INDEX);
		data = (i << 8) | (data & 0xff);
	}

	inline uint32_t get_sector_count() const {
		return data & 0xff;
	}

	inline void set_sector_count(uint32_t c) {
		CRASH_COND(c > 0xff);
		data = (c & 0xff) | (data & 0xffffff00);
	}
};

static_assert(sizeof(RegionBlockInfo) == 4, "Data in this struct must have a consistent size on all target platforms.");

// 以固定稀疏网格数据结构存储体素的归档文件。
// 该格式被设计为易于分块写入，因此可用于游戏内的局部加载与保存。
// 灵感来自 https://www.seedofandromeda.com/blogs/1-creating-a-region-file-system-for-a-voxel-game
// （若该链接无法访问，可在 Wayback Machine 上找到）
//
// 这是一种流式的实现，文件句柄在读写的整个过程中保持打开，并且只在内存中保留一小部分数据。
// 它不是线程安全的。
//
class RegionFile {
public:
	RegionFile();
	~RegionFile();

	Error open(const String &fpath, bool create_if_not_found);
	Error close();
	bool is_open() const;
	void flush();

	bool set_format(const RegionFormat &format);
	const RegionFormat &get_format() const;

	Error load_block(const Vector3i position, VoxelBuffer &out_block);
	Error save_block(
			const Vector3i position,
			const VoxelBuffer &block,
			const CompressedData::Compression compression_mode
	);

	unsigned int get_header_block_count() const;
	bool has_block(Vector3i position) const;
	bool has_block(unsigned int index) const;
	Vector3i get_block_position_from_index(uint32_t i) const;

	void debug_check();

	bool is_valid_block_position(const Vector3 position) const;

private:
	bool save_header(FileAccess &f);
	Error load_header(FileAccess &f);

	unsigned int get_block_index_in_header(const Vector3i &rpos) const;
	uint32_t get_sector_count_from_bytes(uint32_t size_in_bytes) const;

	void pad_to_sector_size(FileAccess &f);
	void remove_sectors_from_block(Vector3i block_pos, unsigned int p_sector_count);

	bool migrate_to_latest(FileAccess &f);
	bool migrate_from_v2_to_v3(FileAccess &f, RegionFormat &format);

	struct Header {
		uint8_t version = -1;
		RegionFormat format;
		// 各区块的位置与大小，以扁平化位置作为索引。
		// 该表的大小始终相同，
		// 且同一个索引始终对应于同一个 3D 位置。
		StdVector<RegionBlockInfo> blocks;
	};

	Ref<FileAccess> _file_access;
	bool _header_modified = false;

	Header _header;

	struct Vector3u16 {
		uint16_t x;
		uint16_t y;
		uint16_t z;

		Vector3u16(Vector3i p) : x(p.x), y(p.y), z(p.z) {}
	};

	// TODO 它是否曾经被读取过？
	// 扇区按其在文件中出现的顺序排列的列表，
	// 以及这些扇区所属区块的位置。同一个区块可以跨越多个扇区。
	// 它本质上就是 `Header::blocks` 的反向表。
	StdVector<Vector3u16> _sectors;
	uint32_t _blocks_begin_offset;
	String _file_path;
};

} // namespace voxel

#endif // REGION_FILE_H
