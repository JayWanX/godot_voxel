#ifndef VOXEL_GODOT_IMAGE_H
#define VOXEL_GODOT_IMAGE_H

#include <core/io/image.h>

namespace voxel::godot {

inline Ref<Image> create_empty_image(int width, int height, bool mipmaps, Image::Format format) {
	return Image::create_empty(width, height, mipmaps, format);
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_IMAGE_H
