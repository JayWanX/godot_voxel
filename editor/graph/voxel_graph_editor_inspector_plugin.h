#ifndef VOXEL_GRAPH_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_GRAPH_EDITOR_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"

namespace voxel {

// 将检查器的字符串编辑器改为仅在按下回车键时调用 setter，类似 Unreal。
// 因为 `EditorPropertyText` 的默认行为是每次输入字符都调用 setter，这在
// 编辑 Expression 节点时是一场噩梦：随着代码书写，输入会不断变化，
// 这大大增加了破坏现有连接的机会，并且还会产生大量的独立 UndoRedo 操作。
class VoxelGraphEditorInspectorPlugin : public voxel::godot::Voxel_EditorInspectorPlugin {
	GDCLASS(VoxelGraphEditorInspectorPlugin, voxel::godot::Voxel_EditorInspectorPlugin)
protected:
	bool _voxel_can_handle(const Object *obj) const override;
	bool _voxel_parse_property(Object *p_object, const Variant::Type p_type, const String &p_path,
			const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage,
			const bool p_wide = false) override;

private:
	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_GRAPH_EDITOR_INSPECTOR_PLUGIN_H
