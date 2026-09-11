#include <core/math/random_pcg.h>
#ifndef VOXEL_TOOL_LOD_TERRAIN_H
#define VOXEL_TOOL_LOD_TERRAIN_H

#include <core/math/random_pcg.h>
#include "../util/macros.h"
#include "voxel_tool.h"

class Node;

namespace voxel {

class VoxelLodTerrain;
class VoxelDataMap;
class VoxelGeneratorGraph;

class VoxelToolLodTerrain : public VoxelTool {
	GDCLASS(VoxelToolLodTerrain, VoxelTool)
public:
	VoxelToolLodTerrain() {}
	VoxelToolLodTerrain(VoxelLodTerrain *terrain);

	// 检查指定区域是否可编辑
	bool is_area_editable(const Box3i &box) const override;
	// 对 LOD 地形执行体素射线检测
	Ref<VoxelRaycastResult> raycast(Vector3 pos, Vector3 dir, float max_distance, uint32_t collision_mask) override;
	// 执行盒 / 球 / 路径 / 网格编辑
	void do_box(Vector3i begin, Vector3i end) override;
	void do_sphere(Vector3 center, float radius) override;
	void do_path(Span<const Vector3> positions, Span<const float> radii) override;
#ifdef VOXEL_ENABLE_MESH_SDF
	void do_mesh(const VoxelMeshSDF &mesh_sdf, const Transform3D &transform, const float isolevel) override;
#endif
	// 将指定区域的体素复制到目标缓冲区
	void copy(
			const Vector3i pos,
			VoxelBuffer &dst,
			const uint8_t p_channels_mask,
			const bool with_metadata
	) const override;
	// 将缓冲区中的体素粘贴到指定位置
	void paste(Vector3i pos, const VoxelBuffer &src, uint8_t channels_mask) override;

	// 读取 / 写入指定位置的体素元数据
	void set_voxel_metadata(const Vector3i pos, const Variant &meta) override;
	Variant get_voxel_metadata(const Vector3i pos) const override;

	// 专用 API

	// 射线检测的二分搜索迭代次数
	int get_raycast_binary_search_iterations() const;
	void set_raycast_binary_search_iterations(int iterations);
	// 异步执行球形编辑
	void do_sphere_async(Vector3 center, float radius);
	// 执行半球形编辑
	void do_hemisphere(Vector3 center, float radius, Vector3 flat_direction, float smoothness);
	// 读取指定位置插值后的浮点体素值
	float get_voxel_f_interpolated(Vector3 position) const override;

	// 分离漂浮的体素块（用于浮空方块物理）
	Array separate_floating_chunks(AABB world_box, Node *parent_node);

#ifdef VOXEL_ENABLE_MESH_SDF
	// 用网格 SDF 印章雕刻地形
	void stamp_sdf(Ref<VoxelMeshSDF> mesh_sdf, Transform3D transform, float isolevel, float sdf_scale);
#endif

	// 使用生成图执行编辑
	void do_graph(Ref<VoxelGeneratorGraph> graph, Transform3D transform, Vector3 area_size);

	// 在区域内运行方块随机刻
	void run_blocky_random_tick(
			const AABB voxel_area,
			const int voxel_count,
			const Callable &callback,
			const int block_batch_count,
			const uint32_t tags_mask
	);

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

	VoxelLodTerrain *_terrain = nullptr;
	int _raycast_binary_search_iterations = 0;
	RandomPCG _random;
};

} // namespace voxel

#endif // VOXEL_TOOL_LOD_TERRAIN_H
