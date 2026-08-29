#ifndef VOXEL_BLOCKY_TYPE_VIEWER_H
#define VOXEL_BLOCKY_TYPE_VIEWER_H

#include "../../../meshers/blocky/types/voxel_blocky_type.h"
#include "../../../util/godot/classes/h_box_container.h"
#include "../model_viewer.h"

VOXEL_GODOT_FORWARD_DECLARE(class MeshInstance3D);

namespace voxel {

class VoxelBlockyTypeAttributeCombinationSelector;

// 专用于检查 blocky 类型的 3D 查看器。
class VoxelBlockyTypeViewer : public Voxel_ModelViewer {
	GDCLASS(VoxelBlockyTypeViewer, Voxel_ModelViewer)
public:
	VoxelBlockyTypeViewer();

	void set_combination_selector(VoxelBlockyTypeAttributeCombinationSelector *selector);
	void set_type(Ref<VoxelBlockyType> type);
	void update_model();

private:
	void _on_type_changed();
	void _on_combination_changed();

	static void _bind_methods();

	Ref<VoxelBlockyType> _type;
	MeshInstance3D *_mesh_instance = nullptr;
	const VoxelBlockyTypeAttributeCombinationSelector *_combination_selector = nullptr;
};

} // namespace voxel

#endif // VOXEL_BLOCKY_TYPE_VIEWER_H
