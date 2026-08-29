#include "test_edition_funcs.h"
#include "../../edition/funcs.h"
#include "../../edition/voxel_tool_terrain.h"
#include "../../generators/graph/voxel_generator_graph.h"
#include "../../meshers/blocky/voxel_blocky_library.h"
#include "../../meshers/blocky/voxel_blocky_model_cube.h"
#include "../../meshers/blocky/voxel_blocky_model_mesh.h"
#include "../../storage/voxel_data.h"
#include "../../util/godot/classes/image.h"
#include "../../util/testing/test_macros.h"
#include "test_util.h"

namespace voxel::tests {

void test_run_blocky_random_tick_with_params(const Box3i voxel_box, const int voxel_count, const int batch_count) {
	// 创建带有可 tick 化体素的库
	Ref<VoxelBlockyLibrary> library;
	library.instantiate();

	{
		Ref<VoxelBlockyModelMesh> air;
		air.instantiate();
		library->add_model(air);
	}

	{
		Ref<VoxelBlockyModelCube> non_tickable;
		non_tickable.instantiate();
		library->add_model(non_tickable);
	}

	int tickable_id = -1;
	{
		Ref<VoxelBlockyModel> tickable;
		tickable.instantiate();
		tickable->set_random_tickable(true);
		tickable_id = library->add_model(tickable);
	}

	library->bake();

	// 创建测试地图
	VoxelData data;
	{
		// 这张地图的所有数据块都是相同的，
		// 即所有数据块类型的交错
		VoxelBuffer model_buffer(VoxelBuffer::ALLOCATOR_DEFAULT);
		model_buffer.create(Vector3iUtil::create(data.get_block_size()));
		for (int z = 0; z < model_buffer.get_size().z; ++z) {
			for (int x = 0; x < model_buffer.get_size().x; ++x) {
				for (int y = 0; y < model_buffer.get_size().y; ++y) {
					const int block_id = (x + y + z) % 3;
					model_buffer.set_voxel(block_id, x, y, z, VoxelBuffer::CHANNEL_TYPE);
				}
			}
		}

		const Box3i world_blocks_box(-4, -4, -4, 8, 8, 8);
		world_blocks_box.for_each_cell_zxy([&data, &model_buffer](Vector3i block_pos) {
			std::shared_ptr<VoxelBuffer> buffer = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_DEFAULT);
			buffer->create(model_buffer.get_size());
			buffer->copy_channels_from(model_buffer);
			VoxelDataBlock block(buffer, 0);
			block.set_edited(true);
			VOXEL_TEST_ASSERT(data.try_set_block(block_pos, block));
		});
	}

	struct Callback {
		const Box3i voxel_box;
		Box3i pick_box;
		bool first_pick = true;
		bool ok = true;
		int tickable_id = -1;

		Callback(const Box3i p_voxel_box, const int p_tickable_id) :
				voxel_box(p_voxel_box), tickable_id(p_tickable_id) {}

		bool exec(const Vector3i pos, const int block_id) {
			if (ok) {
				ok = _exec(pos, block_id);
			}
			return ok;
		}

		inline bool _exec(const Vector3i pos, const int block_id) {
			VOXEL_TEST_ASSERT_V(block_id == tickable_id, false);
			VOXEL_TEST_ASSERT_V(voxel_box.contains(pos), false);
			if (first_pick) {
				first_pick = false;
				pick_box = Box3i(pos, Vector3i(1, 1, 1));
			} else {
				pick_box.merge_with(Box3i(pos, Vector3i(1, 1, 1)));
			}
			return true;
		}
	};

	Callback cb(voxel_box, tickable_id);

	RandomPCG random;
	random.seed(131183);
	voxel::run_blocky_random_tick(
			data,
			voxel_box,
			**library,
			random,
			voxel_count,
			batch_count,
			0xffffffff,
			&cb,
			[](void *self, Vector3i pos, int64_t val) {
				Callback *cb = (Callback *)self;
				return cb->exec(pos, val);
			}
	);

	VOXEL_TEST_ASSERT(cb.ok);

	// 即使存在随机性，我们也期望至少命中一次
	VOXEL_TEST_ASSERT_MSG(!cb.first_pick, "At least one hit is expected, not none");

