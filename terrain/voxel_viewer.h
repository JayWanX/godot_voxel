#ifndef VOXEL_VIEWER_H
#define VOXEL_VIEWER_H

#include "../engine/ids.h"
#include <scene/3d/node_3d.h>
#include "../util/math/vector2f.h"

namespace voxel {

// 触发其位置周围体素节点的加载。靠近观察者的体素会优先更新。
// 通常添加为玩家相机的子节点。
class VoxelViewer : public Node3D {
	GDCLASS(VoxelViewer, Node3D)
public:
	// 用于触发其位置周围体素节点加载的观察者
	VoxelViewer();

	// 以世界空间单位表示的距离
	void set_view_distance(unsigned int distance);
	unsigned int get_view_distance() const;
	// TODO 碰撞距离

	// 垂直方向的观察距离比例
	void set_view_distance_vertical_ratio(float p_ratio);
	float get_view_distance_vertical_ratio() const;

	// TODO 增加一个在编辑器中运行的选项，可能对测试有用？

	// 是否要求生成网格
	void set_requires_visuals(bool enabled);
	bool is_requiring_visuals() const;

	// 是否要求生成碰撞体
	void set_requires_collisions(bool enabled);
	bool is_requiring_collisions() const;

	// 是否要求数据块进入通知
	void set_requires_data_block_notifications(bool enabled);
	bool is_requiring_data_block_notifications() const;

	// 通过网络访问该观察者的对等体 ID
	void set_network_peer_id(int id);
	int get_network_peer_id() const;

	// 是否在编辑器中生效
	void set_enabled_in_editor(bool enable);
	bool is_enabled_in_editor() const;

protected:
	void _notification(int p_what);

private:
	static void _bind_methods();

	void sync_all_parameters();
	void sync_view_distances();

	// static void unregister_deferred_callback(const ObjectID viewer_node_id, const Vector2i encoded_viewer_id);
	static void unregister_deferred_callback(const int64_t viewer_node_id, const Vector2i encoded_viewer_id);

	bool is_active() const;

	ViewerID _viewer_id;
	unsigned int _view_distance = 128;
	float _view_distance_vertical_ratio = 1.f;
	bool _requires_visuals = true;
	bool _requires_collisions = true;
	bool _requires_data_block_notifications = false;
	bool _enabled_in_editor = false;
	bool _pending_deferred_unregistration = false;
	int _network_peer_id = -1;
};

} // namespace voxel

#endif // VOXEL_VIEWER_H
