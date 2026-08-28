#ifndef VOXEL_GODOT_IMAGE_H
#define VOXEL_GODOT_IMAGE_H

#if defined(VOXEL_GODOT)
#include <core/io/image.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/image.hpp>
using namespace godot;
#endif

namespace voxel::godot {

inline Ref<Image> create_empty_image(int width, int height, bool mipmaps, Image::Format format) {
#if defined(VOXEL_GODOT)
	return Image::create_empty(width, height, mipmaps, format);
#elif defined(VOXEL_GODOT_EXTENSION)
	return Image::create(width, height, mipmaps, format);
#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_IMAGE_H
