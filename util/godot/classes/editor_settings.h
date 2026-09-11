#include <core/version.h>
#include <core/input/shortcut.h>
#ifndef VOXEL_GODOT_EDITOR_SETTINGS_H
#define VOXEL_GODOT_EDITOR_SETTINGS_H


#include <core/version.h>

#include <editor/settings/editor_settings.h>


#include <core/input/shortcut.h>

namespace voxel::godot {

Ref<Shortcut> get_or_create_editor_shortcut(const String &p_path, const String &p_name, Key p_keycode);

} // namespace voxel::godot

#endif // VOXEL_GODOT_EDITOR_SETTINGS_H
