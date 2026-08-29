#ifndef VOXEL_GODOT_DEBUG_RENDERER_H
#define VOXEL_GODOT_DEBUG_RENDERER_H

#include "../containers/std_vector.h"
#include "classes/standard_material_3d.h"
#include "direct_multimesh_instance.h"

namespace voxel::godot {

// 用于每帧绘制 3D 图元以辅助调试
class DebugRenderer {
public:
	DebugRenderer();
	~DebugRenderer();

	// 此类不使用节点。先调用此函数以选择它在哪个世界中渲染。
	void set_world(World3D *world);

	// 在发出绘制命令之前调用
	void begin();

	// 绘制一个盒线框。
	// 盒子的原点位于其下角（最小角）。尺寸由变换的基（basis）决定。
	void draw_box(const Transform3D &t, Color8 color);

	// 在发出所有绘制命令之后调用
	void end();

	void clear();

private:
	void init();
	bool _initialized = false;

	StdVector<DirectMultiMeshInstance::TransformAndColor32> _items;
	Ref<MultiMesh> _multimesh;
	DirectMultiMeshInstance _multimesh_instance;
	// TODO World3D 是一个引用，不要以指针方式保存它
	World3D *_world = nullptr;
	bool _inside_block = false;
	PackedFloat32Array _bulk_array;
	Ref<StandardMaterial3D> _material;
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_DEBUG_RENDERER_H
