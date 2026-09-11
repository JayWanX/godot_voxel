#include <scene/resources/3d/world_3d.h>
#ifndef VOXEL_MESH_BLOCK_H
#define VOXEL_MESH_BLOCK_H

#include "../constants/cube_tables.h"
#include "../meshers/voxel_mesher.h"
#include "../util/containers/fixed_array.h"
#include "../util/containers/span.h"
#include <core/version.h>
#include <scene/resources/3d/world_3d.h>
#include "../util/godot/direct_mesh_instance.h"
#include "../util/godot/direct_static_body.h"
#include "../util/ref_count.h"

#include <atomic>

class Node3D;
class ConcavePolygonShape3D;

namespace voxel {

// 为一个渲染体积的数据块（chunk）存储网格和碰撞体。
// 它不存储体素数据，因为可能使用不同的数据块大小或不同的数据结构。
// 重要：这不是一个抽象类。它存在的目的是在它的各个变体之间共享公共代码。
// 只使用显式实例，不使用虚函数。
class VoxelMeshBlock : public NonCopyable {
public:
	Vector3i position; // 以区块为单位

protected:
	VoxelMeshBlock(Vector3i bpos);

public:
	~VoxelMeshBlock();

	void set_world(Ref<World3D> p_world);

	// 视觉

	void set_mesh(
			Ref<Mesh> mesh,
			GeometryInstance3D::GIMode gi_mode,
			RenderingServerEnums::ShadowCastingSetting shadow_setting,
			int render_layers_mask
	);
	Ref<Mesh> get_mesh() const;
	bool has_mesh() const;
	void drop_mesh();

	// 注意，GIMode 不按数据块存储，它是一个共享选项，因此在多个函数中提供。
	// 仅当网格块已存在且网格未变化时才调用此函数
	void set_gi_mode(GeometryInstance3D::GIMode mode);

	// 注意，ShadowCastingSetting 不按数据块存储，它是一个共享选项，因此在多个函数中提供。
	// 仅当网格块已存在且网格未变化时才调用此函数
	void set_shadow_casting(RenderingServerEnums::ShadowCastingSetting setting);

	// 注意，渲染层不按数据块存储，它是一个共享选项，因此在多个函数中提供。
	// 仅当网格块已存在且网格未变化时才调用此函数
	void set_render_layers_mask(int mask);

	void set_visible(bool visible);
	bool is_visible() const;

	void set_parent_visible(bool parent_visible);
	void set_parent_transform(const Transform3D &parent_transform);

	// 碰撞

	void set_collision_shape(Ref<Shape3D> shape, bool debug_collision, const Node3D *node, float margin);
	bool has_collision_shape() const;
	void set_collision_layer(int layer);
	void set_collision_mask(int mask);
	void set_collision_margin(float margin);
	void drop_collision();
	// TODO 碰撞层和碰撞掩码

	void set_collision_enabled(bool enable);
	bool is_collision_enabled() const;

protected:
	void _set_visible(bool visible);

	inline void set_mesh_instance_visible(voxel::godot::DirectMeshInstance &mi, bool visible) {
		if (visible) {
			mi.set_world(*_world);
		} else {
			mi.set_world(nullptr);
		}
	}

	Vector3i _position_in_voxels;

	voxel::godot::DirectMeshInstance _mesh_instance;
	voxel::godot::DirectStaticBody _static_body;
	Ref<World3D> _world;

	// 必须与 `active` 的默认值一致
	bool _visible = false;
	bool _collision_enabled = false;

	bool _parent_visible = true;
};

Ref<ConcavePolygonShape3D> make_collision_shape_from_mesher_output(
		const VoxelMesher::Output &mesher_output,
		const VoxelMesher &mesher
);

} // namespace voxel

#endif // VOXEL_MESH_BLOCK_H
