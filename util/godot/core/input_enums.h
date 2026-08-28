#ifndef VOXEL_GODOT_INPUT_ENUMS_H
#define VOXEL_GODOT_INPUT_ENUMS_H

#if defined(VOXEL_GODOT)
#include <core/input/input_enums.h>
namespace godot {
static const MouseButton MOUSE_BUTTON_LEFT = ::MouseButton::LEFT;
static const MouseButton MOUSE_BUTTON_RIGHT = ::MouseButton::RIGHT;
} // namespace godot
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/global_constants.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_INPUT_ENUMS_H