	// 检查这些点在给定盒子内大致均匀散布。
	// 它们应当如此，因为我们用可 tick 化体素构成的棋盘格充实了世界。
	// 其中涉及随机性，因此遗憾的是我们可能不得不使用容差或挑选合适的种子，
	// 并且我们只检查包围的区域。
	const int error_margin = 0;
	for (int axis_index = 0; axis_index < Vector3iUtil::AXIS_COUNT; ++axis_index) {
		const int nd = cb.pick_box.position[axis_index] - voxel_box.position[axis_index];
		const int pd = cb.pick_box.position[axis_index] + cb.pick_box.size[axis_index] -
				(voxel_box.position[axis_index] + voxel_box.size[axis_index]);
		VOXEL_TEST_ASSERT(Math::abs(nd) <= error_margin);
		VOXEL_TEST_ASSERT(Math::abs(pd) <= error_margin);
	}
}

void test_run_blocky_random_tick() {
	test_run_blocky_random_tick_with_params(Box3i(Vector3i(-24, -23, -22), Vector3i(64, 40, 40)), 1000, 4);
	test_run_blocky_random_tick_with_params(Box3i(Vector3i(1, 2, 3), Vector3i(10, 10, 10)), 500, 800);
}

void test_box_blur() {
	VoxelBuffer voxels(VoxelBuffer::ALLOCATOR_DEFAULT);
	voxels.create(64, 64, 64);

	Vector3i pos;
	for (pos.z = 0; pos.z < voxels.get_size().z; ++pos.z) {
		for (pos.x = 0; pos.x < voxels.get_size().x; ++pos.x) {
			for (pos.y = 0; pos.y < voxels.get_size().y; ++pos.y) {
				const float sd = Math::cos(0.53f * pos.x) + Math::sin(0.37f * pos.y) + Math::sin(0.71f * pos.z);
				voxels.set_voxel_f(sd, pos, VoxelBuffer::CHANNEL_SDF);
			}
		}
	}

	const int blur_radius = 3;
	const Vector3f sphere_pos = to_vec3f(voxels.get_size()) / 2.f;
	const float sphere_radius = 64 - blur_radius;

	struct L {
		static void save_image(const VoxelBuffer &vb, int y, const char *name) {
			Ref<Image> im = godot::VoxelBuffer::debug_print_sdf_z_slice(vb, 1.f, y);
			VOXEL_ASSERT(im.is_valid());
			im->resize(im->get_width() * 4, im->get_height() * 4, Image::INTERPOLATE_NEAREST);
			im->save_png(name);
		}
	};

	// L::save_image(voxels, 32, "test_box_blur_src.png");

	VoxelBuffer voxels_blurred_1(VoxelBuffer::ALLOCATOR_DEFAULT);
	ops::box_blur_slow_ref(voxels, voxels_blurred_1, blur_radius, sphere_pos, sphere_radius);
	// L::save_image(voxels_blurred_1, 32 - blur_radius, "test_box_blur_blurred_1.png");

	VoxelBuffer voxels_blurred_2(VoxelBuffer::ALLOCATOR_DEFAULT);
	ops::box_blur(voxels, voxels_blurred_2, blur_radius, sphere_pos, sphere_radius);
	// L::save_image(voxels_blurred_2, 32 - blur_radius, "test_box_blur_blurred_2.png");

	VOXEL_TEST_ASSERT(sd_equals_approx(voxels_blurred_1, voxels_blurred_2));
}

