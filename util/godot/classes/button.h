#ifndef VOXEL_GODOT_BUTTON_H
#define VOXEL_GODOT_BUTTON_H

#include <core/version.h>
#include <scene/gui/button.h>

namespace voxel::godot {

inline void set_button_icon(Button &button, Ref<Texture2D> icon) {

	button.set_button_icon(icon);

}

} // namespace voxel::godot

#endif // VOXEL_GODOT_BUTTON_H
