#ifndef VOXEL_GODOT_DISPLAY_SERVER_H
#define VOXEL_GODOT_DISPLAY_SERVER_H

#if defined(VOXEL_GODOT)
#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 5
#include <servers/display_server.h>
#else
#include <servers/display/display_server.h>
#endif

#endif

#endif // VOXEL_GODOT_DISPLAY_SERVER_H
