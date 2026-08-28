#ifndef VOXEL_GODOT_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_GODOT_EDITOR_INSPECTOR_PLUGIN_H

#if defined(VOXEL_GODOT)

#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 4
#include <editor/editor_inspector.h>
#else
#include <editor/inspector/editor_inspector.h>
#endif

#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
using namespace godot;
#endif

namespace voxel::godot {

class VOXEL_EditorInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(VOXEL_EditorInspectorPlugin, EditorInspectorPlugin)
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
#elif defined(VOXEL_GODOT_EXTENSION)
	bool _can_handle(Object *p_object) const override;
	void _parse_begin(Object *p_object) override;
	void _parse_end(Object *p_object) override;
	void _parse_group(Object *p_object, const String &p_group) override;
	bool _parse_property(
			Object *p_object,
			Variant::Type p_type,
			const String &p_path,
			PropertyHint p_hint,
			const String &p_hint_text,
			BitField<PropertyUsageFlags> p_usage,
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
	// When compiling with GodotCpp, `_bind_methods` is not optional
	static void _bind_methods() {}
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_EDITOR_INSPECTOR_PLUGIN_H
