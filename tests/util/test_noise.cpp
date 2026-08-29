#include "test_noise.h"
#include "../../util/godot/core/packed_arrays.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite_range.h"
#include "../../util/noise/spot_noise_gd.h"
#include "../../util/testing/test_macros.h"

namespace voxel::tests {

void test_fnl_range() {
	Ref<Voxel_FastNoiseLite> noise;
	noise.instantiate();
	noise->set_noise_type(Voxel_FastNoiseLite::TYPE_OPEN_SIMPLEX_2S);
	noise->set_fractal_type(Voxel_FastNoiseLite::FRACTAL_NONE);
	// noise->set_fractal_type(Voxel_FastNoiseLite::FRACTAL_FBM);
	noise->set_fractal_octaves(1);
	noise->set_fractal_lacunarity(2.0);
	noise->set_fractal_gain(0.5);
	noise->set_period(512);
	noise->set_seed(0);

	const Vector3i min_pos(-1074, 1838, 5587);
	// const Vector3i max_pos(-1073, 1839, 5588);
	const Vector3i max_pos(-1058, 1854, 5603);

	const math::Interval x_range(min_pos.x, max_pos.x - 1);
	const math::Interval y_range(min_pos.y, max_pos.y - 1);
	const math::Interval z_range(min_pos.z, max_pos.z - 1);

	const math::Interval analytic_range = get_fnl_range_3d(**noise, x_range, y_range, z_range);

	math::Interval empiric_range;
	bool first_value = true;
	for (int z = min_pos.z; z < max_pos.z; ++z) {
		for (int y = min_pos.y; y < max_pos.y; ++y) {
			for (int x = min_pos.x; x < max_pos.x; ++x) {
				const float n = noise->get_noise_3d(x, y, z);
				if (first_value) {
					empiric_range.min = n;
					empiric_range.max = n;
					first_value = false;
				} else {
					empiric_range.min = math::min<real_t>(empiric_range.min, n);
					empiric_range.max = math::max<real_t>(empiric_range.max, n);
				}
			}
		}
	}

	VOXEL_TEST_ASSERT(analytic_range.contains(empiric_range));
}

void test_spot_noise() {
	Ref<Voxel_SpotNoise> noise;
	noise.instantiate();
	const float cell_size = 42.f;
	noise->set_cell_size(cell_size);
	noise->set_jitter(0.6f);
	const Rect2 rect(200.f, -20.f, 100.f, 150.f);
	// 这在 (0,0) 单元格之外不起作用，由 Phoenix 在 Discord 上报告
	const PackedVector2Array positions = noise->get_spot_positions_in_area_2d(rect);

	// 获取我们已知会在该矩形内生成的单元格
	const Vector2i cmin = Vector2i((rect.position / cell_size).ceil());
	const Vector2i cmax = Vector2i(((rect.position + rect.size) / cell_size).floor());
	const Vector2i csize = cmax - cmin;

	const int minimum_expected_spot_count = csize.x * csize.y;
	const int obtained_spot_count = positions.size();
	VOXEL_TEST_ASSERT(obtained_spot_count >= minimum_expected_spot_count);
}

} // namespace voxel::tests
