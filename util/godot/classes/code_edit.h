#ifndef VOXEL_GODOT_CODE_EDIT_H
#define VOXEL_GODOT_CODE_EDIT_H

#if defined(VOXEL_GODOT)
#include <scene/gui/code_edit.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/code_edit.hpp>
using namespace godot;
#endif

#endif // VOXEL_GODOT_CODE_EDIT_H
