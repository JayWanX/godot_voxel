#ifndef VOXEL_GODOT_KEYBOARD_H
#define VOXEL_GODOT_KEYBOARD_H


#if defined(VOXEL_GODOT)
#include <core/os/keyboard.h>


namespace godot {
static const KeyModifierMask KEY_CODE_MASK = KeyModifierMask::CODE_MASK;
static const KeyModifierMask KEY_MODIFIER_MASK = KeyModifierMask::MODIFIER_MASK;
static const KeyModifierMask KEY_MASK_CMD_OR_CTRL = KeyModifierMask::CMD_OR_CTRL;
static const KeyModifierMask KEY_MASK_SHIFT = KeyModifierMask::SHIFT;
static const KeyModifierMask KEY_MASK_ALT = KeyModifierMask::ALT;
static const KeyModifierMask KEY_MASK_META = KeyModifierMask::META;
static const KeyModifierMask KEY_MASK_CTRL = KeyModifierMask::CTRL;
static const KeyModifierMask KEY_MASK_KPAD = KeyModifierMask::KPAD;
static const KeyModifierMask KEY_MASK_GROUP_SWITCH = KeyModifierMask::GROUP_SWITCH;

static const Key KEY_NONE = Key::NONE;
static const Key KEY_R = Key::R;
static const Key KEY_UP = Key::UP;
static const Key KEY_DOWN = Key::DOWN;
static const Key KEY_ENTER = Key::ENTER;
}; // namespace godot

#endif

#endif // VOXEL_GODOT_KEYBOARD_H
