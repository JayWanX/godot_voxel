#include "voxel_stream_region_files.h"
#include "../../engine/voxel_engine.h"
#include "../../util/godot/classes/directory.h"
#include "../../util/godot/classes/json.h"
#include "../../util/godot/classes/time.h"
#include "../../util/godot/core/array.h"
#include "../../util/godot/core/string.h"
#include "../../util/io/log.h"
#include "../../util/math/box3i.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"
#include "file_utils.h"

#include <algorithm>

namespace voxel {

namespace {
const uint8_t FORMAT_VERSION = 3;

// 版本 2 与版本 3 相同，只是区域文件使用的是其规范的第 3 版。
const uint8_t FORMAT_VERSION_LEGACY_2 = 2;

const uint8_t FORMAT_VERSION_LEGACY_1 = 1;
const char *META_FILE_NAME = "meta.vxrm";

} // namespace

// 对一个序列进行排序但不修改它，返回排序后的指针列表
template <typename T, typename Comparer_T>
void get_sorted_indices(Span<T> sequence, Comparer_T comparer, StdVector<unsigned int> &out_sorted_indices) {
	struct Compare {
		Span<T> sequence;
		Comparer_T comparer;
		inline bool operator()(unsigned int ia, unsigned int ib) const {
			return comparer(sequence[ia], sequence[ib]);
		}
	};
	out_sorted_indices.resize(sequence.size());
	for (unsigned int i = 0; i < sequence.size(); ++i) {
		out_sorted_indices[i] = i;
	}
	SortArray<unsigned int, Compare> sort_array;
	sort_array.compare.sequence = sequence;
	sort_array.compare.comparer = comparer;
	sort_array.sort(out_sorted_indices.data(), out_sorted_indices.size());
}

VoxelStreamRegionFiles::VoxelStreamRegionFiles() {
	_meta.version = FORMAT_VERSION;
	_meta.block_size_po2 = 4;
	_meta.region_size_po2 = 4;
	_meta.sector_size = 512; // next_power_of_2(_meta.block_size.volume() / 10) // 基于压缩比
	// _meta.lod_count = 1;
	fill(_meta.channel_depths, VoxelBuffer::DEFAULT_CHANNEL_DEPTH);
	_meta.channel_depths[VoxelBuffer::CHANNEL_TYPE] = VoxelBuffer::DEFAULT_TYPE_CHANNEL_DEPTH;
	_meta.channel_depths[VoxelBuffer::CHANNEL_SDF] = VoxelBuffer::DEFAULT_SDF_CHANNEL_DEPTH;
	_meta.channel_depths[VoxelBuffer::CHANNEL_INDICES] = VoxelBuffer::DEFAULT_INDICES_CHANNEL_DEPTH;
	_meta.channel_depths[VoxelBuffer::CHANNEL_WEIGHTS] = VoxelBuffer::DEFAULT_WEIGHTS_CHANNEL_DEPTH;
}

VoxelStreamRegionFiles::~VoxelStreamRegionFiles() {
	close_all_regions();
}

void VoxelStreamRegionFiles::load_voxel_block(VoxelStream::VoxelQueryData &query) {
	load_voxel_blocks(Span<VoxelStream::VoxelQueryData>(&query, 1));
}

void VoxelStreamRegionFiles::save_voxel_block(VoxelStream::VoxelQueryData &query) {
	save_voxel_blocks(Span<VoxelStream::VoxelQueryData>(&query, 1));
}

void VoxelStreamRegionFiles::load_voxel_blocks(Span<VoxelStream::VoxelQueryData> p_blocks) {
	VOXEL_PROFILE_SCOPE();

	// 为了尽量减少文件的打开/关闭次数，请求会按照所属区域进行分组。

	// 必须复制输入以便排序，因为模块中某些部分在收到乱序响应时会出现问题
	StdVector<unsigned int> sorted_block_indices;
	BlockQueryComparator comparator;
	comparator.self = this;
	get_sorted_indices(p_blocks, comparator, sorted_block_indices);

	for (unsigned int i = 0; i < sorted_block_indices.size(); ++i) {
		const unsigned int bi = sorted_block_indices[i];
		VoxelStream::VoxelQueryData &q = p_blocks[bi];
		const EmergeResult result = _load_block(q.voxel_buffer, q.position_in_blocks, q.lod_index);
		switch (result) {
			case EMERGE_OK:
				q.result = RESULT_BLOCK_FOUND;
				break;
			case EMERGE_OK_FALLBACK:
				q.result = RESULT_BLOCK_NOT_FOUND;
				break;
			case EMERGE_FAILED:
				q.result = RESULT_ERROR;
				break;
			default:
				CRASH_NOW();
				break;
		}
	}
}

void VoxelStreamRegionFiles::save_voxel_blocks(Span<VoxelStream::VoxelQueryData> p_blocks) {
	VOXEL_PROFILE_SCOPE();

	// 必须复制输入以便排序，因为模块中某些部分在收到乱序响应时会出现问题
	StdVector<unsigned int> sorted_block_indices;
	BlockQueryComparator comparator;
	comparator.self = this;
	get_sorted_indices(p_blocks, comparator, sorted_block_indices);

	for (unsigned int i = 0; i < sorted_block_indices.size(); ++i) {
		const unsigned int bi = sorted_block_indices[i];
		VoxelStream::VoxelQueryData &q = p_blocks[bi];
		_save_block(q.voxel_buffer, q.position_in_blocks, q.lod_index);
	}
}

int VoxelStreamRegionFiles::get_used_channels_mask() const {
	// 假定为全部通道，因为该流可以存储任何内容。
	return VoxelBuffer::ALL_CHANNELS_MASK;
}

VoxelStreamRegionFiles::EmergeResult VoxelStreamRegionFiles::_load_block(
		VoxelBuffer &out_buffer,
		const Vector3i block_pos,
		const uint8_t lod
) {
	VOXEL_PROFILE_SCOPE();

	MutexLock lock(_mutex);

	if (_directory_path.is_empty()) {
		return EMERGE_OK_FALLBACK;
	}

	if (!_meta_loaded) {
		// TODO 当从零开始加载地形、且尚未保存任何内容时，这种做法并不理想。
		// 它几乎会为每个区块都尝试打开文件、失败并返回“OK_FALLBACK”，但
		// 反复的 IO 操作浪费了时间
		const voxel::godot::FileResult load_res = load_meta();
		if (load_res != voxel::godot::FILE_OK) {
			// 从未保存过任何区块
			return EMERGE_OK_FALLBACK;
		}
	}

	const Vector3i block_size = Vector3iUtil::create(1 << _meta.block_size_po2);
	const Vector3i region_size = Vector3iUtil::create(1 << _meta.region_size_po2);

	CRASH_COND(!_meta_loaded);
	ERR_FAIL_COND_V(lod >= constants::MAX_LOD, EMERGE_FAILED);
	ERR_FAIL_COND_V(block_size != out_buffer.get_size(), EMERGE_FAILED);

	// 配置通道深度，因为旧区块数据可能未指定它们。
	// 区域应当包含这些深度信息，并据此在缓冲区中获知需要读取多少数据。
	for (unsigned int channel_index = 0; channel_index < _meta.channel_depths.size(); ++channel_index) {
		out_buffer.set_channel_depth(channel_index, _meta.channel_depths[channel_index]);
	}

	const Vector3i region_pos = get_region_position_from_blocks(block_pos);

	CachedRegion *cache = open_region(region_pos, lod, false);
	if (cache == nullptr || !cache->file_exists) {
		return EMERGE_OK_FALLBACK;
	}

	const Vector3i block_rpos = math::wrap(block_pos, region_size);

	const Error err = cache->region.load_block(block_rpos, out_buffer);
	switch (err) {
		case OK:
			return EMERGE_OK;

		case ERR_DOES_NOT_EXIST:
			return EMERGE_OK_FALLBACK;

		default:
			return EMERGE_FAILED;
	}
}

void VoxelStreamRegionFiles::_save_block(
		const voxel::VoxelBuffer &voxel_buffer, const Vector3i block_pos, const uint8_t lod
) {
	VOXEL_PROFILE_SCOPE();
	using namespace voxel::godot;

	MutexLock lock(_mutex);

	ERR_FAIL_COND(_directory_path.is_empty());

	if (!_meta_loaded) {
		// 如果尚未加载，总是先尝试加载元数据文件（如果它已存在），
		// 因为我们可能希望在没有任何读取的情况下保存区块
		FileResult load_res = load_meta();
		if (load_res != FILE_OK && load_res != FILE_CANT_OPEN) {
			// 文件存在但有问题
			String meta_path = _directory_path.path_join(META_FILE_NAME);
			ERR_PRINT(String("Could not read {0}: error {1}")
							  .format(varray(meta_path, voxel::godot::to_string(load_res))));
			return;
		}
	}

	if (!_meta_saved) {
		// 首次保存元数据文件时，以第一个区块的格式对其进行初始化
		for (unsigned int i = 0; i < _meta.channel_depths.size(); ++i) {
			_meta.channel_depths[i] = voxel_buffer.get_channel_depth(i);
		}
		FileResult err = save_meta();
		ERR_FAIL_COND(err != FILE_OK);
	}

	// 校验格式
	const Vector3i block_size = Vector3iUtil::create(1 << _meta.block_size_po2);
	ERR_FAIL_COND(voxel_buffer.get_size() != block_size);
	for (unsigned int i = 0; i < voxel::VoxelBuffer::MAX_CHANNELS; ++i) {
		ERR_FAIL_COND(voxel_buffer.get_channel_depth(i) != _meta.channel_depths[i]);
	}

	const Vector3i region_size = Vector3iUtil::create(1 << _meta.region_size_po2);
	Vector3i region_pos = get_region_position_from_blocks(block_pos);
	Vector3i block_rpos = math::wrap(block_pos, region_size);

	CachedRegion *cache = open_region(region_pos, lod, true);
	ERR_FAIL_COND_MSG(cache == nullptr, "Could not save region file data");
	ERR_FAIL_COND(cache->region.save_block(block_rpos, voxel_buffer, _compression_mode) != OK);
}

String VoxelStreamRegionFiles::get_directory() const {
	MutexLock lock(_mutex);
	return _directory_path;
}

void VoxelStreamRegionFiles::set_directory(String dirpath) {
	MutexLock lock(_mutex);
	if (_directory_path != dirpath) {
		close_all_regions();
		_directory_path = dirpath.strip_edges();
		_meta_loaded = false;
		_meta_saved = false;
		load_meta();
		notify_property_list_changed();
	}
}

namespace {

bool u8_from_json_variant(const Variant &v, uint8_t &i) {
	ERR_FAIL_COND_V(v.get_type() != Variant::INT && v.get_type() != Variant::FLOAT, false);
	int n = v;
	ERR_FAIL_COND_V(n < 0 || n > 255, false);
	// 先转为 int 便于范围校验
	i = int(v);
	return true;
}

bool u32_from_json_variant(const Variant &v, uint32_t &i) {
	ERR_FAIL_COND_V(v.get_type() != Variant::INT && v.get_type() != Variant::FLOAT, false);
	ERR_FAIL_COND_V(v.operator int64_t() < 0, false);
	i = v;
	return true;
}

bool depth_from_json_variant(Variant &v, VoxelBuffer::Depth &d) {
	uint8_t n;
	ERR_FAIL_COND_V(!u8_from_json_variant(v, n), false);
	VOXEL_ASSERT_RETURN_V(n < VoxelBuffer::DEPTH_COUNT, false);
	d = (VoxelBuffer::Depth)n;
	return true;
}

} // namespace

voxel::godot::FileResult VoxelStreamRegionFiles::save_meta() {
	using namespace voxel::godot;

	ERR_FAIL_COND_V(_directory_path == "", FILE_CANT_OPEN);

	Dictionary d;
	d["version"] = _meta.version;
	d["block_size_po2"] = _meta.block_size_po2;
	d["region_size_po2"] = _meta.region_size_po2;
	// d["lod_count"] = _meta.lod_count;
	d["sector_size"] = _meta.sector_size;

	Array channel_depths;
	channel_depths.resize(_meta.channel_depths.size());
	for (unsigned int i = 0; i < _meta.channel_depths.size(); ++i) {
		channel_depths[i] = _meta.channel_depths[i];
	}
	d["channel_depths"] = channel_depths;

	const String json_string = JSON::stringify(d, "\t", true);

	// 确保目录存在
	{
		const Error err = check_directory_created_with_file_locker(_directory_path);
		if (err != OK) {
			ERR_PRINT("Could not save meta");
			return FILE_CANT_OPEN;
		}
	}

	const String meta_path = _directory_path.path_join(META_FILE_NAME);
	const CharString meta_path_utf8 = meta_path.utf8();

	Error err;
	VoxelFileLockerWrite file_wlock(meta_path_utf8.get_data());
	Ref<FileAccess> f = open_file(meta_path, FileAccess::WRITE, err);
	if (f.is_null()) {
		ERR_PRINT(String("Could not save {0}").format(varray(meta_path)));
		return FILE_CANT_OPEN;
	}

	f->store_string(json_string);

	_meta_saved = true;
	_meta_loaded = true;

	return FILE_OK;
}

namespace {

void migrate_region_meta_data(Dictionary &data) {
	if (data["version"] == Variant(real_t(FORMAT_VERSION_LEGACY_1))) {
		Array depths;
		depths.resize(VoxelBuffer::MAX_CHANNELS);
		for (int i = 0; i < depths.size(); ++i) {
			depths[i] = VoxelBuffer::DEFAULT_CHANNEL_DEPTH;
		}
		data["channel_depths"] = depths;
		data["version"] = FORMAT_VERSION_LEGACY_2;
	}

	if (data["version"] == Variant(real_t(FORMAT_VERSION_LEGACY_2))) {
		// 区域森林本身无需改动，但表明区域文件可能被升级到 v3。
		data["version"] = FORMAT_VERSION;
	}

	// if (data["version"] != Variant(real_t(FORMAT_VERSION))) {
	//  TODO 抛出错误？
	// }
}

} // namespace

voxel::godot::FileResult VoxelStreamRegionFiles::load_meta() {
	using namespace voxel::godot;

	ERR_FAIL_COND_V(_directory_path == "", FILE_CANT_OPEN);

	// 在加载另一个世界之前，请确保已清理上一个世界
	CRASH_COND(_region_cache.size() > 0);

	const String meta_path = _directory_path.path_join(META_FILE_NAME);
	String json_string;

	{
		Error err;
		const CharString meta_path_utf8 = meta_path.utf8();
		VoxelFileLockerRead file_rlock(meta_path_utf8.get_data());
		Ref<FileAccess> f = open_file(meta_path, FileAccess::READ, err);
		if (f.is_null()) {
			return FILE_CANT_OPEN;
		}
		json_string = get_as_text(**f);
	}

	// 注意：我选择 JSON 纯粹是为了方便调试。这个文件并非设计为由手工编辑。
	// 世界配置的变更可能需要一个完整的转换器。

	Ref<JSON> json;
	json.instantiate();
	const Error json_err = json->parse(json_string);
	Variant res = json->get_data();
	if (json_err != OK) {
		const String json_err_msg = json->get_error_message();
		const int json_err_line = json->get_error_line();
		VOXEL_PRINT_ERROR(format("Error when parsing {}: line {}: {}", meta_path, json_err_line, json_err_msg));
		return FILE_INVALID_DATA;
	}

	Dictionary d = res;
	migrate_region_meta_data(d);
	Meta meta;
	ERR_FAIL_COND_V(!u8_from_json_variant(d["version"], meta.version), FILE_INVALID_DATA);
	ERR_FAIL_COND_V(!u8_from_json_variant(d["block_size_po2"], meta.block_size_po2), FILE_INVALID_DATA);
	ERR_FAIL_COND_V(!u8_from_json_variant(d["region_size_po2"], meta.region_size_po2), FILE_INVALID_DATA);
	// ERR_FAIL_COND_V(!u8_from_json_variant(d["lod_count"], meta.lod_count), FILE_INVALID_DATA);
	ERR_FAIL_COND_V(!u32_from_json_variant(d["sector_size"], meta.sector_size), FILE_INVALID_DATA);

	ERR_FAIL_COND_V(meta.version < 0, FILE_INVALID_DATA);

	Array channel_depths_data = d["channel_depths"];
	ERR_FAIL_COND_V(channel_depths_data.size() != voxel::VoxelBuffer::MAX_CHANNELS, FILE_INVALID_DATA);
	for (int i = 0; i < channel_depths_data.size(); ++i) {
		ERR_FAIL_COND_V(!depth_from_json_variant(channel_depths_data[i], meta.channel_depths[i]), FILE_INVALID_DATA);
	}

	ERR_FAIL_COND_V(!check_meta(meta), FILE_INVALID_DATA);

	_meta = meta;
	_meta_loaded = true;
	_meta_saved = true;

	return FILE_OK;
}

bool VoxelStreamRegionFiles::check_meta(const Meta &meta) {
	ERR_FAIL_COND_V(meta.block_size_po2 < 1 || meta.block_size_po2 > 8, false);
	ERR_FAIL_COND_V(meta.region_size_po2 < 1 || meta.region_size_po2 > 8, false);
	// ERR_FAIL_COND_V(meta.lod_count <= 0 || meta.lod_count > 32, false);
	ERR_FAIL_COND_V(meta.sector_size <= 0 || meta.sector_size > 65536, false);
	return true;
}

Vector3i VoxelStreamRegionFiles::get_block_position_from_voxels(const Vector3i &origin_in_voxels) const {
	return origin_in_voxels >> _meta.block_size_po2;
}

Vector3i VoxelStreamRegionFiles::get_region_position_from_blocks(const Vector3i &block_position) const {
	return block_position >> _meta.region_size_po2;
}

void VoxelStreamRegionFiles::close_all_regions() {
	for (unsigned int i = 0; i < _region_cache.size(); ++i) {
		CachedRegion *cache = _region_cache[i];
		close_region(cache);
		VOXEL_DELETE(cache);
	}
	_region_cache.clear();
}

String VoxelStreamRegionFiles::get_region_file_path(const Vector3i &region_pos, unsigned int lod) const {
	Array a;
	a.resize(5);
	a[0] = lod;
	a[1] = region_pos.x;
	a[2] = region_pos.y;
	a[3] = region_pos.z;
	a[4] = RegionFormat::FILE_EXTENSION;
	return _directory_path.path_join(String("regions/lod{0}/r.{1}.{2}.{3}.{4}").format(a));
}

VoxelStreamRegionFiles::CachedRegion *VoxelStreamRegionFiles::get_region_from_cache(const Vector3i pos, int lod) const {
	// 线性搜索可能比 Map 数据结构更好，
	// 因为同一时间缓存的区域不太会超过约 10 个
	for (unsigned int i = 0; i < _region_cache.size(); ++i) {
		CachedRegion *r = _region_cache[i];
		if (r->position == pos && r->lod == lod) {
			return r;
		}
	}
	return nullptr;
}

VoxelStreamRegionFiles::CachedRegion *VoxelStreamRegionFiles::open_region(
		const Vector3i region_pos,
		unsigned int lod,
		bool create_if_not_found
) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND_V(!_meta_loaded, nullptr);
	VOXEL_ASSERT_RETURN_V(lod < constants::MAX_LOD, nullptr);

