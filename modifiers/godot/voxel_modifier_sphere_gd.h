#ifndef VOXEL_MODIFIER_SPHERE_GD_H
#define VOXEL_MODIFIER_SPHERE_GD_H

#include "voxel_modifier_gd.h"

namespace voxel::godot {

class VoxelModifierSphere : public VoxelModifier {
	GDCLASS(VoxelModifierSphere, VoxelModifier);

public:
	// 球体半径
	float get_radius() const;
	void set_radius(float r);

protected:
	voxel::VoxelModifier *create(voxel::VoxelModifierStack &modifiers, uint32_t id) override;

private:
	static void _bind_methods();

	float _radius = 10.f;
};

} // namespace voxel::godot

#endif // VOXEL_MODIFIER_SPHERE_GD_H