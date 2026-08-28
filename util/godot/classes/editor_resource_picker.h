#ifndef VOXEL_GODOT_EDITOR_RESOURCE_PICKER_H
#define VOXEL_GODOT_EDITOR_RESOURCE_PICKER_H

#if defined(VOXEL_GODOT)

#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 4
#include <editor/editor_resource_picker.h>
#else
#include <editor/inspector/editor_resource_picker.h>
#endif

#endif

#endif // VOXEL_GODOT_EDITOR_RESOURCE_PICKER_H