	CachedRegion *cached_region = get_region_from_cache(region_pos, lod);
	if (cached_region != nullptr) {
		return cached_region;
	}

	while (_region_cache.size() > _max_open_regions - 1) {
		close_oldest_region();
	}
	// 不在缓存中，我们必须打开或创建它

	String fpath = get_region_file_path(region_pos, lod);

	cached_region = VOXEL_NEW(CachedRegion);

	// 配置格式，因为我们可能必须创建该文件，而且一些旧文件版本没有内嵌格式
	{
		RegionFormat format;
		format.block_size_po2 = _meta.block_size_po2;
		format.channel_depths = _meta.channel_depths;
		// TODO 调色板支持
		format.has_palette = false;
		format.region_size = Vector3iUtil::create(1 << _meta.region_size_po2);
		format.sector_size = _meta.sector_size;

		cached_region->region.set_format(format);
		cached_region->position = region_pos;
		cached_region->lod = lod;
	}

	const Error err = cached_region->region.open(fpath, create_if_not_found);

	// 我们可以为优化而做的一些事情：
	// - 缓存文件不存在这一事实，这样就不必每次都通过系统调用来实际检查它。
	// - 一旦读取过头部，就无需再次读取，
	//   我们假设没有其他进程会修改区域文件。

	if (err != OK) {
		VOXEL_DELETE(cached_region);
		if (create_if_not_found) {
			// 显然无法创建它
			ERR_PRINT(String("Could not open or create region file {0}, error: {1}").format(varray(fpath, err)));
			return nullptr;
		} else {
			// 文件不存在，这可能是预期情况
			return nullptr;
		}
	}

