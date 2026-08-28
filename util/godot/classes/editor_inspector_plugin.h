#ifndef VOXEL_GODOT_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_GODOT_EDITOR_INSPECTOR_PLUGIN_H

#if defined(VOXEL_GODOT)

#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 4
#include <editor/editor_inspector.h>
#else
#include <editor/inspector/editor_inspector.h>
#endif

#endif

namespace voxel::godot {

class Voxel_EditorInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(Voxel_EditorInspectorPlugin, EditorInspectorPlugin)
public:
#if defined(VOXEL_GODOT)
	bool can_handle(Object *p_object) override;
	void parse_begin(Object *p_object) override;
	void parse_end(Object *p_object) override;
	void parse_group(Object *p_object, const String &p_group) override;
	bool parse_property(
			Object *p_object,
			const Variant::Type p_type,
			const String &p_path,
			const PropertyHint p_hint,
			const String &p_hint_text,
			const BitField<PropertyUsageFlags> p_usage,
			const bool p_wide = false
	) override;
#endif

protected:
	virtual bool _voxel_can_handle(const Object *p_object) const;
	virtual void _voxel_parse_begin(Object *p_object);
	virtual void _voxel_parse_end(Object *p_object);
	virtual void _voxel_parse_group(Object *p_object, const String &p_group);
	virtual bool _voxel_parse_property(
			Object *p_object,
			const Variant::Type p_type,
			const String &p_path,
			const PropertyHint p_hint,
			const String &p_hint_text,
			const BitField<PropertyUsageFlags> p_usage,
			const bool p_wide
	);

private:
	static void _bind_methods() {}
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_EDITOR_INSPECTOR_PLUGIN_H
