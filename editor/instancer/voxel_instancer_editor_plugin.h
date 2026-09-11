#ifndef VOXEL_INSTANCER_EDITOR_PLUGIN_H
#define VOXEL_INSTANCER_EDITOR_PLUGIN_H

#include "../../util/godot/classes/editor_plugin.h"
#include "../../util/godot/macros.h"

class MenuButton;

namespace voxel {

class VoxelInstancer;
class VoxelInstancerStatView;

class VoxelInstancerEditorPlugin : public voxel::godot::Voxel_EditorPlugin {
	GDCLASS(VoxelInstancerEditorPlugin, voxel::godot::Voxel_EditorPlugin)
public:
	VoxelInstancerEditorPlugin();

protected:
	bool _voxel_handles(const Object *p_object) const override;
	void _voxel_edit(Object *p_object) override;
	void _voxel_make_visible(bool visible) override;

private:
	void init();
	void _notification(int p_what);
	bool toggle_stat_view();
	void _on_menu_item_selected(int id);

	VoxelInstancer *get_instancer();

	static void _bind_methods();

	MenuButton *_menu_button = nullptr;
	// 使用 ObjectID 来引用，因为不断检查指针有效性是一场无休止的挣扎。
	// 在选中节点时关闭场景，Godot 会在删除了所有节点之后才调用 `make_visible(false)` 和 `edit(null)`，
	// 这意味着到需要
	// 关闭节点调试绘制功能的时候，本插件会留下一个悬空指针...
	ObjectID _instancer_object_id = ObjectID();
	VoxelInstancerStatView *_stat_view = nullptr;
};

} // namespace voxel

#endif // VOXEL_INSTANCER_EDITOR_PLUGIN_H