	// 确保它具有正确的格式
	{
		const RegionFormat &format = cached_region->region.get_format();
		if (format.block_size_po2 != _meta.block_size_po2 //
			|| format.channel_depths != _meta.channel_depths //
			|| format.region_size != Vector3iUtil::create(1 << _meta.region_size_po2) //
			|| format.sector_size != _meta.sector_size) {
			ERR_PRINT("Region file has unexpected format");
			VOXEL_DELETE(cached_region);
			return nullptr;
		}
	}

	// TODO 进行调试检查，确保我们尚未将其缓存
	_region_cache.push_back(cached_region);

	cached_region->file_exists = true;
	cached_region->last_opened = Time::get_singleton()->get_ticks_usec();

	return cached_region;
}

// TODO 是否应移除以简化代码？
void VoxelStreamRegionFiles::close_region(CachedRegion *region) {
	region->region.close();
}

void VoxelStreamRegionFiles::close_oldest_region() {
	// 关闭假定为最久未使用的区域

	if (_region_cache.size() == 0) {
		return;
	}

	int oldest_index = -1;
	uint64_t oldest_time = 0;
	const uint64_t now = Time::get_singleton()->get_ticks_usec();

	for (unsigned int i = 0; i < _region_cache.size(); ++i) {
		const CachedRegion *r = _region_cache[i];
		const uint64_t time = now - r->last_opened;
		if (time >= oldest_time) {
			oldest_index = i;
		}
	}

	CachedRegion *region = _region_cache[oldest_index];
	_region_cache.erase(_region_cache.begin() + oldest_index);

	close_region(region);
	VOXEL_DELETE(region);
}

