#ifndef VOXEL_EDITOR_PROPERTY_TEXT_CHANGE_ON_SUBMIT_H
#define VOXEL_EDITOR_PROPERTY_TEXT_CHANGE_ON_SUBMIT_H

#include "../../util/godot/classes/editor_property.h"
#include "../../util/godot/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class LineEdit)

namespace voxel {

// The default string editor of the inspector calls the setter of the edited object on every character typed.
// This is not always desired. Instead, this editor should emit a change only when enter is pressed, or when the
// editor looses focus.
// Note: Godot's default string editor for LineEdit is `EditorPropertyText`
class Voxel_EditorPropertyTextChangeOnSubmit : public voxel::godot::Voxel_EditorProperty {
	GDCLASS(Voxel_EditorPropertyTextChangeOnSubmit, voxel::godot::Voxel_EditorProperty)
public:
	Voxel_EditorPropertyTextChangeOnSubmit();

protected:
	void _voxel_update_property() override;

private:
	void _on_line_edit_focus_entered();
	void _on_line_edit_text_changed(String new_text);
	void _on_line_edit_text_submitted(String text);
	void _on_line_edit_focus_exited();

	static void _bind_methods();

	LineEdit *_line_edit = nullptr;
	bool _ignore_changes = false;
	bool _changed = false;
};

} // namespace voxel

#endif // VOXEL_EDITOR_PROPERTY_TEXT_CHANGE_ON_SUBMIT_H
