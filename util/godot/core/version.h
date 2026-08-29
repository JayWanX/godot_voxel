#ifndef VOXEL_GODOT_VERSION_H
#define VOXEL_GODOT_VERSION_H

#if defined(VOXEL_GODOT)

#include <core/version.h>

// 为了清晰起见，以带前缀的名称暴露 Godot 的版本宏。

#ifndef GODOT_VERSION_MAJOR
#define GODOT_VERSION_MAJOR VERSION_MAJOR
#endif

#ifndef GODOT_VERSION_MINOR
#define GODOT_VERSION_MINOR VERSION_MINOR
#endif

#endif

#endif // VOXEL_GODOT_VERSION_H