namespace {

inline int convert_block_coordinate(int p_x, int old_size, int new_size) {
	return math::floordiv(p_x * old_size, new_size);
}

Vector3i convert_block_coordinates(Vector3i pos, Vector3i old_size, Vector3i new_size) {
	return Vector3i(
			convert_block_coordinate(pos.x, old_size.x, new_size.x),
			convert_block_coordinate(pos.y, old_size.y, new_size.y),
			convert_block_coordinate(pos.z, old_size.z, new_size.z)
	);
}

} // namespace

void VoxelStreamRegionFiles::_convert_files(Meta new_meta) {
	using namespace voxel::godot;

	// TODO 跨不同区块大小的转换尚未经过测试。
	// 我写它是因为，由于设置改动而丢失大型体素世界实在太可惜，所以将来我们
	// 可能会需要它

	VOXEL_PRINT_VERBOSE("Converting region files");
	// 这可能是一个非常漫长缓慢的操作，最好在线程中运行它。

	ERR_FAIL_COND(!_meta_saved);
	ERR_FAIL_COND(!_meta_loaded);

	close_all_regions();

	Ref<VoxelStreamRegionFiles> old_stream;
	old_stream.instantiate();
	// 为旧流将文件缓存保持在最小，反正我们只查询一次所有数据块
	old_stream->_max_open_regions = MAX(1, FOPEN_MAX);

	// 通过重命名来备份当前文件夹，让当前名称空出来
	{
		// Error dir_open_err;
		// Ref<DirAccess> da = open_directory(_directory_path + "/..", &dir_open_err);
		// ERR_FAIL_COND_MSG(
		// 		da.is_null(), String("Failed to open {0}: error {1}").format(varray(_directory_path, dir_open_err))
		// );
		int i = 0;
		String old_dir;
		while (true) {
			if (i == 0) {
				old_dir = _directory_path + "_old";
			} else {
				old_dir = _directory_path + "_old" + String::num_int64(i);
			}
			if (directory_exists(old_dir)) {
				++i;
			} else {
				const Error err = rename_directory(_directory_path, old_dir);
				ERR_FAIL_COND_MSG(
						err != OK,
						String("Failed to rename '{0}' to '{1}', error {2}")
								.format(varray(_directory_path, old_dir, err))
				);
				break;
			}
		}

		old_stream->set_directory(old_dir);
		VOXEL_PRINT_VERBOSE(format("Data backed up as {}", old_dir));
	}

	struct PositionAndLod {
		Vector3i position;
		uint8_t lod_index;
	};

	ERR_FAIL_COND(old_stream->load_meta() != FILE_OK);

	StdVector<PositionAndLod> old_region_list;
	Meta old_meta = old_stream->_meta;

	// 从旧流获取所有区域的列表
	{
		for (unsigned int lod_index = 0; lod_index < constants::MAX_LOD; ++lod_index) {
			const String lod_folder =
					old_stream->_directory_path.path_join("regions").path_join("lod") + String::num_int64(lod_index);
			const String ext = String(".") + RegionFormat::FILE_EXTENSION;

			Ref<DirAccess> da = open_directory(lod_folder, nullptr);
			if (da.is_null()) {
				continue;
			}

			da->list_dir_begin();

			while (true) {
				String fname = da->get_next();
				if (fname == "") {
					break;
				}
				if (da->current_is_dir()) {
					continue;
				}
				if (fname.ends_with(ext)) {
					PackedStringArray parts = fname.split(".");
					// r.x.y.z.ext
					ERR_FAIL_COND_MSG(
							parts.size() < 4, String("Found invalid region file: '{0}'").format(varray(fname))
					);
					PositionAndLod p;
					p.position.x = parts[1].to_int();
					p.position.y = parts[2].to_int();
					p.position.z = parts[3].to_int();
					p.lod_index = lod_index;
					old_region_list.push_back(p);
				}
			}

			da->list_dir_end();
		}
	}

	_meta = new_meta;
	ERR_FAIL_COND(save_meta() != FILE_OK);

	const Vector3i old_block_size = Vector3iUtil::create(1 << old_meta.block_size_po2);
	const Vector3i new_block_size = Vector3iUtil::create(1 << _meta.block_size_po2);

	const Vector3i old_region_size = Vector3iUtil::create(1 << old_meta.region_size_po2);

	// 从旧流读取所有数据块并写入新流

	for (unsigned int i = 0; i < old_region_list.size(); ++i) {
		PositionAndLod region_info = old_region_list[i];

		const CachedRegion *old_region = old_stream->open_region(region_info.position, region_info.lod_index, false);
		if (old_region == nullptr) {
			continue;
		}

		VOXEL_PRINT_VERBOSE(format("Converting region lod{}/{}", region_info.lod_index, region_info.position));

		const unsigned int blocks_count = old_region->region.get_header_block_count();
		for (unsigned int j = 0; j < blocks_count; ++j) {
			if (!old_region->region.has_block(j)) {
				continue;
			}

			voxel::VoxelBuffer old_block(voxel::VoxelBuffer::ALLOCATOR_POOL);
			old_block.create(old_block_size.x, old_block_size.y, old_block_size.z);

			voxel::VoxelBuffer new_block(voxel::VoxelBuffer::ALLOCATOR_POOL);
			new_block.create(new_block_size.x, new_block_size.y, new_block_size.z);

			// 从旧流加载数据块
			Vector3i block_rpos = old_region->region.get_block_position_from_index(j);
			Vector3i block_pos = block_rpos + region_info.position * old_region_size;
			VoxelStream::VoxelQueryData old_block_load_query{
				old_block, //
				block_pos, //
				region_info.lod_index, //
				RESULT_ERROR //
			};
			old_stream->load_voxel_block(old_block_load_query);

			// 将其保存到新流中
			if (old_block_size == new_block_size) {
				VoxelStream::VoxelQueryData old_block_save_query{
					old_block, //
					block_pos, //
					region_info.lod_index,
					RESULT_ERROR //
				};
				save_voxel_block(old_block_save_query);

			} else {
				Vector3i new_block_pos = convert_block_coordinates(block_pos, old_block_size, new_block_size);

				// TODO 是否支持任意尺寸？这里假定是立方体数据块
				if (old_block_size.x < new_block_size.x) {
					Vector3i ratio = new_block_size / old_block_size;
					Vector3i rel = block_pos % ratio;

					// 拷贝到一个数据块的子区域
					VoxelStream::VoxelQueryData new_block_load_query{
						new_block, new_block_pos, region_info.lod_index, RESULT_ERROR
					};
					load_voxel_block(new_block_load_query);

					Vector3i dst_pos = rel * old_block.get_size();

					for (unsigned int channel_index = 0; channel_index < voxel::VoxelBuffer::MAX_CHANNELS; ++channel_index) {
						new_block.copy_channel_from(
								old_block, Vector3i(), old_block.get_size(), dst_pos, channel_index
						);
					}

					new_block.compress_uniform_channels();
					VoxelStream::VoxelQueryData new_block_save_query{
						new_block, new_block_pos, region_info.lod_index, RESULT_ERROR
					};
					save_voxel_block(new_block_save_query);

				} else {
					// 拷贝到多个数据块
					Vector3i area = new_block_size / old_block_size;
					Vector3i rpos;

					for (rpos.z = 0; rpos.z < area.z; ++rpos.z) {
						for (rpos.x = 0; rpos.x < area.x; ++rpos.x) {
							for (rpos.y = 0; rpos.y < area.y; ++rpos.y) {
								Vector3i src_min = rpos * new_block.get_size();
								Vector3i src_max = src_min + new_block.get_size();

								for (unsigned int channel_index = 0; channel_index < voxel::VoxelBuffer::MAX_CHANNELS;
									 ++channel_index) {
									new_block.copy_channel_from(old_block, src_min, src_max, Vector3i(), channel_index);
								}

								VoxelStream::VoxelQueryData new_block_save_query{
									new_block, new_block_pos + rpos, region_info.lod_index, RESULT_ERROR
								};
								save_voxel_block(new_block_save_query);
							}
						}
					}
				}
			}
		}
	}

	close_all_regions();

	VOXEL_PRINT_VERBOSE("Done converting region files");
}

