#ifndef VOXEL_GODOT_EDITOR_INSPECTOR_H
#define VOXEL_GODOT_EDITOR_INSPECTOR_H

#if defined(VOXEL_GODOT)

#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 4
#include <editor/editor_inspector.h>
#else
#include <editor/inspector/editor_inspector.h>
#endif

#endif

#endif // VOXEL_GODOT_EDITOR_INSPECTOR_H
