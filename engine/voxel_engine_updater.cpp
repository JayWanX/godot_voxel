#include "voxel_engine_updater.h"
#include "../util/io/log.h"
#include "voxel_engine.h"

// 执行 `Node *root = SceneTree::get_root()` 所需，Window* 为前置声明
#include "../util/godot/classes/scene_tree.h"
#include "../util/godot/classes/window.h"

namespace voxel {

bool g_updater_created = false;

VoxelEngineUpdater::VoxelEngineUpdater() {
	VOXEL_PRINT_VERBOSE("Creating VoxelEngineUpdater");
	set_process(true);
	// 我们不希望它在场景树暂停时停止
	set_process_mode(PROCESS_MODE_ALWAYS);
	g_updater_created = true;
}

VoxelEngineUpdater::~VoxelEngineUpdater() {
	g_updater_created = false;
}

void VoxelEngineUpdater::ensure_existence(SceneTree *st) {
	if (st == nullptr) {
		return;
	}
	if (g_updater_created) {
		return;
	}
	Node *root = st->get_root();
	for (int i = 0; i < root->get_child_count(); ++i) {
		VoxelEngineUpdater *u = Object::cast_to<VoxelEngineUpdater>(root->get_child(i));
		if (u != nullptr) {
			return;
		}
	}
	VoxelEngineUpdater *u = memnew(VoxelEngineUpdater);
	u->set_name("VoxelEngineUpdater_dont_touch_this");
	// TODO 这可能会失败（例如在 `_ready()` 期间 `Node::data.blocked > 0`），但 Godot 未提供任何 API 来检查。
	// 因此如果失败，节点将泄漏。
	root->add_child(u);

	VoxelEngine::get_singleton().try_initialize_gpu_features();
}

void VoxelEngineUpdater::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS:
			// 为绕开主循环中缺少自定义服务处理 API 的问题
			voxel::VoxelEngine::get_singleton().process();
			break;

		case NOTIFICATION_PREDELETE:
			VOXEL_PRINT_VERBOSE("Deleting VoxelEngineUpdater");
			break;

		default:
			break;
	}
}

} // namespace voxel
