#include "project_settings.h"

namespace voxel::godot {

void add_custom_project_setting(
		Variant::Type type,
		const char *name,
		PropertyHint hint,
		const char *hint_string,
		Variant default_value,
		bool requires_restart
) {
	if (requires_restart) {
		GLOBAL_DEF_RST(name, default_value);
	} else {
		GLOBAL_DEF(name, default_value);
	}
	ProjectSettings::get_singleton()->set_custom_property_info(PropertyInfo(type, name, hint, hint_string));

}

} // namespace voxel::godot
