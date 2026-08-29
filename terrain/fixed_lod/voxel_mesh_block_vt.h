#ifndef VOXEL_MESH_BLOCK_VT_H
#define VOXEL_MESH_BLOCK_VT_H

#include "../../util/godot/classes/material.h"
#include "../voxel_mesh_block.h"

namespace voxel {

// 为 `VoxelLodTerrain` 的单个数据块（chunk）存储网格和碰撞体。
// 它不存储体素数据，因为可能使用不同的数据块大小或不同的数据结构。
//
// 注意：当该区域的体素不产生几何体时，这样的数据块也可以不包含网格。例如，
// 它可以用来检查某个区域是否已加载（从而*知道*它应该有或不应该有网格，而不会
// 在线程仍在计算网格时处于未知状态）。
class VoxelMeshBlockVT : public VoxelMeshBlock {
public:
	// 参见 VoxelMesherBlocky。
	// 不幸的是，这必须是一个完全独立的网格实例，因为 Godot 不支持为每个网格表面
	// 设置 `cast_shadow` 模式。这可能对性能产生影响。
	voxel::godot::DirectMeshInstance shadow_occluder;

	RefCount mesh_viewers;
	RefCount collision_viewers;

	// 如果该数据块在 `VoxelTerrain` 的更新列表中则为 true，这样在它被处理之前进行多次编辑
	// 不会将其重复添加
	bool is_in_update_list = false;

	// 如果该数据块曾经被网格化处理过（无论是否有网格），则为 true。
	// 需要它来判断区域在碰撞层面是否已加载。如果游戏直接使用体素进行
	// 碰撞，可能更适合使用 `is_area_editable` 而不使用网格数据块
	bool is_loaded = false;

	VoxelMeshBlockVT(const Vector3i bpos, unsigned int size) : VoxelMeshBlock(bpos) {
		_position_in_voxels = bpos * size;
	}

	void set_world(Ref<World3D> p_world) {
		if (_world != p_world) {
			_world = p_world;

			// 用于更新世界。我把"可见性"替换为"是否存在于世界中"，因为 Godot 3 的剔除性能
			// 很糟糕
			_set_visible(_visible && _parent_visible);

			if (_static_body.is_valid()) {
				_static_body.set_world(*p_world);
			}
		}
	}

	void set_material_override(Ref<Material> material) {
		// 如果网格为空则该值可能无效，我们不会为空网格创建实例
		if (_mesh_instance.is_valid()) {
			_mesh_instance.set_material_override(material);
		}
	}

	void set_mesh(
			Ref<Mesh> mesh,
			GeometryInstance3D::GIMode gi_mode,
			RenderingServerEnums::ShadowCastingSetting shadow_setting,
			int render_layers_mask,
			Ref<Mesh> shadow_occluder_mesh
#ifdef TOOLS_ENABLED
			,
			RenderingServerEnums::ShadowCastingSetting shadow_occluder_mode
#endif
	) {
		if (shadow_occluder_mesh.is_null()) {
			if (shadow_occluder.is_valid()) {
				shadow_occluder.destroy();
			}
		} else {
			if (!shadow_occluder.is_valid()) {
				// 如果实例不存在则创建
				shadow_occluder.create();
				shadow_occluder.set_interpolated(false);
				shadow_occluder.set_render_layers_mask(render_layers_mask);
#ifdef TOOLS_ENABLED
				shadow_occluder.set_cast_shadows_setting(shadow_occluder_mode);
#else
				shadow_occluder.set_cast_shadows_setting(RenderingServerEnums::SHADOW_CASTING_SETTING_SHADOWS_ONLY);
#endif
				set_mesh_instance_visible(shadow_occluder, _visible && _parent_visible);
			}
			shadow_occluder.set_mesh(shadow_occluder_mesh);
		}

		VoxelMeshBlock::set_mesh(mesh, gi_mode, shadow_setting, render_layers_mask);
	}

	void drop_mesh() {
		if (shadow_occluder.is_valid()) {
			shadow_occluder.destroy();
		}
		VoxelMeshBlock::drop_mesh();
	}

	void set_render_layers_mask(int mask) {
		if (shadow_occluder.is_valid()) {
			shadow_occluder.set_render_layers_mask(mask);
		}
		VoxelMeshBlock::set_render_layers_mask(mask);
	}

	void set_visible(bool visible) {
		if (_visible == visible) {
			return;
		}
		_visible = visible;
		_set_visible(_visible && _parent_visible);
	}

	void set_parent_visible(bool parent_visible) {
		if (_parent_visible && parent_visible) {
			return;
		}
		_parent_visible = parent_visible;
		_set_visible(_visible && _parent_visible);
	}

	void set_parent_transform(const Transform3D &parent_transform) {
		VOXEL_PROFILE_SCOPE();

		if (shadow_occluder.is_valid()) {
			const Transform3D local_transform(Basis(), _position_in_voxels);
			const Transform3D world_transform = parent_transform * local_transform;

			if (shadow_occluder.is_valid()) {
				shadow_occluder.set_transform(world_transform);
			}
		}

		VoxelMeshBlock::set_parent_transform(parent_transform);
	}

protected:
	void _set_visible(bool visible) {
		if (shadow_occluder.is_valid()) {
			set_mesh_instance_visible(shadow_occluder, visible);
		}
		VoxelMeshBlock::_set_visible(visible);
	}
};

} // namespace voxel

#endif // VOXEL_MESH_BLOCK_VT_H
