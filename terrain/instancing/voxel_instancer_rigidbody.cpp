#include "voxel_instancer_rigidbody.h"

#ifdef VOXEL_GODOT
#include "../../util/godot/core/class_db.h"
#endif

namespace voxel {

VoxelInstancerRigidBody::VoxelInstancerRigidBody() {
	set_freeze_mode(RigidBody3D::FREEZE_MODE_STATIC);
	set_freeze_enabled(true);
}

int VoxelInstancerRigidBody::get_library_item_id() const {
	ERR_FAIL_COND_V(_parent == nullptr, -1);
	return _parent->get_library_item_id_from_render_block_index(_render_block_index);
}

void VoxelInstancerRigidBody::_notification(int p_what) {
	switch (p_what) {
		// TODO 优化：当我们退出游戏或销毁世界时也会调用它，
		// 这可能会让操作稍慢，但我不确定能否轻松避免
		case NOTIFICATION_UNPARENTED:
			// 用户可能在游戏中 queue_free() 该节点，
			// 因此我们必须通知实例化器移除 multimesh 实例和指针
			if (_parent != nullptr) {
				_parent->on_body_removed(_data_block_position, _render_block_index, _instance_index);
				_parent = nullptr;
			}
			break;
	}
}

// 此方法的存在是为了绕过无法向同一父节点添加或移除子节点的问题，
// 以防移除行为中需要这样做。但它要求用户显式调用，而不是
// queue_free()。
void VoxelInstancerRigidBody::queue_free_and_notify_instancer() {
	queue_free();
	if (_parent != nullptr) {
		_parent->on_body_removed(_data_block_position, _render_block_index, _instance_index);
		_parent = nullptr;
	}
}

void VoxelInstancerRigidBody::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_library_item_id"), &VoxelInstancerRigidBody::get_library_item_id);
	ClassDB::bind_method(
			D_METHOD("queue_free_and_notify_instancer"), &VoxelInstancerRigidBody::queue_free_and_notify_instancer
	);
}

} // namespace voxel
