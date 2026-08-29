#ifndef VOXEL_GRAPH_EDITOR_WINDOW_H
#define VOXEL_GRAPH_EDITOR_WINDOW_H

#include "../../util/godot/classes/accept_dialog.h"
#include "../../util/godot/classes/button.h"
#include "../../util/godot/editor_scale.h"

namespace voxel {

// TODO 如果不需要被迫使用 AcceptDialog 来创建窗口就好了。
// AcceptDialog 添加了我用不到的东西，但 Window 又过于底层。
class VoxelGraphEditorWindow : public AcceptDialog {
	GDCLASS(VoxelGraphEditorWindow, AcceptDialog)
public:
	VoxelGraphEditorWindow() {
		set_exclusive(false);
		set_close_on_escape(false);
		get_ok_button()->hide();
		set_min_size(Vector2(600, 300) * EDSCALE);
		// 我希望当编辑器获得焦点时窗口仍然保持在编辑器上方。`always_on_top` 是唯一
		// 允许这样做的属性，但它要求 `transient` 为 `false`。没有 `transient`，窗口就不再
		// 被视为子窗口，关闭时也不会把焦点还给编辑器。
		// 所以目前，如果你点击编辑器，窗口会被隐藏到编辑器后面。
		// 如果你想同时看到它们，就只能忍受把弹出的窗口拖出编辑器区域……
		//set_flag(Window::FLAG_ALWAYS_ON_TOP, true);
	}

	// void _notification(int p_what) {
	// 	switch (p_what) {
	// 		case NOTIFICATION_WM_CLOSE_REQUEST:
	// 			call_deferred(SNAME("hide"));
	// 			break;
	// 	}
	// }

	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_GRAPH_EDITOR_WINDOW_H
