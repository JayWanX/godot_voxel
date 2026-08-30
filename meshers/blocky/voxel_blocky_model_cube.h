#ifndef VOXEL_BLOCKY_MODEL_CUBE_H
#define VOXEL_BLOCKY_MODEL_CUBE_H

#include "voxel_blocky_model.h"

namespace voxel {

// 立方体模型，每个侧面都有可配置的贴图
// TODO 添加一个实现此功能的新 PrimitiveMesh 并使用 VoxelBlockyMesh 会更好吗？
class VoxelBlockyModelCube : public VoxelBlockyModel {
	GDCLASS(VoxelBlockyModelCube, VoxelBlockyModel)
public:
	VoxelBlockyModelCube();

	// 从名称获取对应的侧面
	static Cube::Side name_to_side(const String &s);

	// 获取或设置指定侧面的贴图坐标
	Vector2i get_tile(VoxelBlockyModel::Side side) const {
		return _tiles[side];
	}

	void set_tile(VoxelBlockyModel::Side side, Vector2i pos);

	// 立方体高度
	void set_height(float h);
	float get_height() const;

	// 图集以方块为单位的尺寸
	void set_atlas_size_in_tiles(Vector2i s);
	Vector2i get_atlas_size_in_tiles() const;

	// 烘焙模型到上下文
	void bake(blocky::ModelBakingContext &ctx) const override;
	bool is_empty() const override;

	// 获取编辑器预览网格
	Ref<Mesh> get_preview_mesh() const override;

	// 将贴图旋转 90 度
	void rotate_tiles_90(const math::Axis axis, const bool clockwise);
	void rotate_tiles_ortho(const math::OrthoBasis ortho_basis);

private:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	static void _bind_methods();

	FixedArray<Vector2i, Cube::SIDE_COUNT> _tiles;
	float _height = 1.f;

	Vector2i _atlas_size_in_tiles;

	uint8_t _mesh_ortho_rotation = 0;
};

void make_cube_side_vertices(StdVector<Vector3f> &positions, const unsigned int side_index, const float height);
void make_cube_side_indices(StdVector<int> &indices, const unsigned int side_index);
void make_cube_side_tangents(StdVector<float> &tangents, const unsigned int side_index);

} // namespace voxel

#endif // VOXEL_BLOCKY_MODEL_CUBE_H
