#ifndef VOXEL_GRAPH_EDITOR_PLUGIN_H
#define VOXEL_GRAPH_EDITOR_PLUGIN_H

#include "../../generators/graph/voxel_graph_function.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/editor_plugin.h"
#include "../../util/godot/object_weak_ref.h"
#include "../../util/macros.h"
#include "voxel_graph_node_inspector_wrapper.h"

class Button;

namespace voxel {

class VoxelGraphEditor;
class VoxelNode;
class VoxelGraphEditorWindow;
class VoxelGraphEditorIODialog;

class VoxelGraphEditorPlugin : public voxel::godot::Voxel_EditorPlugin {
	GDCLASS(VoxelGraphEditorPlugin, voxel::godot::Voxel_EditorPlugin)
public:
	VoxelGraphEditorPlugin();

	void edit_ios(Ref<pg::VoxelGraphFunction> graph);

private:
	void init();

	bool _voxel_handles(const Object *p_object) const override;
	void _voxel_edit(Object *p_object) override;
	void _voxel_make_visible(bool visible) override;

	void _notification(int p_what);

	void undock_graph_editor();
	void dock_graph_editor();
	void update_graph_editor_window_title();
	void inspect_graph_or_generator(const VoxelGraphEditor &graph_editor);

	void _on_graph_editor_node_selected(uint32_t node_id);
	void _on_graph_editor_nothing_selected();
	void _on_graph_editor_nodes_deleted();
	void _on_graph_editor_regenerate_requested();
	void _on_graph_editor_popout_requested();
	void _on_graph_editor_window_close_requested();
	void _on_generator_changed();
	void _hide_deferred();

	static void _bind_methods();

	VoxelGraphEditor *_graph_editor = nullptr;
	VoxelGraphEditorWindow *_graph_editor_window = nullptr;
	VoxelGraphEditorIODialog *_io_dialog = nullptr;
	Button *_bottom_panel_button = nullptr;
	bool _deferred_visibility_scheduled = false;
	voxel::godot::ObjectWeakRef<VoxelNode> _voxel_node;
	StdVector<Ref<VoxelGraphNodeInspectorWrapper>> _node_wrappers;
	// 针对 Godot 4 新行为的变通方法：
	// 当我们检查一个对象时，Godot 会先在我们的插件上调用 `edit(nullptr)` 和 `make_visible(false)`。
	// 但此插件需要允许检查图形的节点。当选中一个节点时，它告诉 Godot
	// 去检查一个关联对象。
	// 但在新的 `edit(nullptr)` 行为下，插件会清理其 UI，这会把选中的 UI GraphNode 销毁，
	// 导致令人抓狂的崩溃和错误……
	// 由于这归结为插件触发了被检查对象的变更，我们设置一个布尔值来忽略
	// `edit(nullptr)` 调用。
	bool _ignore_edit_null = false;
	bool _ignore_make_visible = false;
};

} // namespace voxel

#endif // VOXEL_GRAPH_EDITOR_PLUGIN_H
