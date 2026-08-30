#ifndef VOXEL_BLOCKY_MODEL_MESH_H
#define VOXEL_BLOCKY_MODEL_MESH_H

#include "voxel_blocky_model.h"

namespace voxel {

// 使用网格作为视觉效果的模型
class VoxelBlockyModelMesh : public VoxelBlockyModel {
	GDCLASS(VoxelBlockyModelMesh, VoxelBlockyModel)
public:
	// 作为视觉效果的网格
	void set_mesh(Ref<Mesh> mesh);
	Ref<Mesh> get_mesh() const {
		return _mesh;
	}

	// 烘焙模型到上下文
	void bake(blocky::ModelBakingContext &ctx) const override;
	bool is_empty() const override;

	// 获取编辑器预览网格
	Ref<Mesh> get_preview_mesh() const override;

	// 侧面顶点容差，用于邻居侧面剔除判定
	void set_side_vertex_tolerance(float tolerance);
	float get_side_vertex_tolerance() const;

	// 是否启用侧面镂空
	void set_side_cutout_enabled(bool enabled);
	bool is_side_cutout_enabled() const;

private:
	static void _bind_methods();

	Ref<Mesh> _mesh;
	// 体素侧面附近的边距，处于该边距内的三角形将被视为"在侧面上"。这些三角形将被
	// 邻居侧面剔除系统处理。
	float _side_vertex_tolerance = 0.001f;
	bool _side_cutout_enabled = false;
};

void rotate_mesh_arrays(Span<Vector3f> vertices, Span<Vector3f> normals, Span<float> tangents, const Basis3f &basis);

} // namespace voxel

#endif // VOXEL_BLOCKY_MODEL_MESH_H
