#ifndef VOXEL_SERVER_UPDATER_H
#define VOXEL_SERVER_UPDATER_H

#include "../util/godot/classes/node.h"

namespace voxel {

// TODO 为让 VoxelEngine 更新而采用的临时手段……需要找到从主循环集成回调的方法！
class VoxelEngineUpdater : public Node {
	GDCLASS(VoxelEngineUpdater, Node)
public:
	~VoxelEngineUpdater();
	static void ensure_existence(SceneTree *st);

protected:
	void _notification(int p_what);

private:
	VoxelEngineUpdater();

	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_SERVER_UPDATER_H