Vector3i VoxelStreamRegionFiles::get_region_size() const {
	MutexLock lock(_mutex);
	return Vector3iUtil::create(1 << _meta.region_size_po2);
}

Vector3 VoxelStreamRegionFiles::get_region_size_v() const {
	return get_region_size();
}

int VoxelStreamRegionFiles::get_region_size_po2() const {
	MutexLock lock(_mutex);
	return _meta.region_size_po2;
}

int VoxelStreamRegionFiles::get_block_size_po2() const {
	MutexLock lock(_mutex);
	return _meta.block_size_po2;
}

int VoxelStreamRegionFiles::get_lod_count() const {
	return constants::MAX_LOD;
}

int VoxelStreamRegionFiles::get_sector_size() const {
	MutexLock lock(_mutex);
	return _meta.sector_size;
}

// TODO 以下设置很难更改。
// 如果文件已存在，这些设置将被忽略。
// 若要应用这些设置，文件要么需要被清除要么需要被转换，这是非常重的操作。
// 可以通过在检查器中添加一个转换现有文件的按钮来让它更简单，以备不时之需

void VoxelStreamRegionFiles::set_region_size_po2(int p_region_size_po2) {
	{
		MutexLock lock(_mutex);
		if (_meta.region_size_po2 == p_region_size_po2) {
			return;
		}
		ERR_FAIL_COND_MSG(
				_meta_loaded, "Can't change existing region size without heavy conversion. Use convert_files()."
		);
		ERR_FAIL_COND(p_region_size_po2 < 1);
		ERR_FAIL_COND(p_region_size_po2 > 8);
		_meta.region_size_po2 = p_region_size_po2;
	}
	emit_changed();
}

