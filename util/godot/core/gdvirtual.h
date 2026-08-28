#ifndef VOXEL_GODOT_GDVIRTUAL_H
#define VOXEL_GODOT_GDVIRTUAL_H

#if defined(VOXEL_GODOT)
#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 1
#include <core/object/script_language.h>
#endif

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 6
#include <core/object/gdvirtual.gen.inc>
#else
#include <core/object/gdvirtual.gen.h>
#endif

#endif

#endif // VOXEL_GODOT_GDVIRTUAL_H
