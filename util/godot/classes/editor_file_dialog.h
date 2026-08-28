#ifndef VOXEL_GODOT_EDITOR_FILE_DIALOG_H
#define VOXEL_GODOT_EDITOR_FILE_DIALOG_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR == 0
#include <editor/editor_file_dialog.h>
#else
#include <editor/gui/editor_file_dialog.h>
#endif

#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/editor_file_dialog.hpp>
using namespace godot;
#endif

namespace voxel::godot {

void popup_file_dialog(EditorFileDialog &dialog);

} // namespace voxel::godot

#endif // VOXEL_GODOT_EDITOR_FILE_DIALOG_H
