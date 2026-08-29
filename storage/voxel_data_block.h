#ifndef VOXEL_DATA_BLOCK_H
#define VOXEL_DATA_BLOCK_H

#include "../util/ref_count.h"
#include <cstdint>
#include <memory>

namespace voxel {

class VoxelBuffer;

// 存储体积中一个 chunk 的体素数据。网格和碰撞体是分开存储的。
// 体素数据可以存在，也可以不存在。如果不存在，意味着我们知道该块不包含编辑，
// 可以通过查询生成器获得体素。
// 体素数据也可以作为生成器的缓存存在，以便更廉价地重复查询。
class VoxelDataBlock {
public:
	RefCount viewers;

	VoxelDataBlock() {}

	VoxelDataBlock(unsigned int p_lod_index) : _lod_index(p_lod_index) {}

	VoxelDataBlock(std::shared_ptr<VoxelBuffer> &buffer, unsigned int p_lod_index) :
			_voxels(buffer), _lod_index(p_lod_index) {}

	VoxelDataBlock(VoxelDataBlock &&src) :
			viewers(src.viewers),
			_voxels(std::move(src._voxels)),
			_lod_index(src._lod_index),
			_needs_lodding(src._needs_lodding),
			_modified(src._modified),
			_edited(src._edited) {}

	VoxelDataBlock(const VoxelDataBlock &src) :
			viewers(src.viewers),
			_voxels(src._voxels),
			_lod_index(src._lod_index),
			_needs_lodding(src._needs_lodding),
			_modified(src._modified),
			_edited(src._edited) {}

	VoxelDataBlock &operator=(VoxelDataBlock &&src) {
		viewers = src.viewers;
		_lod_index = src._lod_index;
		_voxels = std::move(src._voxels);
		_needs_lodding = src._needs_lodding;
		_modified = src._modified;
		_edited = src._edited;
		return *this;
	}

	VoxelDataBlock operator=(const VoxelDataBlock &src) {
		viewers = src.viewers;
		_lod_index = src._lod_index;
		_voxels = src._voxels;
		_needs_lodding = src._needs_lodding;
		_modified = src._modified;
		_edited = src._edited;
		return *this;
	}

	inline unsigned int get_lod_index() const {
		return _lod_index;
	}

	// 测试体素数据是否存在。
	// 若为 false，表示该块没有编辑，也不包含缓存的生成数据，
	// 因此我们可以在运行时回退到程序化生成器，或请求一个缓存。
	inline bool has_voxels() const {
		return _voxels != nullptr;
	}

	// 获取体素，期望它们存在
	VoxelBuffer &get_voxels() {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(_voxels != nullptr);
#endif
		return *_voxels;
	}

	// 获取体素，期望它们存在
	const VoxelBuffer &get_voxels_const() const {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(_voxels != nullptr);
#endif
		return *_voxels;
	}

	// 获取体素，期望它们存在
	std::shared_ptr<VoxelBuffer> get_voxels_shared() const {
#ifdef DEBUG_ENABLED
		VOXEL_ASSERT(_voxels != nullptr);
#endif
		return _voxels;
	}

	void set_voxels(const std::shared_ptr<VoxelBuffer> &buffer) {
		VOXEL_ASSERT_RETURN(buffer != nullptr);
		_voxels = buffer;
	}

	void clear_voxels() {
		_voxels = nullptr;
		_edited = false;
	}

	void set_modified(bool modified);

	inline bool is_modified() const {
		return _modified;
	}

	void set_needs_lodding(bool need_lodding) {
		_needs_lodding = need_lodding;
	}

	inline bool get_needs_lodding() const {
		return _needs_lodding;
	}

	inline void set_edited(bool edited) {
		_edited = edited;
	}

	inline bool is_edited() const {
		return _edited;
	}

private:
	// 体素数据。若为 null，表示数据可以通过程序化生成获得。
	std::shared_ptr<VoxelBuffer> _voxels;

	// TODO 在这里存储 lod 索引可能没必要，因为我们反正要先拿到地图才能知道。
	// 目前它可以留在这里，因为由于其他存储的标志和对齐，实际并不占用空间。
	uint8_t _lod_index = 0;

	// 表示自该块被修改以来，需要重新计算 mipmap。
	bool _needs_lodding = false;

	// 表示该块是否与加载时不同（应保存）。
	bool _modified = false;

	// 表示该块是否曾经被编辑过。
	// 若为 `false`，则数据是生成器和修改器的缓存，可以重新生成。
	// 一旦变为 `true`，除非被还原，否则通常不会再回到 `false`。
	bool _edited = false;

	// TODO 优化：设计一种合适的方式为多人游戏实现客户端缓存
	//
	// 表示该块被编辑了多少次。
	// 这允许在多人游戏中实现客户端缓存。
	//
	// 注意：进行客户端缓存时，如果服务器决定将某个块还原为生成器输出，
	// 将版本重置为 0 可能不是个好主意，因为如果客户端有版本 1，它可能与
	// 下次编辑后的"新版本 1"不匹配。所有曾经加入服务器的客户端都必须在
	// 开始从服务器接收块之前了解该还原，
	// 或者需要被告知哪个版本是"生成的"版本。
	// uint32_t _version;

	// 表示是否值得请求更精确的数据版本。
	// 若不值得，则为 `true`。
	// bool _max_lod_hint = false;
};

} // namespace voxel

#endif // VOXEL_DATA_BLOCK_H
