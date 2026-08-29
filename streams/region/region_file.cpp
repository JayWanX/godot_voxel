#include "region_file.h"
#include "../../streams/voxel_block_serializer.h"
#include "../../util/godot/core/array.h"
#include "../../util/godot/core/string.h"
#include "../../util/io/log.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"
#include "file_utils.h"
#include <algorithm>

namespace voxel {

namespace {
const uint8_t FORMAT_VERSION = 3;

// 版本 2 与版本 3 类似，但不包含任何格式信息
const uint8_t FORMAT_VERSION_LEGACY_2 = 2;
// const uint8_t FORMAT_VERSION_LEGACY_1 = 1;

const char *FORMAT_REGION_MAGIC = "VXR_";
const uint32_t MAGIC_AND_VERSION_SIZE = 4 + 1;
const uint32_t FIXED_HEADER_DATA_SIZE = 7 + RegionFormat::CHANNEL_COUNT;
const uint32_t PALETTE_SIZE_IN_BYTES = 256 * 4;
} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char *RegionFormat::FILE_EXTENSION = "vxr";

bool RegionFormat::validate() const {
	ERR_FAIL_COND_V(region_size.x < 0 || region_size.x >= static_cast<int>(MAX_BLOCKS_ACROSS), false);
	ERR_FAIL_COND_V(region_size.y < 0 || region_size.y >= static_cast<int>(MAX_BLOCKS_ACROSS), false);
	ERR_FAIL_COND_V(region_size.z < 0 || region_size.z >= static_cast<int>(MAX_BLOCKS_ACROSS), false);
	ERR_FAIL_COND_V(block_size_po2 <= 0, false);

	// 测试最坏情况下的限制（这不包含任意元数据，因此无法做到 100% 准确……）
	size_t bytes_per_block = 0;
	for (unsigned int i = 0; i < channel_depths.size(); ++i) {
		bytes_per_block += VoxelBuffer::get_depth_bit_count(channel_depths[i]) / 8;
	}
	bytes_per_block *= Vector3iUtil::get_volume_u64(Vector3iUtil::create(1 << block_size_po2));
	const size_t sectors_per_block = (bytes_per_block - 1) / sector_size + 1;
	ERR_FAIL_COND_V(sectors_per_block > RegionBlockInfo::MAX_SECTOR_COUNT, false);
	const size_t max_potential_sectors = Vector3iUtil::get_volume_u64(region_size) * sectors_per_block;
	ERR_FAIL_COND_V(max_potential_sectors > RegionBlockInfo::MAX_SECTOR_INDEX, false);

	return true;
}

bool RegionFormat::verify_block(const VoxelBuffer &block) const {
	ERR_FAIL_COND_V(block.get_size() != Vector3iUtil::create(1 << block_size_po2), false);
	for (unsigned int i = 0; i < VoxelBuffer::MAX_CHANNELS; ++i) {
		ERR_FAIL_COND_V(block.get_channel_depth(i) != channel_depths[i], false);
	}
	return true;
}

namespace {

uint32_t get_header_size_v3(const RegionFormat &format) {
	// 区块数据从哪个文件偏移量开始
	// 魔数 + 版本 + 数据块信息
	return MAGIC_AND_VERSION_SIZE + FIXED_HEADER_DATA_SIZE + (format.has_palette ? PALETTE_SIZE_IN_BYTES : 0) +
			Vector3iUtil::get_volume_u64(format.region_size) * sizeof(RegionBlockInfo);
}

bool save_header(
		FileAccess &f,
		uint8_t version,
		const RegionFormat &format,
		const StdVector<RegionBlockInfo> &block_infos
) {
	// `f` 可能位于文件的任意位置，我们通过 seek 确保从开头开始
	f.seek(0);

	voxel::godot::store_buffer(f, Span<const uint8_t>(reinterpret_cast<const uint8_t *>(FORMAT_REGION_MAGIC), 4));
	f.store_8(version);

	f.store_8(format.block_size_po2);

	f.store_8(format.region_size.x);
	f.store_8(format.region_size.y);
	f.store_8(format.region_size.z);

	for (unsigned int i = 0; i < format.channel_depths.size(); ++i) {
		f.store_8(format.channel_depths[i]);
	}

	f.store_16(format.sector_size);

	if (format.has_palette) {
		f.store_8(0xff);
		for (unsigned int i = 0; i < format.palette.size(); ++i) {
			const Color8 c = format.palette[i];
			f.store_8(c.r);
			f.store_8(c.g);
			f.store_8(c.b);
			f.store_8(c.a);
		}
	} else {
		f.store_8(0x00);
	}

	// TODO 处理字节序问题，这里应使用小端
	voxel::godot::store_buffer(
			f,
			Span<const uint8_t>(
					reinterpret_cast<const uint8_t *>(block_infos.data()), block_infos.size() * sizeof(RegionBlockInfo)
			)
	);

#ifdef DEBUG_ENABLED
	const size_t blocks_begin_offset = f.get_position();
	CRASH_COND(blocks_begin_offset != get_header_size_v3(format));
#endif

	return true;
}

bool load_header(
		FileAccess &f,
		uint8_t &out_version,
		RegionFormat &out_format,
		StdVector<RegionBlockInfo> &out_block_infos
) {
	ERR_FAIL_COND_V(f.get_position() != 0, false);
	ERR_FAIL_COND_V(f.get_length() < MAGIC_AND_VERSION_SIZE, false);

	FixedArray<char, 5> magic;
	fill(magic, '\0');
	ERR_FAIL_COND_V(
			voxel::godot::get_buffer(f, Span<uint8_t>(reinterpret_cast<uint8_t *>(magic.data()), 4)) != 4, false
	);
	ERR_FAIL_COND_V(strcmp(magic.data(), FORMAT_REGION_MAGIC) != 0, false);

	const uint8_t version = f.get_8();

	if (version == FORMAT_VERSION) {
		out_format.block_size_po2 = f.get_8();

		out_format.region_size.x = f.get_8();
		out_format.region_size.y = f.get_8();
		out_format.region_size.z = f.get_8();

		for (unsigned int i = 0; i < out_format.channel_depths.size(); ++i) {
			const uint8_t d = f.get_8();
			ERR_FAIL_COND_V(d >= VoxelBuffer::DEPTH_COUNT, false);
			out_format.channel_depths[i] = static_cast<VoxelBuffer::Depth>(d);
		}

		out_format.sector_size = f.get_16();

		const uint8_t palette_size = f.get_8();
		if (palette_size == 0xff) {
			out_format.has_palette = true;
			for (unsigned int i = 0; i < out_format.palette.size(); ++i) {
				Color8 c;
				c.r = f.get_8();
				c.g = f.get_8();
				c.b = f.get_8();
				c.a = f.get_8();
				out_format.palette[i] = c;
			}

		} else if (palette_size == 0x00) {
			out_format.has_palette = false;

		} else {
			VOXEL_PRINT_ERROR(format("Unexpected palette value: {}", int(palette_size)));
			return false;
		}
	}

	out_version = version;
	out_block_infos.resize(Vector3iUtil::get_volume_u64(out_format.region_size));

	// TODO 处理字节序问题
	const size_t blocks_len = out_block_infos.size() * sizeof(RegionBlockInfo);
	const size_t read_size = voxel::godot::get_buffer(f, Span<uint8_t>((uint8_t *)out_block_infos.data(), blocks_len));
	ERR_FAIL_COND_V(read_size != blocks_len, false);

	return true;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RegionFile::RegionFile() {
	// 默认值
	_header.format.block_size_po2 = 4;
	_header.format.region_size = Vector3i(16, 16, 16);
	fill(_header.format.channel_depths, VoxelBuffer::DEPTH_8_BIT);
	_header.format.sector_size = 512;
}

RegionFile::~RegionFile() {
	close();
}

Error RegionFile::open(const String &fpath, bool create_if_not_found) {
	close();

	_file_path = fpath;

	Error file_error;
	// 以读写权限打开已存在的文件。如果文件不存在，则不应创建它。
	// 注意，目前不支持只读模式，因为暂时还没有这种需求。
	Ref<FileAccess> f = voxel::godot::open_file(fpath, FileAccess::READ_WRITE, file_error);
	if (file_error != OK) {
		if (create_if_not_found) {
			CRASH_COND(f.is_valid());

			// 检查文件夹，区域“森林”需要用到
			const String fpath_base_dir = fpath.get_base_dir();
			const Error dir_err = check_directory_created_with_file_locker(fpath_base_dir);

			if (dir_err != OK) {
				return ERR_CANT_CREATE;
			}

			// 这一次我们尝试创建文件
			f = voxel::godot::open_file(fpath, FileAccess::WRITE_READ, file_error);
			if (file_error != OK) {
				ERR_PRINT(String("Failed to create file {0}").format(varray(fpath)));
				return file_error;
			}

			_header.version = FORMAT_VERSION;
			ERR_FAIL_COND_V(save_header(**f) == false, ERR_FILE_CANT_WRITE);

		} else {
			return file_error;
		}
	} else {
		CRASH_COND(f.is_null());
		const Error header_error = load_header(**f);
		if (header_error != OK) {
			return header_error;
		}
	}

	_file_access = f;

	// 预先计算扇区的位置以及它们所属的区块。
	// 当扇区在插入和删除时被移动，这将有助于了解情况。

	struct BlockInfoAndIndex {
		RegionBlockInfo b;
		unsigned int i;
	};

	// 只筛选存在的区块，并保留其索引，因为它代表了该区块的 3D 位置
	StdVector<BlockInfoAndIndex> blocks_sorted_by_offset;
	for (unsigned int i = 0; i < _header.blocks.size(); ++i) {
		const RegionBlockInfo b = _header.blocks[i];
		if (b.data != 0) {
			BlockInfoAndIndex p;
			p.b = b;
			p.i = i;
			blocks_sorted_by_offset.push_back(p);
		}
	}

	std::sort(
			blocks_sorted_by_offset.begin(),
			blocks_sorted_by_offset.end(),
			[](const BlockInfoAndIndex &a, const BlockInfoAndIndex &b) {
				return a.b.get_sector_index() < b.b.get_sector_index();
			}
	);

	CRASH_COND(_sectors.size() != 0);
	for (unsigned int i = 0; i < blocks_sorted_by_offset.size(); ++i) {
		const BlockInfoAndIndex b = blocks_sorted_by_offset[i];
		Vector3i bpos = get_block_position_from_index(b.i);
		for (unsigned int j = 0; j < b.b.get_sector_count(); ++j) {
			_sectors.push_back(bpos);
		}
	}

#ifdef DEBUG_ENABLED
	debug_check();
#endif

	return OK;
}

Error RegionFile::close() {
	VOXEL_PROFILE_SCOPE();
	Error err = OK;
	if (_file_access.is_valid()) {
		if (_header_modified) {
			if (!save_header(**_file_access)) {
			// TODO 需要好好梳理一遍这些错误码，以便返回有意义的错误信息……
			// Godot 的错误码相当有限
				err = ERR_FILE_CANT_WRITE;
			}
		}
		_file_access.unref();
	}
	_sectors.clear();
	return err;
}

bool RegionFile::is_open() const {
	return _file_access.is_valid();
}

void RegionFile::flush() {
	if (!_file_access.is_valid()) {
		return;
	}
	if (_header_modified) {
		VOXEL_ASSERT_RETURN(save_header(**_file_access));
	}
	_file_access->flush();
}

bool RegionFile::set_format(const RegionFormat &format) {
	ERR_FAIL_COND_V_MSG(_file_access.is_valid(), false, "Can't set format when the file already exists");
	ERR_FAIL_COND_V(!format.validate(), false);

	// 如果 open() 时未找到文件，将使用此格式来创建下一个文件
	_header.format = format;
	_header.blocks.resize(Vector3iUtil::get_volume_u64(format.region_size));

	return true;
}

const RegionFormat &RegionFile::get_format() const {
	return _header.format;
}

bool RegionFile::is_valid_block_position(const Vector3 position) const {
	return position.x >= 0 && //
			position.y >= 0 && //
			position.z >= 0 && //
			position.x < _header.format.region_size.x && //
			position.y < _header.format.region_size.y && //
			position.z < _header.format.region_size.z;
}

Error RegionFile::load_block(const Vector3i position, VoxelBuffer &out_block) {
	ERR_FAIL_COND_V(_file_access.is_null(), ERR_FILE_CANT_READ);
	FileAccess &f = **_file_access;

	ERR_FAIL_COND_V(!is_valid_block_position(position), ERR_INVALID_PARAMETER);
	const unsigned int lut_index = get_block_index_in_header(position);
	ERR_FAIL_COND_V(lut_index >= _header.blocks.size(), ERR_INVALID_PARAMETER);
	const RegionBlockInfo &block_info = _header.blocks[lut_index];

	if (block_info.data == 0) {
		return ERR_DOES_NOT_EXIST;
	}

	ERR_FAIL_COND_V(out_block.get_size() != out_block.get_size(), ERR_INVALID_PARAMETER);
	// 配置区块格式
	for (unsigned int channel_index = 0; channel_index < _header.format.channel_depths.size(); ++channel_index) {
		out_block.set_channel_depth(channel_index, _header.format.channel_depths[channel_index]);
	}

	const unsigned int sector_index = block_info.get_sector_index();
	const unsigned int block_begin = _blocks_begin_offset + sector_index * _header.format.sector_size;

	f.seek(block_begin);

	unsigned int block_data_size = f.get_32();
	CRASH_COND(f.eof_reached());

	ERR_FAIL_COND_V_MSG(
			!BlockSerializer::decompress_and_deserialize(f, block_data_size, out_block),
			ERR_PARSE_ERROR,
			String("Failed to read block {0}").format(varray(position))
	);

	return OK;
}

Error RegionFile::save_block(
		const Vector3i position,
		const VoxelBuffer &block,
		const CompressedData::Compression compression_mode
) {
	ERR_FAIL_COND_V(_header.format.verify_block(block) == false, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(!is_valid_block_position(position), ERR_INVALID_PARAMETER);

	ERR_FAIL_COND_V(_file_access.is_null(), ERR_FILE_CANT_WRITE);
	FileAccess &f = **_file_access;

	// 在写操作之前，我们应当被允许进行迁移
	if (_header.version != FORMAT_VERSION) {
		ERR_FAIL_COND_V(migrate_to_latest(f) == false, ERR_UNAVAILABLE);
	}

	const unsigned int lut_index = get_block_index_in_header(position);
	ERR_FAIL_COND_V(lut_index >= _header.blocks.size(), ERR_INVALID_PARAMETER);
	RegionBlockInfo &block_info = _header.blocks[lut_index];

	if (block_info.data == 0) {
		// 该区块尚不在文件中，追加到末尾

		const unsigned int end_offset = _blocks_begin_offset + _sectors.size() * _header.format.sector_size;
		f.seek(end_offset);
		const unsigned int block_offset = f.get_position();
		// 检查位置是否符合扇区规则
		CRASH_COND((block_offset - _blocks_begin_offset) % _header.format.sector_size != 0);

		BlockSerializer::SerializeResult res = BlockSerializer::serialize_and_compress(block, compression_mode);
		ERR_FAIL_COND_V(!res.success, ERR_INVALID_PARAMETER);
		f.store_32(res.data.size());
		const unsigned int written_size = sizeof(uint32_t) + res.data.size();
		voxel::godot::store_buffer(f, to_span(res.data));

		const unsigned int end_pos = f.get_position();
		CRASH_COND_MSG(
				written_size != (end_pos - block_offset),
				String("written_size: {0}, block_offset: {1}, end_pos: {2}")
						.format(varray(written_size, block_offset, end_pos))
		);
		pad_to_sector_size(f);

		block_info.set_sector_index((block_offset - _blocks_begin_offset) / _header.format.sector_size);
		block_info.set_sector_count(get_sector_count_from_bytes(written_size));

		for (unsigned int i = 0; i < block_info.get_sector_count(); ++i) {
			_sectors.push_back(position);
		}

		_header_modified = true;

	} else {
		// 该区块已在文件中

		CRASH_COND(_sectors.size() == 0);

		const int old_sector_index = block_info.get_sector_index();
		const int old_sector_count = block_info.get_sector_count();
		CRASH_COND(old_sector_count < 1);

		BlockSerializer::SerializeResult res = BlockSerializer::serialize_and_compress(block, compression_mode);
		ERR_FAIL_COND_V(!res.success, ERR_INVALID_PARAMETER);
		const StdVector<uint8_t> &data = res.data;
		const size_t written_size = sizeof(uint32_t) + data.size();

		const int new_sector_count = get_sector_count_from_bytes(written_size);
		CRASH_COND(new_sector_count < 1);

		if (new_sector_count <= old_sector_count) {
			// 我们可以将区块写入原来的位置

			if (new_sector_count < old_sector_count) {
				// 该区块现在使用的扇区更少了，可以压缩其他区块来腾出空间。
				remove_sectors_from_block(position, old_sector_count - new_sector_count);
				_header_modified = true;
			}

			const size_t block_offset = _blocks_begin_offset + old_sector_index * _header.format.sector_size;
			f.seek(block_offset);

			f.store_32(data.size());
			voxel::godot::store_buffer(f, to_span(data));

			const size_t end_pos = f.get_position();
			CRASH_COND(written_size != (end_pos - block_offset));

		} else {
			// 该区块现在使用了更多扇区，我们必须移动其他区块。
			// 注意：我们可以将区块整体前移，但也可以直接删除该区块并在末尾重写。
			// 需要研究一下是否值得实现前移的方式。
			// TODO 是否更倾向于做某种“尾部交换”的操作？

			// 这也会移动文件其余部分，因此释放的扇区可能会被重新占用。
			remove_sectors_from_block(position, old_sector_count);

			const size_t block_offset = _blocks_begin_offset + _sectors.size() * _header.format.sector_size;
			f.seek(block_offset);

			f.store_32(data.size());
			voxel::godot::store_buffer(f, to_span(data));

			const size_t end_pos = f.get_position();
			CRASH_COND(written_size != (end_pos - block_offset));

			pad_to_sector_size(f);

			block_info.set_sector_index(_sectors.size());
			for (int i = 0; i < new_sector_count; ++i) {
				_sectors.push_back(Vector3u16(position));
			}

			_header_modified = true;
		}

		block_info.set_sector_count(new_sector_count);
	}

	return OK;
}

void RegionFile::pad_to_sector_size(FileAccess &f) {
	const int64_t rpos = f.get_position() - _blocks_begin_offset;
	if (rpos == 0) {
		return;
	}
	CRASH_COND(rpos < 0);
	const int64_t pad = int64_t(_header.format.sector_size) - (rpos - 1) % int64_t(_header.format.sector_size) - 1;
	CRASH_COND(pad < 0);
	for (int64_t i = 0; i < pad; ++i) {
		// 虚函数被多次调用，嗯……
		f.store_8(0);
	}
}

void RegionFile::remove_sectors_from_block(Vector3i block_pos, unsigned int p_sector_count) {
	VOXEL_PROFILE_SCOPE();

	// 从一个区块中移除扇区，从最后的几个扇区开始。
	// 例如，如果一个区块有 5 个扇区，我们移除 2 个，那么前 3 个会被保留。
	// 随后，所有后续的扇区都在文件中向前移动以填补空缺。

	CRASH_COND(_file_access.is_null());
	CRASH_COND(p_sector_count <= 0);

	FileAccess &f = **_file_access;
	const unsigned int sector_size = _header.format.sector_size;
	const unsigned int old_end_offset = _blocks_begin_offset + _sectors.size() * sector_size;

	const unsigned int block_index = get_block_index_in_header(block_pos);
	CRASH_COND(block_index >= _header.blocks.size());
	RegionBlockInfo &block_info = _header.blocks[block_index];

	unsigned int src_offset =
			_blocks_begin_offset + (block_info.get_sector_index() + block_info.get_sector_count()) * sector_size;

	unsigned int dst_offset = src_offset - p_sector_count * sector_size;

	CRASH_COND(_sectors.size() < p_sector_count);
	CRASH_COND(src_offset - sector_size < dst_offset);
	CRASH_COND(block_info.get_sector_index() + p_sector_count > _sectors.size());
	CRASH_COND(p_sector_count > block_info.get_sector_count());
	CRASH_COND(dst_offset < _blocks_begin_offset);

	StdVector<uint8_t> temp;
	temp.resize(sector_size);

	// TODO 也许有更快的方法来收缩文件
	// 从文件中擦除扇区
	while (src_offset < old_end_offset) {
		f.seek(src_offset);
		const size_t read_bytes = voxel::godot::get_buffer(f, to_span(temp));
		CRASH_COND(read_bytes != sector_size); // 文件已损坏

		f.seek(dst_offset);
		voxel::godot::store_buffer(f, to_span(temp));

		src_offset += sector_size;
		dst_offset += sector_size;
	}

	// TODO 我们需要截断文件末尾，因为我们实际上缩短了文件，
	// 但 FileAccess 没有任何函数可以做到这一点……所以也不能依赖 EOF

	// 从缓存中擦除扇区
	_sectors.erase(
			_sectors.begin() + (block_info.get_sector_index() + block_info.get_sector_count() - p_sector_count),
			_sectors.begin() + (block_info.get_sector_index() + block_info.get_sector_count())
	);

	const unsigned int old_sector_index = block_info.get_sector_index();

	// 在头部中减少当前区块的扇区数。
	if (block_info.get_sector_count() > p_sector_count) {
		block_info.set_sector_count(block_info.get_sector_count() - p_sector_count);
	} else {
		// 区块已移除
		block_info.data = 0;
	}

	// 移动后续区块的扇区索引
	if (old_sector_index < _sectors.size()) {
		for (unsigned int i = 0; i < _header.blocks.size(); ++i) {
			RegionBlockInfo &b = _header.blocks[i];
			if (b.data != 0 && b.get_sector_index() > old_sector_index) {
				b.set_sector_index(b.get_sector_index() - p_sector_count);
			}
		}
	}
}

bool RegionFile::save_header(FileAccess &f) {
	// 在写操作之前，我们应当被允许进行迁移.
	if (_header.version != FORMAT_VERSION) {
		ERR_FAIL_COND_V(migrate_to_latest(f) == false, false);
	}
	ERR_FAIL_COND_V(!voxel::save_header(f, _header.version, _header.format, _header.blocks), false);
	_blocks_begin_offset = f.get_position();
	_header_modified = false;
	return true;
}

bool RegionFile::migrate_from_v2_to_v3(FileAccess &f, RegionFormat &format) {
	VOXEL_PRINT_VERBOSE(voxel::format("Migrating region file {} from v2 to v3", _file_path));

	// 如果我们提前知道文件应当包含的格式，就可以进行迁移。
	ERR_FAIL_COND_V_MSG(format.block_size_po2 == 0, false, "Cannot migrate without knowing the correct format");

	// 区块数据从哪个文件偏移量开始
	// 魔数 + 版本 + 数据块信息
	const unsigned int old_header_size = Vector3iUtil::get_volume_u64(format.region_size) * sizeof(uint32_t);

	const unsigned int new_header_size = get_header_size_v3(format) - MAGIC_AND_VERSION_SIZE;
	ERR_FAIL_COND_V_MSG(new_header_size < old_header_size, false, "New version is supposed to have larger header");

	const unsigned int extra_bytes_needed = new_header_size - old_header_size;

	f.seek(MAGIC_AND_VERSION_SIZE);
	voxel::godot::insert_bytes(f, extra_bytes_needed);

	// 设置版本号，否则 `save_header` 会尝试再次迁移，从而导致栈溢出
	_header.version = FORMAT_VERSION;

	return save_header(f);
}

bool RegionFile::migrate_to_latest(FileAccess &f) {
	ERR_FAIL_COND_V(_file_path.is_empty(), false);

	uint8_t version = _header.version;

	// 是否要做一个备份？
	// {
	// 	DirAccessRef da = DirAccess::create_for_path(_file_path.get_base_dir());
	// 	ERR_FAIL_COND_V_MSG(!da, false, String("Can't make a backup before migrating {0}").format(varray(_file_path)));
	// 	da->copy(_file_path, _file_path + ".backup");
	// }

	if (version == FORMAT_VERSION_LEGACY_2) {
		ERR_FAIL_COND_V(!migrate_from_v2_to_v3(f, _header.format), false);
		version = FORMAT_VERSION;
	}

	if (version != FORMAT_VERSION) {
		ERR_PRINT(String("Invalid file version: {0}").format(varray(version)));
		return false;
	}

	_header.version = version;
	return true;
}

Error RegionFile::load_header(FileAccess &f) {
	ERR_FAIL_COND_V(!voxel::load_header(f, _header.version, _header.format, _header.blocks), ERR_PARSE_ERROR);
	_blocks_begin_offset = f.get_position();
	return OK;
}

unsigned int RegionFile::get_block_index_in_header(const Vector3i &rpos) const {
	return Vector3iUtil::get_zxy_index(rpos, _header.format.region_size);
}

Vector3i RegionFile::get_block_position_from_index(uint32_t i) const {
	return Vector3iUtil::from_zxy_index(i, _header.format.region_size);
}

uint32_t RegionFile::get_sector_count_from_bytes(uint32_t size_in_bytes) const {
	return (size_in_bytes - 1) / _header.format.sector_size + 1;
}

unsigned int RegionFile::get_header_block_count() const {
	ERR_FAIL_COND_V(!is_open(), 0);
	return _header.blocks.size();
}

bool RegionFile::has_block(Vector3i position) const {
	ERR_FAIL_COND_V(!is_open(), false);
	ERR_FAIL_COND_V(!is_valid_block_position(position), false);
	const unsigned int bi = get_block_index_in_header(position);
	return _header.blocks[bi].data != 0;
}

bool RegionFile::has_block(unsigned int index) const {
	ERR_FAIL_COND_V(!is_open(), false);
	CRASH_COND(index >= _header.blocks.size());
	return _header.blocks[index].data != 0;
}

// 检查以检测文件中是否存在某些损坏迹象
void RegionFile::debug_check() {
	ERR_FAIL_COND(!is_open());
	ERR_FAIL_COND(_file_access.is_null());
	FileAccess &f = **_file_access;
	const size_t file_len = f.get_length();

	for (unsigned int lut_index = 0; lut_index < _header.blocks.size(); ++lut_index) {
		const RegionBlockInfo &block_info = _header.blocks[lut_index];
		const Vector3i position = get_block_position_from_index(lut_index);
		if (block_info.data == 0) {
			continue;
		}
		const unsigned int sector_index = block_info.get_sector_index();
		const unsigned int block_begin = _blocks_begin_offset + sector_index * _header.format.sector_size;
		if (block_begin >= file_len) {
			VOXEL_PRINT_ERROR(format(
					"LUT {} {}: offset {} is larger than file size {}", lut_index, position, block_begin, file_len
			));
			continue;
		}
		f.seek(block_begin);
		const size_t block_data_size = f.get_32();
		const size_t pos = f.get_position();
		const size_t remaining_size = file_len - pos;
		if (block_data_size > remaining_size) {
			VOXEL_PRINT_ERROR(
					format("LUT {} {}: block size {} at offset {} is larger than remaining size {}",
						   lut_index,
						   position,
						   block_data_size,
						   block_begin,
						   remaining_size)
			);
		}
	}
}

} // namespace voxel
