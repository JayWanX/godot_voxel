#ifndef VOXEL_GRAPH_EDITOR_H
#define VOXEL_GRAPH_EDITOR_H

#include "../../generators/graph/voxel_generator_graph.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/control.h"
#include "../../util/godot/classes/editor_undo_redo_manager.h"
#include "../../util/godot/classes/graph_edit_connection.h"
#include "../../util/godot/debug_renderer.h"
#include "../../util/godot/object_weak_ref.h"
#include "../../util/math/vector2f.h"
#include "graph_preview_mode.h"
#include "voxel_graph_editor_node_preview_info.h"

VOXEL_GODOT_NAMESPACE_BEGIN
class GraphEdit;
class PopupMenu;
class AcceptDialog;
class UndoRedo;
class Button;
class Label;
class OptionButton;
class CheckBox;
class MenuButton;
VOXEL_GODOT_NAMESPACE_END

namespace voxel {

class VoxelRangeAnalysisDialog;
class VoxelNode;
class VoxelGraphEditorShaderDialog;
class VoxelGraphNodeDialog;

// 图形编辑器的主 GUI
class VoxelGraphEditor : public Control {
	GDCLASS(VoxelGraphEditor, Control)
public:
	static const char *SIGNAL_NODE_SELECTED;
	static const char *SIGNAL_NOTHING_SELECTED;
	static const char *SIGNAL_NODES_DELETED;
	static const char *SIGNAL_REGENERATE_REQUESTED;
	static const char *SIGNAL_POPOUT_REQUESTED;

	VoxelGraphEditor();

	void set_generator(Ref<VoxelGeneratorGraph> generator);
	inline Ref<VoxelGeneratorGraph> get_generator() const {
		return _generator;
	}

	void set_graph(Ref<pg::VoxelGraphFunction> graph);
	Ref<pg::VoxelGraphFunction> get_graph() const;

	void set_undo_redo(EditorUndoRedoManager *undo_redo);
	EditorUndoRedoManager *get_undo_redo() const;

	void set_voxel_node(VoxelNode *node);

	// 当节点输入数量变化时调用。
	// 重建节点的内部控件，并更新图形中连接到该节点的 GUI 连线。
	void update_node_layout(uint32_t node_id);

	void update_node_comment(uint32_t node_id);

	bool is_pinned_hint() const;
	void set_popout_button_enabled(bool enable);


private:
	void _notification(int p_what);
	void process(float delta);

	void clear();
	void build_gui_from_graph();
	void create_node_gui(uint32_t node_id);
	void remove_node_gui(StringName gui_node_name);
	void set_node_position(int id, Vector2 offset);
	void set_node_size(int id, Vector2 size);

	void schedule_preview_update();
	void update_previews(bool with_live_update);
	StdVector<VoxelGraphEditorNodePreviewInfo> get_slice_previews() const;
	void update_slice_previews();
	void update_range_analysis_previews();
	void update_range_analysis_gizmo();
	void clear_range_analysis_tooltips();
	void hide_profiling_ratios();
	void update_buttons_availability();
	void create_function_node(String fpath);
	void profile();
	void update_preview_axes_menu();
	void update_functions();
	void set_preview_transform(Vector2f offset, float scale);

	void copy_selected_nodes_to_clipboard();
	void paste_clipboard();
	void create_node_gui_input_connections(int node_id);

	void delete_selected_nodes();
	void remove_connection(
			const String from_node_name,
			const int from_slot,
			const String to_node_name,
			const int to_slot
	);

	void _on_graph_edit_gui_input(Ref<InputEvent> event);
	void _on_graph_edit_connection_request(String from_node_name, int from_slot, String to_node_name, int to_slot);
	void _on_graph_edit_disconnection_request(String from_node_name, int from_slot, String to_node_name, int to_slot);

#if defined(VOXEL_GODOT)
	void _on_graph_edit_delete_nodes_request(TypedArray<StringName> node_names);
	void _on_graph_edit_node_selected(Node *p_node);
	void _on_graph_edit_node_deselected(Node *p_node);
#endif

	void _on_menu_id_pressed(int id);
	void _on_graph_node_dragged(Vector2 from, Vector2 to, int id);
	// void _on_context_menu_id_pressed(int id);
	void _on_graph_changed();
	void _on_graph_node_name_changed(int node_id);
	void _on_range_analysis_toggled(bool enabled);
	void _on_range_analysis_area_changed();
	void _on_popout_button_pressed();
	void _on_node_dialog_node_selected(int node_type_id);
	void _on_node_dialog_file_selected(String fpath);
	void _on_node_resize_request(Vector2 new_size, int node_id);
	void _on_graph_node_preview_gui_input(Ref<InputEvent> event);
	void _on_graph_edit_copy_nodes_request();
	void _on_graph_edit_paste_nodes_request();

	void _check_nothing_selected();

	static void _bind_methods();

	Ref<VoxelGeneratorGraph> _generator;
	Ref<pg::VoxelGraphFunction> _graph;

	GraphEdit *_graph_edit = nullptr;
	// PopupMenu *_context_menu = nullptr;
	Label *_profile_label = nullptr;
	Label *_compile_result_label = nullptr;
	Label *_no_graph_open_label = nullptr;
	VoxelRangeAnalysisDialog *_range_analysis_dialog = nullptr;
	// 不拥有。
	// TODO 不确定直接使用 `EditorUndoRedoManager` 是否正确？
	// 当这个管理器取代旧的全局 UndoRedo 时，VisualShader 就是这么做的...
	// 这个类似乎还没有任何文档
	EditorUndoRedoManager *_undo_redo = nullptr;
	Vector2 _click_position;
	bool _nothing_selected_check_scheduled = false;
	float _time_before_preview_update = 0.f;
	voxel::godot::ObjectWeakRef<VoxelNode> _terrain_node;
	voxel::godot::DebugRenderer _debug_renderer;
	VoxelGraphEditorShaderDialog *_shader_dialog = nullptr;
	bool _live_update_enabled = false;
	uint64_t _last_output_graph_hash = 0;
	Button *_pin_button = nullptr;
	Button *_popout_button = nullptr;
	MenuButton *_graph_menu_button = nullptr;
	MenuButton *_debug_menu_button = nullptr;
	PopupMenu *_preview_axes_menu = nullptr;
	VoxelGraphNodeDialog *_node_dialog = nullptr;
	PopupMenu *_context_menu = nullptr;
	voxel::godot::GraphEditConnection _context_connection;

	GraphEditorPreview::ViewMode _node_preview_mode = GraphEditorPreview::VIEW_SLICE_XY;
	Vector2f _preview_offset;
	float _preview_scale = 1.f;

	struct Clipboard {
		Ref<pg::VoxelGraphFunction> graph;
	};

	Clipboard _clipboard;
};

} // namespace voxel

#endif // VOXEL_GRAPH_EDITOR_H
