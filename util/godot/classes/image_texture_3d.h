#ifndef VOXEL_GODOT_IMAGE_TEXTURE_3D_H
#define VOXEL_GODOT_IMAGE_TEXTURE_3D_H

#if defined(VOXEL_GODOT)
#include <core/version.h>

#if VERSION_MAJOR == 4 && VERSION_MINOR <= 1
#include <scene/resources/texture.h>
#else
#include <scene/resources/image_texture.h>
#endif

#endif

#include "../core/typed_array.h"

namespace voxel::godot {

Ref<ImageTexture3D> create_image_texture_3d(
		const Image::Format p_format,
		const Vector3i resolution,
		const bool p_mipmaps,
		const TypedArray<Image> &p_data
);

void update_image_texture_3d(ImageTexture3D &p_texture, const TypedArray<Image> p_data);

} // namespace voxel::godot

#endif // VOXEL_GODOT_IMAGE_TEXTURE_3D_H
