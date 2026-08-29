#include "test_fast_noise_2.h"
#include "../../util/godot/classes/image.h"
#include "../../util/io/log.h"
#include "../../util/noise/fast_noise_2.h"
#include "../../util/string/format.h"

namespace voxel::tests {

void test_fast_noise_2_basic() {
	// 非常基础的测试。目的是确保它不会崩溃，因此没有需要检查的特殊条件。
	Ref<FastNoise2> noise;
	noise.instantiate();
	float nv = noise->get_noise_2d_single(Vector2(42, 666));
	print_line(format("SIMD level: {}", FastNoise2::get_simd_level_name_c_str(noise->get_simd_level())));
	print_line(format("Noise: {}", nv));
	Ref<Image> im = godot::create_empty_image(256, 256, false, Image::FORMAT_RGB8);
	noise->generate_image(im, false);
	// im->save_png("voxel_test_fastnoise2.png");
}

void test_fast_noise_2_empty_encoded_node_tree() {
	Ref<FastNoise2> noise;
	noise.instantiate();
	noise->set_noise_type(FastNoise2::TYPE_ENCODED_NODE_TREE);
	// 这可能会打印一个错误，但不应该崩溃
	noise->update_generator();
}

} // namespace voxel::tests
