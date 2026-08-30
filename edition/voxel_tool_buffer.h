#ifndef VOXEL_TOOL_BUFFER_H
#define VOXEL_TOOL_BUFFER_H

#include "voxel_tool.h"

namespace voxel {

class VoxelToolBuffer : public VoxelTool {
	GDCLASS(VoxelToolBuffer, VoxelTool)
public:
	VoxelToolBuffer() {}
	VoxelToolBuffer(Ref<godot::VoxelBuffer> vb);

	// 检查指定区域是否可编辑
	bool is_area_editable(const Box3i &box) const override;
	// 将缓冲区中的体素粘贴到指定位置
	void paste(Vector3i p_pos, const VoxelBuffer &src, uint8_t channels_mask) override;
	// 按掩码粘贴缓冲区中的体素
	void paste_masked(
			Vector3i p_pos,
			Ref<godot::VoxelBuffer> p_voxels,
			uint8_t channels_mask,
			uint8_t mask_channel,
			uint64_t mask_value
	) override;

	// 按掩码粘贴，仅写入可写列表中的位置
	void paste_masked_writable_list(
			Vector3i pos,
			Ref<godot::VoxelBuffer> p_voxels,
			uint8_t channels_mask,
			uint8_t src_mask_channel,
			uint64_t src_mask_value,
			uint8_t dst_mask_channel,
			PackedInt32Array dst_writable_list
	) override;

	// 读取 / 写入指定位置的体素元数据
	void set_voxel_metadata(const Vector3i pos, const Variant &meta) override;
	Variant get_voxel_metadata(const Vector3i pos) const override;

	// 执行球 / 盒 / 路径编辑
	void do_sphere(Vector3 center, float radius) override;
	void do_box(Vector3i begin, Vector3i end) override;
	void do_path(Span<const Vector3> positions, Span<const float> radii) override;

protected:
	uint64_t _get_voxel(Vector3i pos) const override;
	float _get_voxel_f(Vector3i pos) const override;
	void _set_voxel(Vector3i pos, uint64_t v) override;
	void _set_voxel_f(Vector3i pos, float v) override;
	void _post_edit(const Box3i &box) override;

private:
	static void _bind_methods() {}

	Ref<godot::VoxelBuffer> _buffer;
};

} // namespace voxel

#endif // VOXEL_TOOL_BUFFER_H
