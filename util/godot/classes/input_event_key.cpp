#include "input_event_key.h"
#include "../core/keyboard.h"

namespace voxel::godot {

Ref<InputEventKey> create_input_event_from_key(Key p_keycode_with_modifier_masks, bool p_physical) {
	return InputEventKey::create_reference(p_keycode_with_modifier_masks, p_physical);

}

} // namespace voxel::godot
