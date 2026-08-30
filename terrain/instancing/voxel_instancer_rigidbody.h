#ifndef VOXEL_INSTANCER_RIGIDBODY_H
#define VOXEL_INSTANCER_RIGIDBODY_H

#include "../../util/godot/classes/rigid_body_3d.h"
#include "voxel_instancer.h"

namespace voxel {

// 为 VoxelInstancer multimesh 实例提供碰撞
class VoxelInstancerRigidBody : public RigidBody3D {
	GDCLASS(VoxelInstancerRigidBody, RigidBody3D);

public:
	VoxelInstancerRigidBody();

	// 设置所在数据块位置
	void set_data_block_position(Vector3i data_block_position) {
		_data_block_position = data_block_position;
	}

	// 设置渲染块索引
	void set_render_block_index(unsigned int render_block_index) {
		_render_block_index = render_block_index;
	}

	// 设置实例索引
	void set_instance_index(int instance_index) {
		_instance_index = instance_index;
	}

	// 附加到实例化器
	void attach(VoxelInstancer *parent) {
		_parent = parent;
	}

	// 分离并销毁刚体
	void detach_and_destroy() {
		_parent = nullptr;
		queue_free();
	}

	// 获取对应的库项目 ID
	int get_library_item_id() const;

	// 注意，为此刚体必须切换到凸形
	// void detach_and_become_rigidbody() {
	// 	//...
	// }

	// 释放刚体并通知实例化器
	void queue_free_and_notify_instancer();

protected:
	static void _bind_methods();

	void _notification(int p_what);

private:
	VoxelInstancer *_parent = nullptr;
	Vector3i _data_block_position;
	unsigned int _render_block_index;
	int _instance_index = -1;
};

} // namespace voxel

#endif // VOXEL_INSTANCER_RIGIDBODY_H
