#ifndef VOXEL_GODOT_EDITOR_SCALE_H
#define VOXEL_GODOT_EDITOR_SCALE_H

#if defined(VOXEL_GODOT)

#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 2
#include <editor/editor_scale.h>
#else
#include <editor/themes/editor_scale.h>
#endif

#elif defined(VOXEL_GODOT_EXTENSION)
#define EDSCALE voxel::get_editor_scale()
namespace voxel {
float get_editor_scale();
}
#endif

#endif // VOXEL_GODOT_EDITOR_SCALE_H
