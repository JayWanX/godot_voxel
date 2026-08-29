#ifndef VOXEL_TOOL_MULTIPASS_GENERATOR_H
#define VOXEL_TOOL_MULTIPASS_GENERATOR_H

#include "../../edition/voxel_tool.h"
#include "voxel_generator_multipass_cb_structs.h"

namespace voxel {

// 在多 pass 生成器生成数据块列的上下文中，提供给脚本的访问器。
// 它不应同时被多个线程使用。
// 脚本不允许在传入它的方法之外持有对它的引用（遗憾的是 Godot 不提供
// 防止此行为的机制）
class VoxelToolMultipassGenerator : public VoxelTool {
	GDCLASS(VoxelToolMultipassGenerator, VoxelTool)
public:
	void set_pass_input(VoxelGeneratorMultipassCBStructs::PassInput &pass_input);

	// VoxelTool 方法

	void copy(
			const Vector3i pos,
			VoxelBuffer &dst,
			const uint8_t channels_mask,
			const bool with_metadata
	) const override;

	void paste(Vector3i pos, const VoxelBuffer &src, uint8_t channels_mask) override;

	void paste_masked(
			Vector3i pos,
			Ref<godot::VoxelBuffer> p_voxels,
			uint8_t channels_mask,
			uint8_t mask_channel,
			uint64_t mask_value
	) override;

	void paste_masked_writable_list(
			Vector3i pos,
			Ref<godot::VoxelBuffer> p_voxels,
			uint8_t channels_mask,
			uint8_t src_mask_channel,
			uint64_t src_mask_value,
			uint8_t dst_mask_channel,
			PackedInt32Array dst_mask_values
	) override;

	bool is_area_editable(const Box3i &box) const override;

	void do_path(Span<const Vector3> positions, Span<const float> radii) override;

	void set_voxel_metadata(const Vector3i pos, const Variant &meta) override;
	Variant get_voxel_metadata(const Vector3i pos) const override;

	// TODO 实现更多方法

	// 特有方法

	Vector3i get_editable_area_min() const;
	Vector3i get_editable_area_max() const;

	Vector3i get_main_area_min() const;
	Vector3i get_main_area_max() const;

	// 调试

	// 创建一个用于测试的独立实例。
	// static Ref<VoxelToolMultipassGenerator> create_offline(
	// 		Vector3i grid_origin_blocks, Vector3i grid_size_blocks, Vector3i main_block_position, int block_size_po2);

protected:
	uint64_t _get_voxel(Vector3i pos) const override;
	float _get_voxel_f(Vector3i pos) const override;
	void _set_voxel(Vector3i pos, uint64_t v) override;
	void _set_voxel_f(Vector3i pos, float v) override;
	void _post_edit(const Box3i &box) override;

	static void _bind_methods();

private:
	VoxelGeneratorMultipassCBStructs::Block *get_block_and_relative_position(
			Vector3i terrain_voxel_pos,
			Vector3i &out_voxel_rpos
	) const;

	VoxelGeneratorMultipassCBStructs::PassInput _pass_input;
	int _block_size_po2 = 0;
	int _block_size_mask = 0;
	Box3i _editable_voxel_box;

	// "offline" 意味着该类使用自己的存储，用于测试目的。
	// bool _is_offline = false;
	// StdVector<VoxelGeneratorMultipassCBStructs::Block> _offline_blocks;
	// StdVector<VoxelGeneratorMultipassCBStructs::Block *> _offline_block_pointers;
};

} // namespace voxel

#endif // VOXEL_TOOL_MULTIPASS_GENERATOR_H
