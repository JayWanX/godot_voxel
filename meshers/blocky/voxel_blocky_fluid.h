#ifndef VOXEL_BLOCKY_FLUID_H
#define VOXEL_BLOCKY_FLUID_H

#include "../../constants/cube_tables.h"
#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/resource.h"
#include "../../util/math/vector3f.h"
#include "blocky_baked_library.h"
#include <cstdint>

VOXEL_GODOT_FORWARD_DECLARE(class Material);

namespace voxel {

namespace blocky {
struct MaterialIndexer;
}

// Minecraft 风格的流体通用配置。
// 与普通模型相比，流体有些特殊。用预计算模型渲染它们需要太多模型。
// 因此，它们是在网格化过程中程序化生成的。
// 它们只需要单独的模型来表示液位，或其他状态（如下落）。
class VoxelBlockyFluid : public Resource {
	GDCLASS(VoxelBlockyFluid, Resource)
public:
	enum FlowState : uint8_t {
		// o---x
		// |
		// z
		// 这些值正比于角度，并以俯视 OpenGL 坐标系命名。
		FLOW_STRAIGHT_POSITIVE_X,
		FLOW_DIAGONAL_POSITIVE_X_NEGATIVE_Z,
		FLOW_STRAIGHT_NEGATIVE_Z,
		FLOW_DIAGONAL_NEGATIVE_X_NEGATIVE_Z,
		FLOW_STRAIGHT_NEGATIVE_X,
		FLOW_DIAGONAL_NEGATIVE_X_POSITIVE_Z,
		FLOW_STRAIGHT_POSITIVE_Z,
		FLOW_DIAGONAL_POSITIVE_X_POSITIVE_Z,
		FLOW_IDLE,
		FLOW_STATE_COUNT
	};

	VoxelBlockyFluid();

	void set_material(Ref<Material> material);
	Ref<Material> get_material() const;

	void set_dip_when_flowing_down(bool enable);
	bool get_dip_when_flowing_down() const;

	void bake(blocky::BakedFluid &baked_fluid, blocky::MaterialIndexer &materials) const;

private:
	static void _bind_methods();

	Ref<Material> _material;
	bool _dip_when_flowing_down = false;
};

} // namespace voxel

#endif // VOXEL_BLOCKY_FLUID_H
