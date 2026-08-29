#ifndef VOXEL_INSTANCE_COMPONENT_H
#define VOXEL_INSTANCE_COMPONENT_H

#include "../../util/godot/classes/node.h"
#include "voxel_instancer.h"

namespace voxel {

// 用作使用 VoxelInstancer 实例化的场景项的子节点。
//
// 之所以需要它，是因为此类实例与 VoxelInstancer 中的一些逻辑绑定在一起。
// 场景的根节点可以是任意 Node3D 派生类，
// 因此在根节点上使用继承来提供 API 不切实际。
// 因此改为采用组件方式。
// 如果需要大量实例，请优先使用 fast/multimesh 实例。
class VoxelInstanceComponent : public Node {
	GDCLASS(VoxelInstanceComponent, Node)
public:
	void mark_modified() {
		ERR_FAIL_COND(_instancer == nullptr);
		_instancer->on_scene_instance_modified(_data_block_position, _render_block_index);
	}

	void detach() {
		ERR_FAIL_COND_MSG(_instancer == nullptr, "Already detached");
		_instancer = nullptr;
	}

	void attach(VoxelInstancer *instancer) {
		ERR_FAIL_COND_MSG(_instancer != nullptr, "Already attached");
		_instancer = instancer;
	}

	// TODO 需要调查我们是否真的需要它
	//
	// 如果用户希望实例化器记住一次移除，则必须从脚本中调用此方法。
	// 在根节点（可能是刚体或区域）上调用 `queue_free()` 可能很常见，但从子节点似乎
	// 没有可靠的方法来检测这种情况是否发生。
	// `_exit_tree` 可能因不同原因而触发。
	// `unparented` 不会发生，因为被解除父节点的是父节点而不是子节点。
	void detach_as_removed() {
		ERR_FAIL_COND_MSG(_instancer == nullptr, "Already detached");
		_instancer->on_scene_instance_removed(_data_block_position, _render_block_index, _instance_index);
		_instancer = nullptr;
	}

	Variant serialize_state() {
		// TODO 脚本
		return Variant();
	}

	Variant deserialize_state() {
		// TODO 脚本
		return Variant();
	}

	void set_instance_index(int instance_index) {
		_instance_index = instance_index;
	}

	void set_data_block_position(Vector3i data_block_position) {
		_data_block_position = data_block_position;
	}

	void set_render_block_index(unsigned int render_block_index) {
		_render_block_index = render_block_index;
	}

	static VoxelInstanceComponent *find_in(Node *root) {
		ERR_FAIL_COND_V(root == nullptr, nullptr);
		for (int i = 0; i < root->get_child_count(); ++i) {
			Node *child = root->get_child(i);
			VoxelInstanceComponent *cmp = Object::cast_to<VoxelInstanceComponent>(child);
			if (cmp != nullptr) {
				return cmp;
			}
		}
		return nullptr;
	}

protected:
	void _notification(int p_what) {
		switch (p_what) {
				// case NOTIFICATION_PARENTED:
				// 	Spatial *spatial = Object::cast_to<Spatial>(get_parent());
				// 	if (spatial == nullptr) {
				// 		ERR_PRINT("VoxelInstanceComponent must have a parent derived from Spatial");
				// 	}
				// 	break;

			// TODO 优化：当我们退出游戏或销毁世界时也会调用它，
			// 这可能会让操作稍慢，但我不确定能否轻松避免
			case NOTIFICATION_UNPARENTED:
				// 用户可能会出于某种原因在游戏中 queue_free() 该节点或其父节点，
				// 因此我们必须通知实例化器移除该实例
				if (_instancer != nullptr) {
					detach_as_removed();
				}
				break;
		}
	}

private:
	static void _bind_methods() {
		// TODO 脚本
	}

	VoxelInstancer *_instancer = nullptr;
	Vector3i _data_block_position;
	unsigned int _render_block_index;
	int _instance_index = -1;
};

} // namespace voxel

#endif // VOXEL_INSTANCE_COMPONENT_H
