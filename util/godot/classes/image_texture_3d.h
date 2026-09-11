#ifndef VOXEL_GODOT_IMAGE_TEXTURE_3D_H
#define VOXEL_GODOT_IMAGE_TEXTURE_3D_H

#include <core/version.h>

#include <scene/resources/image_texture.h>


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
