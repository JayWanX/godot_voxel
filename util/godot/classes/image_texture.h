#ifndef VOXEL_GODOT_IMAGE_TEXTURE_H
#define VOXEL_GODOT_IMAGE_TEXTURE_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 1
#include <scene/resources/texture.h>
#else
#include <scene/resources/image_texture.h>
#endif

#endif

#endif // VOXEL_GODOT_IMAGE_TEXTURE_H
