#ifndef VOXEL_GODOT_EDITOR_SETTINGS_H
#define VOXEL_GODOT_EDITOR_SETTINGS_H

#if defined(VOXEL_GODOT)

#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 4
#include <editor/editor_settings.h>
#else
#include <editor/settings/editor_settings.h>
#endif

#endif

#include "shortcut.h"

namespace voxel::godot {

Ref<Shortcut> get_or_create_editor_shortcut(const String &p_path, const String &p_name, Key p_keycode);

} // namespace voxel::godot

#endif // VOXEL_GODOT_EDITOR_SETTINGS_H
