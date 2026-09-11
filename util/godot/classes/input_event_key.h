#ifndef VOXEL_GODOT_INPUT_EVENT_KEY_H
#define VOXEL_GODOT_INPUT_EVENT_KEY_H

#include <core/input/input_event.h>

namespace voxel::godot {

Ref<InputEventKey> create_input_event_from_key(Key p_keycode_with_modifier_masks, bool p_physical = false);

} // namespace voxel::godot

#endif // VOXEL_GODOT_INPUT_EVENT_KEY_H
