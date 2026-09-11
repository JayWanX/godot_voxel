#include <scene/gui/box_container.h>
#ifndef VOXEL_BLOCKY_TYPE_VARIANT_LIST_EDITOR_H
#define VOXEL_BLOCKY_TYPE_VARIANT_LIST_EDITOR_H

#include "../../../meshers/blocky/types/voxel_blocky_type.h"
#include "../../../util/containers/std_vector.h"
#include <scene/gui/box_container.h>

class Label;
class EditorResourcePicker;
class GridContainer;
class EditorInterface;
class EditorUndoRedoManager;

namespace voxel {

// 允许编辑属性组合与关联模型之间的映射。
// 这无法作为常规属性暴露，因此它是一个自定义控件。
class VoxelBlockyTypeVariantListEditor : public VBoxContainer {
	GDCLASS(VoxelBlockyTypeVariantListEditor, VBoxContainer)
public:
	VoxelBlockyTypeVariantListEditor();

	void set_type(Ref<VoxelBlockyType> type);
	void set_editor_interface(EditorInterface *ed);
	void set_undo_redo(EditorUndoRedoManager *undo_redo);

private:
	void update_list();

	void _on_type_changed();
	void _on_model_changed(Ref<VoxelBlockyModel> model, int editor_index);
	void _on_model_picker_selected(Ref<VoxelBlockyModel> model, bool inspect);

	static void _bind_methods();

	Ref<VoxelBlockyType> _type;
	Label *_header_label = nullptr;

	struct VariantEditor {
		Label *key_label = nullptr;
		EditorResourcePicker *resource_picker = nullptr;
		VoxelBlockyType::VariantKey key;
	};

	StdVector<VariantEditor> _variant_editors;
	GridContainer *_grid_container = nullptr;
	EditorInterface *_editor_interface = nullptr;
	EditorUndoRedoManager *_undo_redo = nullptr;
};

} // namespace voxel

#endif // VOXEL_BLOCKY_TYPE_VARIANT_LIST_EDITOR_H