void test_discord_soakil_copypaste() {
	// 这是 Soakil 在 Discord 上报告的一个 bug。
	//
	// 1) 启用了数据流式传输的 VoxelLodTerrain
	// 2) 生成体素材质设置为 1 的平坦 SDF 地形
	// 3) 把一个区域拷贝到缓冲区
	// 4) 在区域内添加一个球体
	// 5) 把缓冲区粘贴回去以“撤销”该球体
	//
	// 观察到的现象：球体仍然存在，且粘贴区域内的材质变成了 0。
	// 预期结果：地形必须保持与步骤 4 之前相同的状态。
	// 备注：拷贝因 VoxelData 的一个缺陷而不起作用，该缺陷忽略了没有缓存体素的数据块，且没有回退
	// 到生成器。

	// 没有集成测试项目我们就无法测试 VoxelLodTerrain 这类节点，但我们可以用
	// 底层数据结构来测试这个问题。

	// 生成一个以原点为中心、浮空盒子样式的平台的生成器
	Ref<VoxelGeneratorGraph> generator;
	{
		generator.instantiate();
		Ref<pg::VoxelGraphFunction> graph = generator->get_main_function();
		VOXEL_ASSERT(graph.is_valid());

		const uint32_t n_out_sdf = graph->create_node(pg::VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());

		const uint32_t n_out_tex = graph->create_node(pg::VoxelGraphFunction::NODE_OUTPUT_SINGLE_TEXTURE, Vector2());
		graph->set_node_default_input(n_out_tex, 0, 1);

		const uint32_t n_x = graph->create_node(pg::VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_y = graph->create_node(pg::VoxelGraphFunction::NODE_INPUT_Y, Vector2());
		const uint32_t n_z = graph->create_node(pg::VoxelGraphFunction::NODE_INPUT_Z, Vector2());

		const uint32_t n_box = graph->create_node(pg::VoxelGraphFunction::NODE_SDF_BOX, Vector2());
		graph->set_node_param(n_box, 0, 50.0);
		graph->set_node_param(n_box, 1, 1.0);
		graph->set_node_param(n_box, 2, 50.0);

		graph->add_connection(n_x, 0, n_box, 0);
		graph->add_connection(n_y, 0, n_box, 1);
		graph->add_connection(n_z, 0, n_box, 2);
		graph->add_connection(n_box, 0, n_out_sdf, 0);

		pg::CompilationResult compilation_result = generator->compile(false);
		VOXEL_TEST_ASSERT_MSG(
				compilation_result.success,
				String("Failed to compile graph: {0}: {1}")
						.format(varray(compilation_result.node_id, compilation_result.message))
		);
	}

	VoxelData voxel_data;
	voxel_data.set_bounds(Box3i(Vector3iUtil::create(-5000), Vector3iUtil::create(10000)));
	voxel_data.set_streaming_enabled(true);
	voxel_data.set_generator(generator);

	const Box3i terrain_blocks_box(Vector3i(-2, -2, -2), Vector3i(4, 4, 4));

	terrain_blocks_box.for_each_cell([&voxel_data](Vector3i bpos) {
		// std::shared_ptr<VoxelBuffer> vb = make_shared_instance<VoxelBuffer>();
		// vb->create(Vector3iUtil::create(1 << constants::DEFAULT_BLOCK_SIZE_PO2));
		// VoxelGenerator::VoxelQueryData q{ *vb, bpos << constants::DEFAULT_BLOCK_SIZE_PO2, 0 };
		// generator->generate_block(q);
		VoxelDataBlock block;
		// block.set_voxels(vb);
		// 我们标记此数据块已加载但没有体素数据，因此应在运行时按需填充。
		const bool inserted = voxel_data.try_set_block(bpos, block);
		VOXEL_ASSERT(inserted);
	});

	struct L {
		static void check_original(VoxelData &vd) {
			// 平台上方为空气
			const float sd_above_platform = vd.get_voxel_f(Vector3i(0, 5, 0), VoxelBuffer::CHANNEL_SDF);
			VOXEL_TEST_ASSERT(sd_above_platform > 0.01f);

			// 平台中为实体
			const float sd_in_platform = vd.get_voxel_f(Vector3i(0, 0, 0), VoxelBuffer::CHANNEL_SDF);
			VOXEL_TEST_ASSERT(sd_in_platform < -0.01f);

			// 平台下方为空气
			const float sd_below_platform = vd.get_voxel_f(Vector3i(0, -5, 0), VoxelBuffer::CHANNEL_SDF);
			VOXEL_TEST_ASSERT(sd_below_platform > 0.01f);

			// Material

			const VoxelFormat format = vd.get_format();

			VoxelSingleValue default_indices;
			default_indices.i = format.get_default_raw_value(VoxelBuffer::CHANNEL_INDICES);
			VoxelSingleValue default_weights;
			default_weights.i = format.get_default_raw_value(VoxelBuffer::CHANNEL_WEIGHTS);

			const uint16_t packed_indices_in_platform =
					vd.get_voxel(Vector3i(0, 0, 0), VoxelBuffer::CHANNEL_INDICES, default_indices).i;
			const uint16_t packed_weights_in_platform =
					vd.get_voxel(Vector3i(0, 0, 0), VoxelBuffer::CHANNEL_WEIGHTS, default_weights).i;

			check_indices_and_weights_in_platform(packed_indices_in_platform, packed_weights_in_platform);
		}

		static void check_indices_and_weights_in_platform(uint16_t packed_indices, uint16_t packed_weights) {
			const FixedArray<uint8_t, 4> indices_in_platform = mixel4::decode_indices_from_packed_u16(packed_indices);
			unsigned int expected_material_index_index;
			VOXEL_TEST_ASSERT(find(indices_in_platform, uint8_t(1), expected_material_index_index));

			const FixedArray<uint8_t, 4> weights_in_platform = mixel4::decode_weights_from_packed_u16(packed_weights);
			for (unsigned int i = 0; i < weights_in_platform.size(); ++i) {
				if (i == expected_material_index_index) {
					VOXEL_TEST_ASSERT(weights_in_platform[i] > 0);
				} else {
					VOXEL_TEST_ASSERT(weights_in_platform[i] == 0);
				}
			}
		}
	};

	// 检查地形是否与预期一致
	L::check_original(voxel_data);

	VoxelBuffer buffer_before_edit(VoxelBuffer::ALLOCATOR_DEFAULT);
	buffer_before_edit.create(Vector3i(20, 20, 20));
	const Vector3i undo_pos(-10, -10, -10);
	voxel_data.copy(undo_pos, buffer_before_edit, 0xff, true);

	// 检查拷贝结果
	{
		const float sd_above_platform = buffer_before_edit.get_voxel_f(Vector3i(10, 19, 10), VoxelBuffer::CHANNEL_SDF);
		VOXEL_TEST_ASSERT(sd_above_platform > 0.01f);

		const Vector3i pos_in_platform(10, 10, 10);
		const float sd_in_platform = buffer_before_edit.get_voxel_f(pos_in_platform, VoxelBuffer::CHANNEL_SDF);
		VOXEL_TEST_ASSERT(sd_in_platform < -0.01f);

		const float sd_below_platform = buffer_before_edit.get_voxel_f(Vector3i(10, 0, 10), VoxelBuffer::CHANNEL_SDF);
		VOXEL_TEST_ASSERT(sd_below_platform > 0.01f);

		const uint16_t packed_indices_in_platform =
				buffer_before_edit.get_voxel(pos_in_platform, VoxelBuffer::CHANNEL_INDICES);
		const uint16_t packed_weights_in_platform =
				buffer_before_edit.get_voxel(pos_in_platform, VoxelBuffer::CHANNEL_WEIGHTS);

		L::check_indices_and_weights_in_platform(packed_indices_in_platform, packed_weights_in_platform);
	}

	{
		ops::DoSphere op;
		op.shape.center = Vector3f();
		op.shape.radius = 5.f;
		op.shape.sdf_scale = 1.f;
		op.box = op.shape.get_box();
		op.mode = ops::MODE_ADD;
		// op.texture_params = _texture_params
		// op.blocky_value = _value;
		op.channel = VoxelBuffer::CHANNEL_SDF;
		op.strength = 1.f;

		VOXEL_ASSERT(voxel_data.is_area_loaded(op.box));

		voxel_data.pre_generate_box(op.box);
		voxel_data.get_blocks_grid(op.blocks, op.box, 0);
		op();
	}

	voxel_data.pre_generate_box(Box3i(undo_pos, buffer_before_edit.get_size()));
	voxel_data.paste(undo_pos, buffer_before_edit, 0xff, false, true);

	// 检查地形仍与预期一致。不依赖先调用 copy() 再调用 equals()，因为 copy() 正是
	// 我们要测试的对象之一
	L::check_original(voxel_data);
}

void test_sdf_hemisphere() {
	ops::SdfHemisphere shape;
	shape.center = Vector3f();
	shape.flat_direction = Vector3f(0, 1, 0);
	shape.plane_d = 0.f;
	shape.radius = 1.0;
	shape.sdf_scale = 1.0;
	shape.smoothness = 0.1;

	VOXEL_TEST_ASSERT(shape(Vector3f(0, 0.5, 0)) > 0);
	VOXEL_TEST_ASSERT(shape(Vector3f(0, -0.5, 0)) < 0);
	VOXEL_TEST_ASSERT(shape(Vector3f(2, 0, 0)) > 0);
}

} // namespace voxel::tests