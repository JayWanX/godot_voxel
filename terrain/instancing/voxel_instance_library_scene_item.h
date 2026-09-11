#ifndef VOXEL_INSTANCE_LIBRARY_SCENE_ITEM_H
#define VOXEL_INSTANCE_LIBRARY_SCENE_ITEM_H

#include <scene/resources/packed_scene.h>
#include "voxel_instance_library_item.h"

namespace voxel {

class VoxelInstanceLibrarySceneItem : public VoxelInstanceLibraryItem {
	GDCLASS(VoxelInstanceLibrarySceneItem, VoxelInstanceLibraryItem)
public:
	// 用作实例的场景
	void set_scene(Ref<PackedScene> scene);
	Ref<PackedScene> get_scene() const;

private:
	static void _bind_methods();

	Ref<PackedScene> _scene;
};

} // namespace voxel

#endif // VOXEL_INSTANCE_LIBRARY_SCENE_ITEM_H
