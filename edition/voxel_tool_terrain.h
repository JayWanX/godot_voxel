#ifndef VOXEL_TOOL_TERRAIN_H
#define VOXEL_TOOL_TERRAIN_H

#include "../util/godot/core/random_pcg.h"
#include "voxel_tool.h"

namespace voxel {

class VoxelTerrain;
class VoxelBlockyLibraryBase;
class VoxelData;

class VoxelToolTerrain : public VoxelTool {
	GDCLASS(VoxelToolTerrain, VoxelTool)
public:
	VoxelToolTerrain();
	VoxelToolTerrain(VoxelTerrain *terrain);

	// 检查指定区域是否可编辑
	bool is_area_editable(const Box3i &box) const override;
	// 对地形执行体素射线检测
	Ref<VoxelRaycastResult> raycast(
			Vector3 p_pos,
			Vector3 p_dir,
			float p_max_distance,
			uint32_t p_collision_mask
	) override;

	// 读取 / 写入指定位置的体素元数据
	void set_voxel_metadata(const Vector3i pos, const Variant &meta) override;
	Variant get_voxel_metadata(const Vector3i pos) const override;

	// 将指定区域的体素复制到目标缓冲区
	void copy(
			const Vector3i pos,
			VoxelBuffer &dst,
			const uint8_t p_channels_mask,
			const bool with_metadata
	) const override;

	// 将缓冲区中的体素粘贴到指定位置
	void paste(Vector3i pos, const VoxelBuffer &src, uint8_t channels_mask) override;

	// 按掩码粘贴缓冲区中的体素
	void paste_masked(
			Vector3i pos,
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

	// 执行盒 / 球 / 路径 / 网格编辑
	void do_box(Vector3i begin, Vector3i end) override;
	void do_sphere(Vector3 center, float radius) override;
	void do_path(Span<const Vector3> positions, Span<const float> radii) override;
#ifdef VOXEL_ENABLE_MESH_SDF
	void do_mesh(const VoxelMeshSDF &mesh_sdf, const Transform3D &transform, const float isolevel) override;
#endif

	// 专用 API

	// 执行半球形编辑
	void do_hemisphere(Vector3 center, float radius, Vector3 flat_direction, float smoothness);

	// 在区域内运行方块随机刻
	void run_blocky_random_tick(
			const AABB voxel_area,
			const int voxel_count,
			const Callable &callback,
			const int block_batch_count,
			const uint32_t tags_mask
	);

	// 遍历区域内所有体素的元数据
	void for_each_voxel_metadata_in_area(AABB voxel_area, const Callable &callback);

	// 获取工具使用的数据格式
	VoxelFormat get_format() const override;

protected:
	uint64_t _get_voxel(Vector3i pos) const override;
	float _get_voxel_f(Vector3i pos) const override;
	void _set_voxel(Vector3i pos, uint64_t v) override;
	void _set_voxel_f(Vector3i pos, float v) override;
	void _post_edit(const Box3i &box) override;

private:
	static void _bind_methods();

	VoxelTerrain *_terrain = nullptr;
	RandomPCG _random;
};

} // namespace voxel

#endif // VOXEL_TOOL_TERRAIN_H
