#include "editor_inspector_plugin.h"

namespace voxel::godot {

#if defined(VOXEL_GODOT)
bool VOXEL_EditorInspectorPlugin::can_handle(Object *p_object) {
#elif defined(VOXEL_GODOT_EXTENSION)
bool VOXEL_EditorInspectorPlugin::_can_handle(Object *p_object) const {
#endif
	return _voxel_can_handle(p_object);
}

#if defined(VOXEL_GODOT)
void VOXEL_EditorInspectorPlugin::parse_begin(Object *p_object) {
#elif defined(VOXEL_GODOT_EXTENSION)
void VOXEL_EditorInspectorPlugin::_parse_begin(Object *p_object) {
#endif
	_voxel_parse_begin(p_object);
}

#if defined(VOXEL_GODOT)
void VOXEL_EditorInspectorPlugin::parse_end(Object *p_object) {
#elif defined(VOXEL_GODOT_EXTENSION)
void VOXEL_EditorInspectorPlugin::_parse_end(Object *p_object) {
#endif
	_voxel_parse_end(p_object);
}

#if defined(VOXEL_GODOT)
void VOXEL_EditorInspectorPlugin::parse_group(Object *p_object, const String &p_group) {
#elif defined(VOXEL_GODOT_EXTENSION)
void VOXEL_EditorInspectorPlugin::_parse_group(Object *p_object, const String &p_group) {
#endif
	_voxel_parse_group(p_object, p_group);
}

#if defined(VOXEL_GODOT)
bool VOXEL_EditorInspectorPlugin::parse_property(
		Object *p_object,
		const Variant::Type p_type,
		const String &p_path,
		const PropertyHint p_hint,
		const String &p_hint_text,
		const BitField<PropertyUsageFlags> p_usage,
		const bool p_wide
) {
#elif defined(VOXEL_GODOT_EXTENSION)
bool VOXEL_EditorInspectorPlugin::_parse_property(
		Object *p_object,
		Variant::Type p_type,
		const String &p_path,
		PropertyHint p_hint,
		const String &p_hint_text,
		BitField<PropertyUsageFlags> p_usage,
		const bool p_wide
) {
#endif
	return _voxel_parse_property(p_object, p_type, p_path, p_hint, p_hint_text, p_usage, p_wide);
}

bool VOXEL_EditorInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return false;
}

void VOXEL_EditorInspectorPlugin::_voxel_parse_begin(Object *p_object) {}

void VOXEL_EditorInspectorPlugin::_voxel_parse_end(Object *p_object) {}

void VOXEL_EditorInspectorPlugin::_voxel_parse_group(Object *p_object, const String &p_group) {}

bool VOXEL_EditorInspectorPlugin::_voxel_parse_property(
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
