#include "editor_settings.h"
#include "input_event_key.h"

namespace voxel::godot {

Ref<Shortcut> get_or_create_editor_shortcut(const String &p_path, const String &p_name, Key p_keycode) {
#if defined(VOXEL_GODOT)
	return ED_SHORTCUT(p_path, p_name, p_keycode);

#endif
}

} // namespace voxel::godot
