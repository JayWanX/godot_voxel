#include "voxel_generator_multipass_editor_plugin.h"

namespace voxel {

VoxelGeneratorMultipassEditorPlugin::VoxelGeneratorMultipassEditorPlugin() {}

void VoxelGeneratorMultipassEditorPlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_ENTER_TREE) {
		_inspector_plugin.instantiate();
		// TODO 为什么其他 Godot 插件可以在构造函数中做这件事？？
		// 我发现我不能把它放在构造函数中，
		// 否则 `add_inspector_plugin` 会导致另一个编辑器插件在退出时泄漏……唉
		add_inspector_plugin(_inspector_plugin);

	} else if (p_what == NOTIFICATION_EXIT_TREE) {
		remove_inspector_plugin(_inspector_plugin);
	}
}

} // namespace voxel
