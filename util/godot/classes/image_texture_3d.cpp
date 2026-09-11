#include "image_texture_3d.h"
#include "../../profiling.h"

namespace voxel::godot {

Ref<ImageTexture3D> create_image_texture_3d(
		const Image::Format p_format,
		const Vector3i resolution,
		const bool p_mipmaps,
		const TypedArray<Image> &p_data
) {
	VOXEL_PROFILE_SCOPE();

	Ref<ImageTexture3D> texture;
	texture.instantiate();

	Vector<Ref<Image>> images = to_ref_vector(p_data);
	texture->create(p_format, resolution.x, resolution.y, resolution.z, p_mipmaps, images);


	return texture;
}

void update_image_texture_3d(ImageTexture3D &p_texture, const TypedArray<Image> p_data) {
	VOXEL_PROFILE_SCOPE();

	Vector<Ref<Image>> images = to_ref_vector(p_data);
	p_texture.update(images);

}

} // namespace voxel::godot
