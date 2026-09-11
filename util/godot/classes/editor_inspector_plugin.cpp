#include "editor_inspector_plugin.h"

namespace voxel::godot {

bool Voxel_EditorInspectorPlugin::can_handle(Object *p_object) {
	return _voxel_can_handle(p_object);
}

void Voxel_EditorInspectorPlugin::parse_begin(Object *p_object) {
	_voxel_parse_begin(p_object);
}

void Voxel_EditorInspectorPlugin::parse_end(Object *p_object) {
	_voxel_parse_end(p_object);
}

void Voxel_EditorInspectorPlugin::parse_group(Object *p_object, const String &p_group) {
	_voxel_parse_group(p_object, p_group);
}

bool Voxel_EditorInspectorPlugin::parse_property(
		Object *p_object,
		const Variant::Type p_type,
		const String &p_path,
		const PropertyHint p_hint,
		const String &p_hint_text,
		const BitField<PropertyUsageFlags> p_usage,
		const bool p_wide
) {
	return _voxel_parse_property(p_object, p_type, p_path, p_hint, p_hint_text, p_usage, p_wide);
}

bool Voxel_EditorInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return false;
}

void Voxel_EditorInspectorPlugin::_voxel_parse_begin(Object *p_object) {}

void Voxel_EditorInspectorPlugin::_voxel_parse_end(Object *p_object) {}

void Voxel_EditorInspectorPlugin::_voxel_parse_group(Object *p_object, const String &p_group) {}

bool Voxel_EditorInspectorPlugin::_voxel_parse_property(
		Object *p_object,
		const Variant::Type p_type,
		const String &p_path,
		const PropertyHint p_hint,
		const String &p_hint_text,
		const BitField<PropertyUsageFlags> p_usage,
		const bool p_wide
) {
	return false;
}

} // namespace voxel::godot
