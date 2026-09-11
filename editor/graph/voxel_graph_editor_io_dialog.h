#include <scene/gui/dialogs.h>
#include <editor/editor_undo_redo_manager.h>
#ifndef VOXEL_GRAPH_EDITOR_IO_DIALOG_H
#define VOXEL_GRAPH_EDITOR_IO_DIALOG_H

#include "../../generators/graph/voxel_graph_function.h"
#include "../../util/containers/std_vector.h"
#include <scene/gui/dialogs.h>
#include <editor/editor_undo_redo_manager.h>
#include "../../util/godot/macros.h"

class ItemList;
class LineEdit;
class OptionButton;
class SpinBox;
class Button;

namespace voxel {

// 用于编辑 `VoxelGraphFunction` 暴露的输入和输出的对话框
class VoxelGraphEditorIODialog : public ConfirmationDialog {
	GDCLASS(VoxelGraphEditorIODialog, ConfirmationDialog)
public:
	VoxelGraphEditorIODialog();

	void set_graph(Ref<pg::VoxelGraphFunction> graph);
	void set_undo_redo(EditorUndoRedoManager *undo_redo);

private:
	void set_enabled(bool enabled);

	void _on_auto_generate_button_pressed();
	void _on_ok_pressed();

	void reshow(Ref<pg::VoxelGraphFunction> graph);

	void process();

	void _notification(int p_what);

	struct PortsUI {
		ItemList *item_list = nullptr;
		LineEdit *name = nullptr;
		OptionButton *usage = nullptr;
		SpinBox *default_value = nullptr;
		Button *add = nullptr;
		Button *remove = nullptr;
		Button *move_up = nullptr;
		Button *move_down = nullptr;
		int selected_item = -1;
	};

	static Control *create_ui(PortsUI &ui, String title, bool has_default_values);
	static void set_enabled(PortsUI &ui, bool enabled);
	static void clear(PortsUI &ui);
	void copy_ui_to_data(const PortsUI &ui, StdVector<pg::VoxelGraphFunction::Port> &ports);
	void copy_data_to_ui(PortsUI &ui, const StdVector<pg::VoxelGraphFunction::Port> &ports);
	void process_ui(PortsUI &ui, StdVector<pg::VoxelGraphFunction::Port> &ports);

	static void _bind_methods();

	Ref<pg::VoxelGraphFunction> _graph;
	StdVector<pg::VoxelGraphFunction::Port> _inputs;
	StdVector<pg::VoxelGraphFunction::Port> _outputs;

	PortsUI _inputs_ui;
	PortsUI _outputs_ui;
	Button *_auto_generate_button = nullptr;
	EditorUndoRedoManager *_undo_redo = nullptr;
	bool _reshow_on_undo_redo = true;
};

} // namespace voxel

#endif // VOXEL_GRAPH_EDITOR_IO_DIALOG_H
