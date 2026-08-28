#ifndef VOXEL_GODOT_MOUSE_BUTTON_H
#define VOXEL_GODOT_MOUSE_BUTTON_H

#if defined(VOXEL_GODOT)
#include <core/input/input_enums.h>

#define VOXEL_GODOT_MouseButton_NONE MouseButton::NONE
#define VOXEL_GODOT_MouseButton_RIGHT MouseButton::RIGHT
#define VOXEL_GODOT_MouseButton_WHEEL_UP MouseButton::WHEEL_UP
#define VOXEL_GODOT_MouseButton_WHEEL_DOWN MouseButton::WHEEL_DOWN
#define VOXEL_GODOT_MouseButtonMask_MIDDLE MouseButtonMask::MIDDLE

#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/global_constants.hpp>
using namespace godot;

#define VOXEL_GODOT_MouseButton_NONE MouseButton::MOUSE_BUTTON_NONE
#define VOXEL_GODOT_MouseButton_RIGHT MouseButton::MOUSE_BUTTON_RIGHT
#define VOXEL_GODOT_MouseButton_WHEEL_UP MouseButton::MOUSE_BUTTON_WHEEL_UP
#define VOXEL_GODOT_MouseButton_WHEEL_DOWN MouseButton::MOUSE_BUTTON_WHEEL_DOWN
#define VOXEL_GODOT_MouseButtonMask_MIDDLE MouseButtonMask::MOUSE_BUTTON_MASK_MIDDLE

#endif

#endif // VOXEL_GODOT_MOUSE_BUTTON_H
