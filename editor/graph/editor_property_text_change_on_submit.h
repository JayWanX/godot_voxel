#ifndef VOXEL_EDITOR_PROPERTY_TEXT_CHANGE_ON_SUBMIT_H
#define VOXEL_EDITOR_PROPERTY_TEXT_CHANGE_ON_SUBMIT_H

#include "../../util/godot/classes/editor_property.h"
#include "../../util/godot/macros.h"

class LineEdit;

namespace voxel {

// 检查器的默认字符串编辑器会在每次输入字符时调用被编辑对象的 setter。
// 这并非总是所期望的。相反，这个编辑器应该只在按下回车键或
// 编辑器失去焦点时才发出更改。
// 注意：Godot 针对 LineEdit 的默认字符串编辑器是 `EditorPropertyText`
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
