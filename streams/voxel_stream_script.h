#ifndef VOXEL_STREAM_SCRIPT_H
#define VOXEL_STREAM_SCRIPT_H

#include "../util/godot/core/gdvirtual.h"
#include "voxel_stream.h"


namespace voxel {

// 提供对分页体素数据源的访问，可加载和保存。
// 必须以多线程安全的方式实现。
// 若你需要更专业的 API 来生成体素，请使用 VoxelGenerator。
class VoxelStreamScript : public VoxelStream {
	GDCLASS(VoxelStreamScript, VoxelStream)
public:
	void load_voxel_block(VoxelStream::VoxelQueryData &q) override;
	void save_voxel_block(VoxelStream::VoxelQueryData &q) override;

	int get_used_channels_mask() const override;

	bool is_runnable() const override;

protected:
	// TODO 为什么即使在 voxel_stream.h 中定义了转换，仍无法将 `Result` 转换为 `Variant`？？？
	GDVIRTUAL3R(int, _load_voxel_block, Ref<godot::VoxelBuffer>, Vector3i, int)
	GDVIRTUAL3(_save_voxel_block, Ref<godot::VoxelBuffer>, Vector3i, int)
	GDVIRTUAL0RC(int, _get_used_channels_mask) // 我想 `C` 表示 `const`？

	static void _bind_methods();
};

} // namespace voxel

#endif // VOXEL_STREAM_SCRIPT_H
