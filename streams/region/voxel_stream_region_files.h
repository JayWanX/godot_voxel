#ifndef VOXEL_STREAM_REGION_H
#define VOXEL_STREAM_REGION_H

#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/file_utils.h"
#include "../../util/thread/mutex.h"
#include "../voxel_stream.h"
#include "region_file.h"

namespace voxel {

// TODO 重命名 VoxelStreamRegionForest

// 将数据块加载并保存到文件系统，存储在按世界位置索引的多个区域文件中，位于某个目录下。
// 在这里按相似区域的批次加载和保存数据块更有意义，
// 因为这样可以持续使用相同的文件句柄并避免切换。
// 灵感来自 https://www.seedofandromeda.com/blogs/1-creating-a-region-file-system-for-a-voxel-game
//
// 区域文件不是线程安全的。正因为如此，内部互斥锁往往会把使用限制为仅一个线程。
//
class VoxelStreamRegionFiles : public VoxelStream {
	GDCLASS(VoxelStreamRegionFiles, VoxelStream)
public:
	VoxelStreamRegionFiles();
	~VoxelStreamRegionFiles();

	// 加载单个体素数据块
	void load_voxel_block(VoxelStream::VoxelQueryData &query) override;
	// 保存单个体素数据块
	void save_voxel_block(VoxelStream::VoxelQueryData &query) override;

	// 批量加载体素数据块
	void load_voxel_blocks(Span<VoxelStream::VoxelQueryData> p_blocks) override;
	// 批量保存体素数据块
	void save_voxel_blocks(Span<VoxelStream::VoxelQueryData> p_blocks) override;

	// 获取此数据流中可用的通道掩码
	int get_used_channels_mask() const override;

	// 获取存储目录路径
	String get_directory() const;
	// 设置存储目录路径
	void set_directory(String dirpath);

	// 获取区域大小（数据块数，Vector3i 形式）
	Vector3i get_region_size() const;
	// 获取区域大小（Vector3 形式，用于脚本）
	Vector3 get_region_size_v() const;
	// 获取区域大小的以 2 为底的对数
	int get_region_size_po2() const;

	// 获取扇区大小
	int get_sector_size() const;

	// 获取数据块大小的以 2 为底的对数
	int get_block_size_po2() const override;
	// 获取数据块的细节层级（LOD）数量
	int get_lod_count() const override;

	// 设置数据块大小的以 2 为底的对数
	void set_block_size_po2(int p_block_size_po2);
	// 设置区域大小的以 2 为底的对数
	void set_region_size_po2(int p_region_size_po2);
	// 设置扇区大小
	void set_sector_size(int p_sector_size);

	// 将旧格式的区域文件转换为新元数据格式
	void convert_files(Dictionary d);

	// 强制将待处理数据写入文件系统
	void flush() override;

protected:
	static void _bind_methods();

private:
	struct CachedRegion;

	// TODO 与 VoxelStream::Result 冗余，可能会被替换
	enum EmergeResult { //
		EMERGE_OK,
		EMERGE_OK_FALLBACK,
		EMERGE_FAILED
	};

	EmergeResult _load_block(VoxelBuffer &out_buffer, const Vector3i block_pos, const uint8_t lod);
	void _save_block(const VoxelBuffer &voxel_buffer, const Vector3i block_pos, const uint8_t lod);

	voxel::godot::FileResult save_meta();
	voxel::godot::FileResult load_meta();
	Vector3i get_block_position_from_voxels(const Vector3i &origin_in_voxels) const;
	Vector3i get_region_position_from_blocks(const Vector3i &block_position) const;
	void close_all_regions();
	String get_region_file_path(const Vector3i &region_pos, unsigned int lod) const;
	CachedRegion *open_region(const Vector3i region_pos, unsigned int lod, bool create_if_not_found);
	void close_region(CachedRegion *cache);
	CachedRegion *get_region_from_cache(const Vector3i pos, int lod) const;
	void close_oldest_region();

	struct Meta {
		uint8_t version = -1;
		// uint8_t lod_count = 0;
		uint8_t block_size_po2 = 0; // 一个立方体数据块中有多少个体素
		uint8_t region_size_po2 = 0; // 一个立方体区域中有多少个数据块
		FixedArray<VoxelBuffer::Depth, VoxelBuffer::MAX_CHANNELS> channel_depths;
		uint32_t sector_size = 0; // 数据块存储在该尺寸的整数倍偏移处
	};

	static bool check_meta(const Meta &meta);
	void _convert_files(Meta new_meta);

	// 对数据块请求排序，使查询相同区域的请求被分组在一起
	struct BlockQueryComparator {
		VoxelStreamRegionFiles *self = nullptr;

		// operator<
		_FORCE_INLINE_ bool operator()(
				const VoxelStream::VoxelQueryData &a,
				const VoxelStream::VoxelQueryData &b
		) const {
			if (a.lod_index < b.lod_index) {
				return true;
			} else if (a.lod_index > b.lod_index) {
				return false;
			}
			Vector3i bpos_a = a.position_in_blocks;
			Vector3i bpos_b = b.position_in_blocks;
			Vector3i rpos_a = self->get_region_position_from_blocks(bpos_a);
			Vector3i rpos_b = self->get_region_position_from_blocks(bpos_b);
			return rpos_a < rpos_b;
		}
	};

	// TODO 这并不利于多线程。
	// `VoxelRegionFile` 不是线程安全的，因此我们必须将使用限制为同一时间仅一个线程，阻塞其他线程。
	// 应该进行重构以实现更好的多线程。

	struct CachedRegion {
		Vector3i position;
		int lod = 0;
		bool file_exists = false;
		RegionFile region;
		uint64_t last_opened = 0;
		// uint64_t last_accessed;
	};

	String _directory_path;
	Meta _meta;
	bool _meta_loaded = false;
	bool _meta_saved = false;
	StdVector<CachedRegion *> _region_cache;
	// TODO 添加内存缓存以提高容量。
	unsigned int _max_open_regions = MIN(8, FOPEN_MAX);

	Mutex _mutex;
};

} // namespace voxel

#endif // VOXEL_STREAM_REGION_H
