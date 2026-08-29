#ifndef VOXEL_DATA_BLOCK_ENTER_INFO_H
#define VOXEL_DATA_BLOCK_ENTER_INFO_H

#include "../storage/voxel_data_block.h"
#include "../util/godot/classes/object.h"

namespace voxel {

namespace godot {
class VoxelBuffer;
}

// 随数据块进入通知发送的信息。
// 它是一个类，以方便脚本 API 的使用。
// 你既不能自己创建这个对象，也不能持有它的引用。
class VoxelDataBlockEnterInfo : public Object {
	GDCLASS(VoxelDataBlockEnterInfo, Object)
public:
	int network_peer_id = -1;
	Vector3i block_position;
	// 数据块的浅拷贝。我们不使用指针，因为线程安全，所以该信息仅代表
	// 数据块被插入映射时的那个时刻。
	VoxelDataBlock voxel_block;

private:
	int _b_get_network_peer_id() const;
	Ref<godot::VoxelBuffer> _b_get_voxels() const;
	Vector3i _b_get_position() const;
	int _b_get_lod_index() const;
	bool _b_are_voxels_edited() const;
	// int _b_viewer_id() const;

	static void _bind_methods();
};

} // namespace voxel

#endif // VOXEL_DATA_BLOCK_ENTER_INFO_H