void VoxelStreamRegionFiles::set_block_size_po2(int p_block_size_po2) {
	{
		MutexLock lock(_mutex);
		if (_meta.block_size_po2 == p_block_size_po2) {
			return;
		}
		ERR_FAIL_COND_MSG(
				_meta_loaded, "Can't change existing block size without heavy conversion. Use convert_files()."
		);
		ERR_FAIL_COND(p_block_size_po2 < 1);
		ERR_FAIL_COND(p_block_size_po2 > 8);
		_meta.block_size_po2 = p_block_size_po2;
	}
	emit_changed();
}

void VoxelStreamRegionFiles::set_sector_size(int p_sector_size) {
	{
		MutexLock lock(_mutex);
		if (static_cast<int>(_meta.sector_size) == p_sector_size) {
			return;
		}
		ERR_FAIL_COND_MSG(
				_meta_loaded, "Can't change existing sector size without heavy conversion. Use convert_files()."
		);
		ERR_FAIL_COND(p_sector_size < 256);
		ERR_FAIL_COND(p_sector_size > 65536);
		_meta.sector_size = p_sector_size;
	}
	emit_changed();
}

void VoxelStreamRegionFiles::convert_files(Dictionary d) {
	Meta meta;
	meta.version = _meta.version;
	meta.block_size_po2 = int(d["block_size_po2"]);
	meta.region_size_po2 = int(d["region_size_po2"]);
	meta.sector_size = int(d["sector_size"]);
	// meta.lod_count = int(d["lod_count"]);

	{
		MutexLock lock(_mutex);

		ERR_FAIL_COND_MSG(!check_meta(meta), "Invalid setting");

		if (!_meta_loaded) {
			if (load_meta() != voxel::godot::FILE_OK) {
				// 新流，无需转换
				_meta = meta;

			} else {
				// 刚刚打开了现有流
				_convert_files(meta);
			}

		} else {
			// 该流之前被使用过
			_convert_files(meta);
		}
	}

	emit_changed();
}

