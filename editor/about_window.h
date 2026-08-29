#ifndef VOXEL_ABOUT_WINDOW_H
#define VOXEL_ABOUT_WINDOW_H

#include "../util/godot/classes/accept_dialog.h"
#include "../util/godot/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class TextureRect);
VOXEL_GODOT_FORWARD_DECLARE(class RichTextLabel);

namespace voxel {

class VoxelAboutWindow : public AcceptDialog {
	GDCLASS(VoxelAboutWindow, AcceptDialog)
public:
	VoxelAboutWindow();

	// 同一个窗口可能被多个插件显示，因此它在内部只创建一次。
	// 它不能在模块初始化时创建，因为此时编辑器尚未就绪。
	static void create_singleton(Node &base_control);
	static void destroy_singleton();
	static void popup_singleton();

protected:
	void _notification(int p_what);

private:
	void _on_about_rich_text_label_meta_clicked(Variant meta);
	void _on_third_party_list_item_selected(int index);

	static void _bind_methods();

	TextureRect *_icon_texture_rect;
	RichTextLabel *_third_party_rich_text_label;
};

} // namespace voxel

#endif // VOXEL_ABOUT_WINDOW_H
