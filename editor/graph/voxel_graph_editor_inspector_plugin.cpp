#include "voxel_graph_editor_inspector_plugin.h"
#include "editor_property_text_change_on_submit.h"
#include "voxel_graph_node_inspector_wrapper.h"

namespace voxel {

bool VoxelGraphEditorInspectorPlugin::_voxel_can_handle(const Object *obj) const {
	return obj != nullptr && Object::cast_to<VoxelGraphNodeInspectorWrapper>(obj) != nullptr;
}

bool VoxelGraphEditorInspectorPlugin::_voxel_parse_property(Object *p_object, const Variant::Type p_type,
		const String &p_path, const PropertyHint p_hint, const String &p_hint_text,
		const BitField<PropertyUsageFlags> p_usage, const bool p_wide) {
	if (p_type == Variant::STRING && p_hint != PROPERTY_HINT_MULTILINE_TEXT) {
		add_property_editor(p_path, memnew(VOXEL_EditorPropertyTextChangeOnSubmit));
		return true;
	}
	return false;
}

} // namespace voxel