void VoxelStreamRegionFiles::flush() {
	VOXEL_PROFILE_SCOPE();
	MutexLock lock(_mutex);
	for (CachedRegion *cr : _region_cache) {
		cr->region.flush();
	}
}

void VoxelStreamRegionFiles::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_directory", "directory"), &VoxelStreamRegionFiles::set_directory);
	ClassDB::bind_method(D_METHOD("get_directory"), &VoxelStreamRegionFiles::get_directory);

	ClassDB::bind_method(D_METHOD("get_block_size_po2"), &VoxelStreamRegionFiles::get_block_size_po2);
	ClassDB::bind_method(D_METHOD("get_region_size"), &VoxelStreamRegionFiles::get_region_size_v);
	ClassDB::bind_method(D_METHOD("get_region_size_po2"), &VoxelStreamRegionFiles::get_region_size_po2);
	ClassDB::bind_method(D_METHOD("get_sector_size"), &VoxelStreamRegionFiles::get_sector_size);

	ClassDB::bind_method(D_METHOD("set_block_size_po2"), &VoxelStreamRegionFiles::set_block_size_po2);
	ClassDB::bind_method(D_METHOD("set_region_size_po2"), &VoxelStreamRegionFiles::set_region_size_po2);
	ClassDB::bind_method(D_METHOD("set_sector_size"), &VoxelStreamRegionFiles::set_sector_size);

	ClassDB::bind_method(D_METHOD("convert_files", "new_settings"), &VoxelStreamRegionFiles::convert_files);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "directory", PROPERTY_HINT_DIR), "set_directory", "get_directory");

	ADD_GROUP("Dimensions", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "region_size_po2"), "set_region_size_po2", "get_region_size_po2");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "block_size_po2"), "set_block_size_po2", "get_block_size_po2");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sector_size"), "set_sector_size", "get_sector_size");
}

} // namespace voxel
