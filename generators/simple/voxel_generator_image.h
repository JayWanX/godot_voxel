#ifndef HEADER_VOXEL_GENERATOR_IMAGE
#define HEADER_VOXEL_GENERATOR_IMAGE

#include "../../util/godot/macros.h"
#include "../../util/thread/rw_lock.h"
#include "voxel_generator_heightmap.h"

VOXEL_GODOT_FORWARD_DECLARE(class Image)

namespace voxel {

// 提供基于图像的无缝平铺高度图
class VoxelGeneratorImage : public VoxelGeneratorHeightmap {
	GDCLASS(VoxelGeneratorImage, VoxelGeneratorHeightmap)

public:
	VoxelGeneratorImage();
	~VoxelGeneratorImage();

	void set_image(Ref<Image> im);
	Ref<Image> get_image() const;

	void set_blur_enabled(bool enable);
	bool is_blur_enabled() const;

	Result generate_block(VoxelGenerator::VoxelQueryData input) override;

private:
	static void _bind_methods();

private:
	// 供外部访问使用的正式引用。
	Ref<Image> _image;

	struct Parameters {
		// 这是图像的只读副本。
		// 它肯定会浪费内存，但 Godot 没有提供更好的方式来保证这一点。
		// 如果某天这成为问题，我们可以添加一个选项，在游戏中对外部图像解除引用。
		Ref<Image> image;
		// 这里主要是作为演示/调整用途。更推荐使用 EXR/浮点图像。
		bool blur_enabled = false;
	};

	Parameters _parameters;
	RWLock _parameters_lock;
};

} // namespace voxel

#endif // HEADER_VOXEL_GENERATOR_IMAGE
