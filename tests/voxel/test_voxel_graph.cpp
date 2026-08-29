#include "test_voxel_graph.h"
#include "../../generators/graph/curve_utility.h"
#include "../../generators/graph/image_range_grid.h"
#include "../../generators/graph/image_utility.h"
#include "../../generators/graph/node_type_db.h"
#include "../../generators/graph/voxel_generator_graph.h"
#include "../../storage/mixel4.h"
#include "../../storage/voxel_buffer.h"
#include "../../util/containers/container_funcs.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/fast_noise_lite.h"
#include "../../util/godot/classes/image.h"
#include "../../util/godot/core/random_pcg.h"
#include "../../util/io/std_string_text_writer.h"
#include "../../util/math/conv.h"
#include "../../util/math/sdf.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite.h"
#include "../../util/string/format.h"
#include "../../util/string/std_string.h"
#include "../../util/testing/test_macros.h"
#include "test_util.h"
#include <sstream>

#ifdef VOXEL_ENABLE_FAST_NOISE_2
#include "../../util/noise/fast_noise_2.h"
#endif

namespace voxel::tests {

using namespace pg;

math::Interval get_sdf_range(const VoxelBuffer &block) {
	const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
	math::Interval range = math::Interval::from_single_value(block.get_voxel_f(Vector3i(), channel));
	Vector3i pos;
	const Vector3i size = block.get_size();

	for (pos.z = 0; pos.z < size.z; ++pos.z) {
		for (pos.x = 0; pos.x < size.x; ++pos.x) {
			for (pos.y = 0; pos.y < size.y; ++pos.y) {
				range.add_point(block.get_voxel_f(pos, channel));
			}
		}
	}

	return range;
}

bool check_graph_results_are_equal(VoxelGeneratorGraph &generator1, VoxelGeneratorGraph &generator2, Vector3i origin) {
	{
		const float sd1 = generator1.generate_single(origin, VoxelBuffer::CHANNEL_SDF).f;
		const float sd2 = generator2.generate_single(origin, VoxelBuffer::CHANNEL_SDF).f;

		if (!Math::is_equal_approx(sd1, sd2)) {
			VOXEL_PRINT_ERROR(format("sd1: ", sd1));
			VOXEL_PRINT_ERROR(format("sd2: ", sd1));
			return false;
		}
	}

	const Vector3i block_size(16, 16, 16);

	VoxelBuffer block1(VoxelBuffer::ALLOCATOR_DEFAULT);
	block1.create(block_size);

	VoxelBuffer block2(VoxelBuffer::ALLOCATOR_DEFAULT);
	block2.create(block_size);

	// 注意，并非所有不等价的图配置都可以被视为无效。
	// SDF 裁剪确实会产生差异，而这些差异在我们的用例中理应是不相关的。
	// 因此，用相同的 SDF 裁剪选项来测试生成器非常重要。
	VOXEL_ASSERT(generator1.get_sdf_clip_threshold() == generator2.get_sdf_clip_threshold());

	generator1.generate_block(VoxelGenerator::VoxelQueryData{ block1, origin, 0 });
	generator2.generate_block(VoxelGenerator::VoxelQueryData{ block2, origin, 0 });

	if (block1.equals(block2)) {
		return true;
	}

	const math::Interval range1 = get_sdf_range(block1);
	const math::Interval range2 = get_sdf_range(block2);
	VOXEL_PRINT_ERROR(format("When testing box ", Box3i(origin, block_size)));
	VOXEL_PRINT_ERROR(format("Block1 range: ", range1));
	VOXEL_PRINT_ERROR(format("Block2 range: ", range2));
	return false;
}

bool check_graph_results_are_equal(VoxelGeneratorGraph &generator1, VoxelGeneratorGraph &generator2) {
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(generator1, generator2, Vector3i()));
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(generator1, generator2, Vector3i(-8, -8, -8)));
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(generator1, generator2, Vector3i(0, 100, 0)));
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(generator1, generator2, Vector3i(0, -100, 0)));
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(generator1, generator2, Vector3i(100, 0, 0)));
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(generator1, generator2, Vector3i(-100, 0, 0)));
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(generator1, generator2, Vector3i(100, 100, 100)));
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(generator1, generator2, Vector3i(-100, -100, -100)));
	return true;
}

void test_voxel_graph_generator_default_graph_compilation() {
	Ref<VoxelGeneratorGraph> generator_debug;
	Ref<VoxelGeneratorGraph> generator;
	{
		generator_debug.instantiate();
		generator_debug->load_plane_preset();
		pg::CompilationResult result = generator_debug->compile(true);
		VOXEL_TEST_ASSERT_MSG(
				result.success,
				String("Failed to compile graph: {0}: {1}").format(varray(result.node_id, result.message))
		);
	}
	{
		generator.instantiate();
		generator->load_plane_preset();
		pg::CompilationResult result = generator->compile(false);
		VOXEL_TEST_ASSERT_MSG(
				result.success,
				String("Failed to compile graph: {0}: {1}").format(varray(result.node_id, result.message))
		);
	}
	if (generator_debug.is_valid() && generator.is_valid()) {
		VOXEL_TEST_ASSERT(check_graph_results_are_equal(**generator_debug, **generator));
	}
}

void test_voxel_graph_invalid_connection() {
	Ref<VoxelGraphFunction> graph;
	graph.instantiate();

	VoxelGraphFunction &g = **graph;

	const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
	const uint32_t n_add1 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
	const uint32_t n_add2 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
	const uint32_t n_out = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
	g.add_connection(n_x, 0, n_add1, 0);
	g.add_connection(n_add1, 0, n_add2, 0);
	g.add_connection(n_add2, 0, n_out, 0);

	VOXEL_TEST_ASSERT(g.can_connect(n_add1, 0, n_add2, 1) == true);
	VOXEL_TEST_ASSERT_MSG(g.can_connect(n_add1, 0, n_add2, 0) == false, "Adding twice the same connection is not allowed");
	VOXEL_TEST_ASSERT_MSG(
			g.can_connect(n_x, 0, n_add2, 0) == false, "Adding a connection to a port already connected is not allowed"
	);
	VOXEL_TEST_ASSERT_MSG(g.can_connect(n_add1, 0, n_add1, 1) == false, "Connecting a node to itself is not allowed");
	VOXEL_TEST_ASSERT_MSG(g.can_connect(n_add2, 0, n_add1, 1) == false, "Creating a cycle is not allowed");
}

void load_graph_with_sphere_on_plane(VoxelGraphFunction &g, float radius) {
	//      X
	//       \
	//  Z --- Sphere --- Union --- OutSDF
	//       /          /
	//      Y --- Plane
	//

	const uint32_t n_in_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2(0, 0));
	const uint32_t n_in_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2(0, 0));
	const uint32_t n_in_z = g.create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2(0, 0));
	const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2(0, 0));
	const uint32_t n_plane = g.create_node(VoxelGraphFunction::NODE_SDF_PLANE, Vector2());
	const uint32_t n_sphere = g.create_node(VoxelGraphFunction::NODE_SDF_SPHERE, Vector2());
	const uint32_t n_union = g.create_node(VoxelGraphFunction::NODE_SDF_SMOOTH_UNION, Vector2());

	uint32_t union_smoothness_id;
	VOXEL_ASSERT(
			NodeTypeDB::get_singleton().try_get_param_index_from_name(
					VoxelGraphFunction::NODE_SDF_SMOOTH_UNION, "smoothness", union_smoothness_id
			)
	);

	g.add_connection(n_in_x, 0, n_sphere, 0);
	g.add_connection(n_in_y, 0, n_sphere, 1);
	g.add_connection(n_in_z, 0, n_sphere, 2);
	g.set_node_default_input(n_sphere, 3, radius);
	g.add_connection(n_in_y, 0, n_plane, 0);
	g.set_node_default_input(n_plane, 1, 0.f);
	g.add_connection(n_sphere, 0, n_union, 0);
	g.add_connection(n_plane, 0, n_union, 1);
	g.set_node_param(n_union, union_smoothness_id, 0.f);
	g.add_connection(n_union, 0, n_out_sdf, 0);
}

void load_graph_with_expression(VoxelGraphFunction &g) {
	const uint32_t in_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2(0, 0));
	const uint32_t in_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2(0, 0));
	const uint32_t in_z = g.create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2(0, 0));
	const uint32_t out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2(0, 0));
	const uint32_t n_expression = g.create_node(VoxelGraphFunction::NODE_EXPRESSION, Vector2());

	//             0.5
	//                \
	//     0.1   y --- min
	//        \           \
	//   x --- * --- + --- + --- sdf
	//              /
	//     0.2 --- *
	//            /
	//           z

	g.set_node_param(n_expression, 0, "0.1 * x + 0.2 * z + min(y, 0.5)");
	PackedStringArray var_names;
	var_names.push_back("x");
	var_names.push_back("y");
	var_names.push_back("z");
	g.set_expression_node_inputs(n_expression, var_names);

	g.add_connection(in_x, 0, n_expression, 0);
	g.add_connection(in_y, 0, n_expression, 1);
	g.add_connection(in_z, 0, n_expression, 2);
	g.add_connection(n_expression, 0, out_sdf, 0);
}

void load_graph_with_expression_and_noises(VoxelGraphFunction &g, Ref<Voxel_FastNoiseLite> *out_zfnl) {
	//                       SdfPreview
	//                      /
	//     X --- FastNoise2D
	//      \/              \
	//      /\               \
	//     Z --- Noise2D ----- a+b+c --- OutputSDF
	//                        /
	//     Y --- SdfPlane ----

	const uint32_t in_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2(0, 0));
	const uint32_t in_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2(0, 0));
	const uint32_t in_z = g.create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2(0, 0));
	const uint32_t out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2(0, 0));
	const uint32_t n_fn2d = g.create_node(VoxelGraphFunction::NODE_FAST_NOISE_2D, Vector2());
	const uint32_t n_n2d = g.create_node(VoxelGraphFunction::NODE_NOISE_2D, Vector2());
	const uint32_t n_plane = g.create_node(VoxelGraphFunction::NODE_SDF_PLANE, Vector2());
	const uint32_t n_expr = g.create_node(VoxelGraphFunction::NODE_EXPRESSION, Vector2());
	const uint32_t n_preview = g.create_node(VoxelGraphFunction::NODE_SDF_PREVIEW, Vector2());

	g.set_node_param(n_expr, 0, "a+b+c");
	PackedStringArray var_names;
	var_names.push_back("a");
	var_names.push_back("b");
	var_names.push_back("c");
	g.set_expression_node_inputs(n_expr, var_names);

	Ref<Voxel_FastNoiseLite> zfnl;
	zfnl.instantiate();
	g.set_node_param(n_fn2d, 0, zfnl);

	Ref<FastNoiseLite> fnl;
	fnl.instantiate();
	g.set_node_param(n_n2d, 0, fnl);

	g.add_connection(in_x, 0, n_fn2d, 0);
	g.add_connection(in_x, 0, n_n2d, 0);
	g.add_connection(in_z, 0, n_fn2d, 1);
	g.add_connection(in_z, 0, n_n2d, 1);
	g.add_connection(in_y, 0, n_plane, 0);
	g.add_connection(n_fn2d, 0, n_expr, 0);
	g.add_connection(n_fn2d, 0, n_preview, 0);
	g.add_connection(n_n2d, 0, n_expr, 1);
	g.add_connection(n_plane, 0, n_expr, 2);
	g.add_connection(n_expr, 0, out_sdf, 0);

	if (out_zfnl != nullptr) {
		*out_zfnl = zfnl;
	}
}

void load_graph_with_clamp(VoxelGraphFunction &g, float ramp_half_size) {
	// 两个高度不同的平面，它们之间沿 X 轴有一段 45 度的斜坡。
	// 该平面在负 X 方向更高，在正 X 方向更低。
	//
	//   X --- Clamp --- + --- Out
	//                  /
	//                 Y

	const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
	const uint32_t n_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2());
	// 不使用 CLAMP_C 来测试简化
	const uint32_t n_clamp = g.create_node(VoxelGraphFunction::NODE_CLAMP, Vector2());
	const uint32_t n_add = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
	const uint32_t n_out = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());

	g.set_node_default_input(n_clamp, 1, -ramp_half_size);
	g.set_node_default_input(n_clamp, 2, ramp_half_size);

	g.add_connection(n_x, 0, n_clamp, 0);
	g.add_connection(n_clamp, 0, n_add, 0);
	g.add_connection(n_y, 0, n_add, 1);
	g.add_connection(n_add, 0, n_out, 0);
}

void test_voxel_graph_clamp_simplification() {
	// 编译时 CLAMP 节点会被替换为 CLAMP_C 节点。
	// 这用于测试生成器在替换后仍能正常运行。
	static const float RAMP_HALF_SIZE = 4.f;
	struct L {
		static Ref<VoxelGeneratorGraph> create_graph(bool debug) {
			Ref<VoxelGeneratorGraph> generator;
			generator.instantiate();
			VOXEL_ASSERT(generator->get_main_function().is_valid());
			load_graph_with_clamp(**generator->get_main_function(), RAMP_HALF_SIZE);
			pg::CompilationResult result = generator->compile(debug);
			VOXEL_TEST_ASSERT_MSG(
					result.success,
					String("Failed to compile graph: {0}: {1}").format(varray(result.node_id, result.message))
			);
			return generator;
		}
		static void test_locations(VoxelGeneratorGraph &g) {
			const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
			const float sd_on_higher_side_below_ground =
					g.generate_single(Vector3i(-RAMP_HALF_SIZE - 10, 0, 0), channel).f;
			const float sd_on_higher_side_above_ground =
					g.generate_single(Vector3i(-RAMP_HALF_SIZE - 10, RAMP_HALF_SIZE + 2, 0), channel).f;
			const float sd_on_lower_side_above_ground =
					g.generate_single(Vector3i(RAMP_HALF_SIZE + 10, 0, 0), channel).f;
			const float sd_on_lower_side_below_ground =
					g.generate_single(Vector3i(RAMP_HALF_SIZE + 10, -RAMP_HALF_SIZE - 2, 0), channel).f;

			VOXEL_TEST_ASSERT(sd_on_lower_side_above_ground > 0.f);
			VOXEL_TEST_ASSERT(sd_on_lower_side_below_ground < 0.f);
			VOXEL_TEST_ASSERT(sd_on_higher_side_above_ground > 0.f);
			VOXEL_TEST_ASSERT(sd_on_higher_side_below_ground < 0.f);
		}
	};
	Ref<VoxelGeneratorGraph> generator_debug = L::create_graph(true);
	Ref<VoxelGeneratorGraph> generator = L::create_graph(false);
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(**generator_debug, **generator));
	L::test_locations(**generator);
	L::test_locations(**generator_debug);
}

void test_voxel_graph_generator_expressions() {
	struct L {
		static Ref<VoxelGeneratorGraph> create_graph(bool debug) {
			Ref<VoxelGeneratorGraph> generator;
			generator.instantiate();
			VOXEL_ASSERT(generator->get_main_function().is_valid());
			load_graph_with_expression(**generator->get_main_function());
			pg::CompilationResult result = generator->compile(debug);
			VOXEL_TEST_ASSERT_MSG(
					result.success,
					String("Failed to compile graph: {0}: {1}").format(varray(result.node_id, result.message))
			);
			return generator;
		}
	};
	Ref<VoxelGeneratorGraph> generator_debug = L::create_graph(true);
	Ref<VoxelGeneratorGraph> generator = L::create_graph(false);
	VOXEL_TEST_ASSERT(check_graph_results_are_equal(**generator_debug, **generator));
}

void test_voxel_graph_generator_expressions_2() {
	Ref<Voxel_FastNoiseLite> zfnl;
	{
		Ref<VoxelGeneratorGraph> generator_debug;
		{
			generator_debug.instantiate();
			Ref<VoxelGraphFunction> graph = generator_debug->get_main_function();
			VOXEL_ASSERT(graph.is_valid());
			load_graph_with_expression_and_noises(**graph, &zfnl);
			pg::CompilationResult result = generator_debug->compile(true);
			VOXEL_TEST_ASSERT_MSG(
					result.success,
					String("Failed to compile graph: {0}: {1}").format(varray(result.node_id, result.message))
			);

			generator_debug->generate_single(Vector3i(1, 2, 3), VoxelBuffer::CHANNEL_SDF);

			StdVector<VoxelGeneratorGraph::NodeProfilingInfo> profiling_info;
			generator_debug->debug_measure_microseconds_per_voxel(false, &profiling_info);
			VOXEL_TEST_ASSERT(profiling_info.size() >= 4);
			for (const VoxelGeneratorGraph::NodeProfilingInfo &info : profiling_info) {
				VOXEL_TEST_ASSERT(graph->has_node(info.node_id));
			}
		}

		Ref<VoxelGeneratorGraph> generator;
		{
			generator.instantiate();
			VOXEL_ASSERT(generator->get_main_function().is_valid());
			load_graph_with_expression_and_noises(**generator->get_main_function(), nullptr);
			pg::CompilationResult result = generator->compile(false);
			VOXEL_TEST_ASSERT_MSG(
					result.success,
					String("Failed to compile graph: {0}: {1}").format(varray(result.node_id, result.message))
			);
		}

		VOXEL_TEST_ASSERT(check_graph_results_are_equal(**generator_debug, **generator));
	}

	// 确保它没有泄漏
	VOXEL_TEST_ASSERT(zfnl.is_valid());
	VOXEL_TEST_ASSERT(zfnl->get_reference_count() == 1);
}

void test_voxel_graph_generator_texturing() {
	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();

	VoxelGraphFunction &g = **generator->get_main_function();

	// 平面中心位于 Y=0，倾斜 45 度，朝 +X 方向上升
	// 当 Y<0 时，weight0 必须为 1，weight1 必须为 0。
	// 当 Y>0 时，weight0 必须为 0，weight1 必须为 1。
	// 当 0<Y<1 时，weight0 必须从 1 过渡到 0，weight1 必须从 0 过渡到 1。

	/*
	 *        Clamp --- Sub1 --- Weight0
	 *       /      \
	 *  Z   Y        Weight1
	 *       \
	 *  X --- Sub0 --- Sdf
	 *
	 */

	const uint32_t in_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2(0, 0));
	const uint32_t in_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2(0, 0));
	/*const uint32_t in_z =*/g.create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2(0, 0));
	const uint32_t out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2(0, 0));
	const uint32_t n_clamp = g.create_node(VoxelGraphFunction::NODE_CLAMP_C, Vector2(0, 0));
	const uint32_t n_sub0 = g.create_node(VoxelGraphFunction::NODE_SUBTRACT, Vector2(0, 0));
	const uint32_t n_sub1 = g.create_node(VoxelGraphFunction::NODE_SUBTRACT, Vector2(0, 0));
	const uint32_t out_weight0 = g.create_node(VoxelGraphFunction::NODE_OUTPUT_WEIGHT, Vector2(0, 0));
	const uint32_t out_weight1 = g.create_node(VoxelGraphFunction::NODE_OUTPUT_WEIGHT, Vector2(0, 0));

	g.set_node_default_input(n_sub1, 0, 1.0);
	g.set_node_param(n_clamp, 0, 0.0);
	g.set_node_param(n_clamp, 1, 1.0);
	g.set_node_param(out_weight0, 0, 0);
	g.set_node_param(out_weight1, 0, 1);

	g.add_connection(in_y, 0, n_sub0, 0);
	g.add_connection(in_x, 0, n_sub0, 1);
	g.add_connection(n_sub0, 0, out_sdf, 0);
	g.add_connection(in_y, 0, n_clamp, 0);
	g.add_connection(n_clamp, 0, n_sub1, 1);
	g.add_connection(n_sub1, 0, out_weight0, 0);
	g.add_connection(n_clamp, 0, out_weight1, 0);

	pg::CompilationResult compilation_result = generator->compile(false);
	VOXEL_TEST_ASSERT_MSG(
			compilation_result.success,
			String("Failed to compile graph: {0}: {1}")
					.format(varray(compilation_result.node_id, compilation_result.message))
	);

	// 单值测试
	{
		const float sdf_must_be_in_air = generator->generate_single(Vector3i(-2, 0, 0), VoxelBuffer::CHANNEL_SDF).f;
		const float sdf_must_be_in_ground = generator->generate_single(Vector3i(2, 0, 0), VoxelBuffer::CHANNEL_SDF).f;
		VOXEL_TEST_ASSERT(sdf_must_be_in_air > 0.f);
		VOXEL_TEST_ASSERT(sdf_must_be_in_ground < 0.f);

		uint32_t out_weight0_buffer_index;
		uint32_t out_weight1_buffer_index;
		VOXEL_TEST_ASSERT(generator->try_get_output_port_address(
				ProgramGraph::PortLocation{ out_weight0, 0 }, out_weight0_buffer_index
		));
		VOXEL_TEST_ASSERT(generator->try_get_output_port_address(
				ProgramGraph::PortLocation{ out_weight1, 0 }, out_weight1_buffer_index
		));

		// 在地面下方 1 个单位的斜坡上采样两个点

		{
			const float sdf = generator->generate_single(Vector3i(-2, -3, 0), VoxelBuffer::CHANNEL_SDF).f;
			VOXEL_TEST_ASSERT(sdf < 0.f);
			const pg::Runtime::State &state = VoxelGeneratorGraph::get_last_state_from_current_thread();

			const pg::Runtime::Buffer &out_weight0_buffer = state.get_buffer(out_weight0_buffer_index);
			const pg::Runtime::Buffer &out_weight1_buffer = state.get_buffer(out_weight1_buffer_index);

			VOXEL_TEST_ASSERT(out_weight0_buffer.size >= 1);
			VOXEL_TEST_ASSERT(out_weight0_buffer.data != nullptr);
			VOXEL_TEST_ASSERT(out_weight0_buffer.data[0] >= 1.f);

			VOXEL_TEST_ASSERT(out_weight1_buffer.size >= 1);
			VOXEL_TEST_ASSERT(out_weight1_buffer.data != nullptr);
			VOXEL_TEST_ASSERT(out_weight1_buffer.data[0] <= 0.f);
		}
		{
			const float sdf = generator->generate_single(Vector3i(2, 1, 0), VoxelBuffer::CHANNEL_SDF).f;
			VOXEL_TEST_ASSERT(sdf < 0.f);
			const pg::Runtime::State &state = VoxelGeneratorGraph::get_last_state_from_current_thread();

			const pg::Runtime::Buffer &out_weight0_buffer = state.get_buffer(out_weight0_buffer_index);
			const pg::Runtime::Buffer &out_weight1_buffer = state.get_buffer(out_weight1_buffer_index);

			VOXEL_TEST_ASSERT(out_weight0_buffer.size >= 1);
			VOXEL_TEST_ASSERT(out_weight0_buffer.data != nullptr);
			VOXEL_TEST_ASSERT(out_weight0_buffer.data[0] <= 0.f);

			VOXEL_TEST_ASSERT(out_weight1_buffer.size >= 1);
			VOXEL_TEST_ASSERT(out_weight1_buffer.data != nullptr);
			VOXEL_TEST_ASSERT(out_weight1_buffer.data[0] >= 1.f);
		}
	}

	// 数据块测试
	{
		// 由于折中，打包的 U16 格式解码的最大值会略低一些
		const uint8_t WEIGHT_MAX = 240;

		struct L {
			static void check_weights(
					VoxelBuffer &buffer,
					Vector3i pos,
					bool weight0_must_be_1,
					bool weight1_must_be_1
			) {
				const uint16_t encoded_indices = buffer.get_voxel(pos, VoxelBuffer::CHANNEL_INDICES);
				const uint16_t encoded_weights = buffer.get_voxel(pos, VoxelBuffer::CHANNEL_WEIGHTS);
				const FixedArray<uint8_t, 4> indices = mixel4::decode_indices_from_packed_u16(encoded_indices);
				const FixedArray<uint8_t, 4> weights = mixel4::decode_weights_from_packed_u16(encoded_weights);
				for (unsigned int i = 0; i < indices.size(); ++i) {
					switch (indices[i]) {
						case 0:
							if (weight0_must_be_1) {
								VOXEL_TEST_ASSERT(weights[i] >= WEIGHT_MAX);
							} else {
								VOXEL_TEST_ASSERT(weights[i] <= 0);
							}
							break;
						case 1:
							if (weight1_must_be_1) {
								VOXEL_TEST_ASSERT(weights[i] >= WEIGHT_MAX);
							} else {
								VOXEL_TEST_ASSERT(weights[i] <= 0);
							}
							break;
						default:
							break;
					}
				}
			}

			static void do_block_tests(Ref<VoxelGeneratorGraph> generator) {
				ERR_FAIL_COND(generator.is_null());
				{
					// 以原点为中心的数据块
					VoxelBuffer buffer(VoxelBuffer::ALLOCATOR_DEFAULT);
					buffer.create(Vector3i(16, 16, 16));

					VoxelGenerator::VoxelQueryData query{ buffer, -buffer.get_size() / 2, 0 };
					generator->generate_block(query);

					L::check_weights(buffer, Vector3i(4, 3, 8), true, false);
					L::check_weights(buffer, Vector3i(12, 11, 8), false, true);
				}
				{
					// 两个数据块：一个在 0 之上，另一个在 0 之下。
					// 目的是检查可能因优化而产生的 bug。

					// Below 0
					VoxelBuffer buffer0(VoxelBuffer::ALLOCATOR_DEFAULT);
					{
						buffer0.create(Vector3i(16, 16, 16));
						VoxelGenerator::VoxelQueryData query{ buffer0, Vector3i(0, -16, 0), 0 };
						generator->generate_block(query);
					}

					// Above 0
					VoxelBuffer buffer1(VoxelBuffer::ALLOCATOR_DEFAULT);
					{
						buffer1.create(Vector3i(16, 16, 16));
						VoxelGenerator::VoxelQueryData query{ buffer1, Vector3i(0, 0, 0), 0 };
						generator->generate_block(query);
					}

					L::check_weights(buffer0, Vector3i(8, 8, 8), true, false);
					L::check_weights(buffer1, Vector3i(8, 8, 8), false, true);
				}
			}
		};

		// 把状态放到栈上，因为调试器不让我访问它
		// const pg::Runtime::State &state = VoxelGeneratorGraph::get_last_state_from_current_thread();

		// 先尝试不使用优化
		generator->set_use_optimized_execution_map(false);
		L::do_block_tests(generator);
		// 再尝试使用优化
		generator->set_use_optimized_execution_map(true);
		L::do_block_tests(generator);
	}
}

void test_voxel_graph_equivalence_merging() {
	{
		// 有两个等价分支的基础图

		//        1
		//         \
		//    X --- +                         1
		//           \             =>          \
		//        1   + --- Out           X --- + === + --- Out
		//         \ /
		//    X --- +

		Ref<VoxelGeneratorGraph> graph;
		graph.instantiate();
		VoxelGraphFunction &g = **graph->get_main_function();
		const uint32_t n_x1 = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_add1 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_x2 = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_add2 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_add3 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_out = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		g.set_node_default_input(n_add1, 0, 1.0);
		g.set_node_default_input(n_add2, 0, 1.0);
		g.add_connection(n_x1, 0, n_add1, 1);
		g.add_connection(n_add1, 0, n_add3, 0);
		g.add_connection(n_x2, 0, n_add2, 1);
		g.add_connection(n_add2, 0, n_add3, 1);
		g.add_connection(n_add3, 0, n_out, 0);
		pg::CompilationResult result = graph->compile(false);
		VOXEL_TEST_ASSERT(result.success);
		VOXEL_TEST_ASSERT(result.expanded_nodes_count == 4);
		const VoxelSingleValue value = graph->generate_single(Vector3i(10, 0, 0), VoxelBuffer::CHANNEL_SDF);
		VOXEL_TEST_ASSERT(value.f == 22);
	}
	{
		// 与上一个相同，但 X 输入节点是共享的

		//          1
		//           \
		//    X ----- +
		//     \       \
		//      \   1   + --- Out
		//       \   \ /
		//        --- +

		Ref<VoxelGeneratorGraph> graph;
		graph.instantiate();
		VoxelGraphFunction &g = **graph->get_main_function();
		const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_add1 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_add2 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_add3 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_out = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		g.set_node_default_input(n_add1, 0, 1.0);
		g.set_node_default_input(n_add2, 0, 1.0);
		g.add_connection(n_x, 0, n_add1, 1);
		g.add_connection(n_add1, 0, n_add3, 0);
		g.add_connection(n_x, 0, n_add2, 1);
		g.add_connection(n_add2, 0, n_add3, 1);
		g.add_connection(n_add3, 0, n_out, 0);
		pg::CompilationResult result = graph->compile(false);
		VOXEL_TEST_ASSERT(result.success);
		VOXEL_TEST_ASSERT(result.expanded_nodes_count == 4);
		const VoxelSingleValue value = graph->generate_single(Vector3i(10, 0, 0), VoxelBuffer::CHANNEL_SDF);
		VOXEL_TEST_ASSERT(value.f == 22);
	}
}

int get_decimal_integer_character_count(int n) {
	if (n == 0) {
		return 1;
	}
	int count = 0;
	if (n < 0) {
		n = -n;
		++count;
	}
	while (n > 0) {
		n /= 10;
		++count;
	}
	return count;
}

void print_sdf_as_ascii(const VoxelBuffer &vb) {
	Vector3i pos;
	const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
	for (pos.y = 0; pos.y < vb.get_size().y; ++pos.y) {
		print_line(format("Y = {}", pos.y));
		for (pos.z = 0; pos.z < vb.get_size().z; ++pos.z) {
			// 并排打印同一行的两个视图
			StdStringTextWriter ss;
			StdStringTextWriter ss2;
			for (pos.x = 0; pos.x < vb.get_size().x; ++pos.x) {
				const float sd = vb.get_voxel_f(pos, channel);
				char c;
				if (sd < -0.9f) {
					c = '=';
				} else if (sd < 0.0f) {
					c = '-';
				} else if (sd == 0.f) {
					c = ' ';
				} else if (sd < 0.9f) {
					c = '+';
				} else {
					c = '#';
				}
				ss << c;
				ss << ' ';

				const int n = math::clamp(int(sd * 1000.f), -999, 999);
				const int n_char_length = get_decimal_integer_character_count(n);
				const int space_count = 4 - n_char_length;
				for (int i = 0; i < space_count; ++i) {
					ss2 << ' ';
				}
				ss2 << n;
				ss2 << ' ';
			}
			ss << " | ";
			ss << ss2.get_written();
			print_line(ss.get_written());
		}
	}
}

/*bool find_different_voxel(const VoxelBuffer &vb1, const VoxelBuffer &vb2, Vector3i *out_pos,
		unsigned int *out_channel_index) {
	VOXEL_ASSERT(vb1.get_size() == vb2.get_size());
	Vector3i pos;
	for (pos.y = 0; pos.y < vb1.get_size().y; ++pos.y) {
		for (pos.z = 0; pos.z < vb1.get_size().z; ++pos.z) {
			for (pos.x = 0; pos.x < vb1.get_size().x; ++pos.x) {
				for (unsigned int channel_index = 0; channel_index < VoxelBuffer::MAX_CHANNELS;
						++channel_index) {
					const uint64_t v1 = vb1.get_voxel(pos, channel_index);
					const uint64_t v2 = vb2.get_voxel(pos, channel_index);
					if (v1 != v2) {
						if (out_pos != nullptr) {
							*out_pos = pos;
						}
						if (out_channel_index != nullptr) {
							*out_channel_index = channel_index;
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}*/

void test_voxel_graph_generate_block_with_input_sdf() {
	static const int BLOCK_SIZE = 16;
	static const float SPHERE_RADIUS = 6;

	struct L {
		static void load_graph(VoxelGraphFunction &g) {
			// 只是把输入原样输出
			const uint32_t n_in_sdf = g.create_node(VoxelGraphFunction::NODE_INPUT_SDF, Vector2());
			const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
			g.add_connection(n_in_sdf, 0, n_out_sdf, 0);
		}

		static void test(bool subdivision_enabled, int subdivision_size) {
			// 创建生成器
			Ref<VoxelGeneratorGraph> generator;
			generator.instantiate();
			L::load_graph(**generator->get_main_function());
			const pg::CompilationResult compilation_result = generator->compile(false);
			VOXEL_TEST_ASSERT_MSG(
					compilation_result.success,
					String("Failed to compile graph: {0}: {1}")
							.format(varray(compilation_result.node_id, compilation_result.message))
			);

			// 创建包含球体一部分的缓冲区
			VoxelBuffer buffer(VoxelBuffer::ALLOCATOR_DEFAULT);
			buffer.create(Vector3i(BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE));
			const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
			// const VoxelBuffer::Depth depth = buffer.get_channel_depth(channel);
			for (int z = 0; z < buffer.get_size().z; ++z) {
				for (int x = 0; x < buffer.get_size().x; ++x) {
					for (int y = 0; y < buffer.get_size().y; ++y) {
						// 位于原点的球体
						const float sd = math::sdf_sphere(Vector3f(x, y, z), Vector3f(), SPHERE_RADIUS);
						buffer.set_voxel_f(sd, Vector3i(x, y, z), channel);
					}
				}
			}

			// 在运行生成器之前做一次备份
			VoxelBuffer buffer_before(VoxelBuffer::ALLOCATOR_DEFAULT);
			buffer_before.create(buffer.get_size());
			buffer_before.copy_channels_from(buffer);

			generator->set_use_subdivision(subdivision_enabled);
			generator->set_subdivision_size(subdivision_size);
			generator->generate_block(VoxelGenerator::VoxelQueryData{ buffer, Vector3i(), 0 });

			/*if (!buffer.equals(buffer_before)) {
				println("Buffer before:");
				print_sdf_as_ascii(buffer_before);
				println("Buffer after:");
				print_sdf_as_ascii(buffer);
				Vector3i different_pos;
				unsigned int different_channel;
				if (find_different_voxel(buffer_before, buffer, &different_pos, &different_channel)) {
					const uint64_t v1 = buffer_before.get_voxel(different_pos, different_channel);
					const uint64_t v2 = buffer.get_voxel(different_pos, different_channel);
					println(format("Different position: {}, v1={}, v2={}", different_pos, v1, v2));
				}
			}*/
			VOXEL_TEST_ASSERT(sd_equals_approx(buffer, buffer_before));
		}
	};

	L::test(false, BLOCK_SIZE / 2);
	L::test(true, BLOCK_SIZE / 2);
}

Ref<VoxelGraphFunction> create_pass_through_function() {
	Ref<VoxelGraphFunction> func;
	func.instantiate();
	{
		VoxelGraphFunction &g = **func;
		// 直通
		// X --- OutSDF
		const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		g.add_connection(n_x, 0, n_out_sdf, 0);

		func->auto_pick_inputs_and_outputs();
	}
	return func;
}

void test_voxel_graph_functions_pass_through() {
	Ref<VoxelGraphFunction> func = create_pass_through_function();
	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	{
		VoxelGraphFunction &g = **generator->get_main_function();
		// X --- Func --- OutSDF
		const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_f = g.create_function_node(func, Vector2());
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		g.add_connection(n_x, 0, n_f, 0);
		g.add_connection(n_f, 0, n_out_sdf, 0);
	}
	const pg::CompilationResult compilation_result = generator->compile(false);
	VOXEL_TEST_ASSERT_MSG(
			compilation_result.success,
			String("Failed to compile graph: {0}: {1}")
					.format(varray(compilation_result.node_id, compilation_result.message))
	);
	const float f = generator->generate_single(Vector3i(42, 0, 0), VoxelBuffer::CHANNEL_SDF).f;
	VOXEL_TEST_ASSERT(f == 42.f);
}

void test_voxel_graph_functions_nested_pass_through() {
	Ref<VoxelGraphFunction> func1 = create_pass_through_function();

	// 使用另一个函数的最小函数
	Ref<VoxelGraphFunction> func2;
	func2.instantiate();
	{
		VoxelGraphFunction &g = **func2;
		// 嵌套直通
		// X --- Func1 --- OutSDF
		const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_f = g.create_function_node(func1, Vector2());
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		g.add_connection(n_x, 0, n_f, 0);
		g.add_connection(n_f, 0, n_out_sdf, 0);

		func2->auto_pick_inputs_and_outputs();
	}

	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	{
		VoxelGraphFunction &g = **generator->get_main_function();
		// X --- Func2 --- OutSDF
		const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_f = g.create_function_node(func2, Vector2());
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		g.add_connection(n_x, 0, n_f, 0);
		g.add_connection(n_f, 0, n_out_sdf, 0);
	}
	const pg::CompilationResult compilation_result = generator->compile(false);
	VOXEL_TEST_ASSERT_MSG(
			compilation_result.success,
			String("Failed to compile graph: {0}: {1}")
					.format(varray(compilation_result.node_id, compilation_result.message))
	);
	const float f = generator->generate_single(Vector3i(42, 0, 0), VoxelBuffer::CHANNEL_SDF).f;
	VOXEL_TEST_ASSERT(f == 42.f);
}

void test_voxel_graph_functions_autoconnect() {
	Ref<VoxelGraphFunction> func;
	func.instantiate();
	const float sphere_radius = 10.f;
	{
		VoxelGraphFunction &g = **func;
		// Sphere --- OutSDF
		const uint32_t n_sphere = g.create_node(VoxelGraphFunction::NODE_SDF_SPHERE, Vector2());
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		g.add_connection(n_sphere, 0, n_out_sdf, 0);
		g.set_node_default_input(n_sphere, 3, sphere_radius);

		g.auto_pick_inputs_and_outputs();
	}

	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	const float z_offset = 5.f;
	{
		VoxelGraphFunction &g = **generator->get_main_function();
		//      X (auto)
		//              \
		//  Y (auto) --- Func --- OutSDF
		//              /
		//     Z --- Add+5
		//
		const uint32_t n_z = g.create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2());
		const uint32_t n_add = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_f = g.create_function_node(func, Vector2());
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		g.add_connection(n_z, 0, n_add, 0);
		g.set_node_default_input(n_add, 1, z_offset);
		g.add_connection(n_add, 0, n_f, 2);
		g.add_connection(n_f, 0, n_out_sdf, 0);
	}
	const pg::CompilationResult compilation_result = generator->compile(false);
	VOXEL_TEST_ASSERT_MSG(
			compilation_result.success,
			String("Failed to compile graph: {0}: {1}")
					.format(varray(compilation_result.node_id, compilation_result.message))
	);
	FixedArray<Vector3i, 3> positions;
	positions[0] = Vector3i(1, 1, 1);
	positions[1] = Vector3i(20, 7, -4);
	positions[2] = Vector3i(-5, 0, 18);
	for (const Vector3i &pos : positions) {
		const float sd = generator->generate_single(pos, VoxelBuffer::CHANNEL_SDF).f;
		const float expected = math::length(Vector3f(pos.x, pos.y, pos.z + z_offset)) - sphere_radius;
		VOXEL_TEST_ASSERT(Math::is_equal_approx(sd, expected));
	}
}

void test_voxel_graph_functions_io_mismatch() {
	Ref<VoxelGraphFunction> func;
	func.instantiate();

	// X --- Add --- OutSDF
	//      /
	//     Y
	const uint32_t fn_x = func->create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
	const uint32_t fn_y = func->create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2());
	const uint32_t fn_add = func->create_node(VoxelGraphFunction::NODE_ADD, Vector2());
	const uint32_t fn_out_sdf = func->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
	func->add_connection(fn_x, 0, fn_add, 0);
	func->add_connection(fn_y, 0, fn_add, 1);
	func->add_connection(fn_add, 0, fn_out_sdf, 0);
	func->auto_pick_inputs_and_outputs();

	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	{
		VoxelGraphFunction &g = **generator->get_main_function();
		// X --- Func --- OutSDF
		//      /
		//     Y
		const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2());
		const uint32_t n_f = g.create_function_node(func, Vector2());
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		g.add_connection(n_x, 0, n_f, 0);
		g.add_connection(n_y, 0, n_f, 1);
		g.add_connection(n_f, 0, n_out_sdf, 0);
	}
	{
		const pg::CompilationResult compilation_result = generator->compile(false);
		VOXEL_TEST_ASSERT_MSG(
				compilation_result.success,
				String("Failed to compile graph: {0}: {1}")
						.format(varray(compilation_result.node_id, compilation_result.message))
		);
	}

	// 现在从函数中移除一个输入，看看结果如何
	{
		FixedArray<VoxelGraphFunction::Port, 1> inputs;
		inputs[0] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_INPUT_X, "x" };
		FixedArray<VoxelGraphFunction::Port, 1> outputs;
		outputs[0] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_OUTPUT_SDF, "sdf" };
		func->set_io_definitions(to_span(inputs), to_span(outputs));
	}
	{
		const pg::CompilationResult compilation_result = generator->compile(false);
		// 编译应该失败，但不崩溃
		VOXEL_TEST_ASSERT(compilation_result.success == false);
		VOXEL_PRINT_VERBOSE(format("Compiling failed with message '{}'", compilation_result.message));
	}
	generator->get_main_function()->update_function_nodes(nullptr);
	{
		const pg::CompilationResult compilation_result = generator->compile(false);
		// 现在编译应该能通过了
		VOXEL_TEST_ASSERT(compilation_result.success == true);
	}
}

void test_voxel_graph_functions_misc() {
	static const float func_custom_input_defval = 42.f;

	struct L {
		static Ref<VoxelGraphFunction> create_misc_function() {
			Ref<VoxelGraphFunction> func;
			func.instantiate();
			{
				VoxelGraphFunction &g = **func;
				//
				//          X              OutCustom
				//           \
				//       Z -- Add --- Add --- OutSDF
				//                   /
				//           InCustom
				//
				//   Y(unused)
				//
				const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
				/*const uint32_t n_y =*/g.create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2());
				const uint32_t n_z = g.create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2());
				const uint32_t n_add1 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
				const uint32_t n_add2 = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2());
				const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
				const uint32_t n_in_custom = g.create_node(VoxelGraphFunction::NODE_CUSTOM_INPUT, Vector2());
				const uint32_t n_out_custom = g.create_node(VoxelGraphFunction::NODE_CUSTOM_OUTPUT, Vector2());

				g.set_node_name(n_in_custom, "custom_input");
				g.set_node_name(n_out_custom, "custom_output");

				g.add_connection(n_x, 0, n_add1, 0);
				g.add_connection(n_z, 0, n_add1, 1);
				g.add_connection(n_add1, 0, n_add2, 0);
				g.add_connection(n_in_custom, 0, n_add2, 1);
				g.add_connection(n_add2, 0, n_out_sdf, 0);
			}
			return func;
		}

		static Ref<VoxelGeneratorGraph> create_generator(Ref<VoxelGraphFunction> func, int input_count) {
			Ref<VoxelGeneratorGraph> generator;
			generator.instantiate();
			//      X
			//       \
			//  Z --- Func --- OutSDF
			//
			{
				VoxelGraphFunction &g = **generator->get_main_function();

				const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
				const uint32_t n_z = g.create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2());
				const uint32_t n_f = g.create_function_node(func, Vector2());
				const uint32_t n_out = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());

				if (input_count == 4) {
					g.set_node_default_input(n_f, 3, func_custom_input_defval);
					// 这个应该无所谓，它没有被使用，但仍被定义
					g.set_node_default_input(n_f, 2, 12345);
				}

				g.add_connection(n_x, 0, n_f, 0);
				g.add_connection(n_z, 0, n_f, 1);
				g.add_connection(n_f, 0, n_out, 0);
			}

			return generator;
		}
	};

	// 常规测试
	{
		Ref<VoxelGraphFunction> func = L::create_misc_function();
		func->auto_pick_inputs_and_outputs();
		VOXEL_TEST_ASSERT(func->get_input_definitions().size() == 4);
		VOXEL_TEST_ASSERT(func->get_output_definitions().size() == 2);

		Ref<VoxelGeneratorGraph> generator = L::create_generator(func, 4);

		const pg::CompilationResult compilation_result = generator->compile(false);
		VOXEL_TEST_ASSERT_MSG(
				compilation_result.success,
				String("Failed to compile graph: {0}: {1}")
						.format(varray(compilation_result.node_id, compilation_result.message))
		);

		const Vector3i pos(1, 2, 3);
		const float sd = generator->generate_single(pos, VoxelBuffer::CHANNEL_SDF).f;
		const float expected = float(pos.x) + float(pos.z) + func_custom_input_defval;
		VOXEL_TEST_ASSERT(Math::is_equal_approx(sd, expected));
	}
	// 输入节点多于输入参数，但仍应能编译
	{
		Ref<VoxelGraphFunction> func = L::create_misc_function();
		FixedArray<VoxelGraphFunction::Port, 2> inputs;
		inputs[0] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_INPUT_X, "x" };
		inputs[1] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_CUSTOM_INPUT, "custom_input" };
		// 2 个输入节点没有对应的输入
		FixedArray<VoxelGraphFunction::Port, 2> outputs;
		outputs[0] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_OUTPUT_SDF, "sdf" };
		outputs[1] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_CUSTOM_OUTPUT, "custom_output" };
		func->set_io_definitions(to_span(inputs), to_span(outputs));

		Ref<VoxelGeneratorGraph> generator = L::create_generator(func, 2);

		const pg::CompilationResult compilation_result = generator->compile(false);
		VOXEL_TEST_ASSERT_MSG(
				compilation_result.success,
				String("Failed to compile graph: {0}: {1}")
						.format(varray(compilation_result.node_id, compilation_result.message))
		);
	}
	// I/O 节点少于 I/O 数量，但仍应能编译
	{
		Ref<VoxelGraphFunction> func = L::create_misc_function();
		FixedArray<VoxelGraphFunction::Port, 5> inputs;
		inputs[0] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_INPUT_X, "x" };
		inputs[1] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_CUSTOM_INPUT, "custom_input" };
		inputs[2] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_CUSTOM_INPUT, "custom_input2" };
		inputs[3] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_CUSTOM_INPUT, "custom_input3" };
		inputs[4] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_CUSTOM_INPUT, "custom_input4" };
		// 2 个输入节点没有对应的输入
		FixedArray<VoxelGraphFunction::Port, 3> outputs;
		outputs[0] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_OUTPUT_SDF, "sdf" };
		outputs[1] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_CUSTOM_OUTPUT, "custom_output" };
		outputs[2] = VoxelGraphFunction::Port{ VoxelGraphFunction::NODE_CUSTOM_OUTPUT, "custom_output2" };
		func->set_io_definitions(to_span(inputs), to_span(outputs));

		Ref<VoxelGeneratorGraph> generator = L::create_generator(func, 2);

		const pg::CompilationResult compilation_result = generator->compile(false);
		VOXEL_TEST_ASSERT_MSG(
				compilation_result.success,
				String("Failed to compile graph: {0}: {1}")
						.format(varray(compilation_result.node_id, compilation_result.message))
		);
	}
}

#ifdef VOXEL_ENABLE_GPU

void test_voxel_graph_issue461() {
	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	generator->get_main_function()->create_node(VoxelGraphFunction::NODE_OUTPUT_TYPE, Vector2(), -69);
	generator->debug_load_waves_preset();
	generator->debug_load_waves_preset();
	// 此处曾导致崩溃
	VoxelGenerator::ShaderSourceData ssd;
	generator->get_shader_source(ssd);
}

#endif

template <typename T>
void get_node_types(const NodeTypeDB &type_db, StdVector<VoxelGraphFunction::NodeTypeID> &types, T predicate) {
	for (unsigned int i = 0; i < VoxelGraphFunction::NODE_TYPE_COUNT; ++i) {
		const NodeType &type = type_db.get_type(i);
		if (predicate(type)) {
			types.push_back(VoxelGraphFunction::NodeTypeID(i));
		}
	}
}

// 本测试的目标是发现崩溃。它可能会产生错误，但不应崩溃。
void test_voxel_graph_fuzzing() {
	struct L {
		static String make_random_name(RandomPCG &rng) {
			String name;
			const int len = rng.rand() % 8;
			// 注意，我们允许出现空名称。
			for (int i = 0; i < len; ++i) {
				const char c = 'a' + (rng.rand() % ('z' - 'a'));
				name += c;
			}
			return name;
		}

		static void make_random_graph(VoxelGraphFunction &g, RandomPCG &rng, bool allow_custom_io) {
			const int input_count = rng.rand() % 4;
			const int output_count = rng.rand() % 4;
			const int intermediary_node_count = rng.rand() % 8;

			const NodeTypeDB &type_db = NodeTypeDB::get_singleton();

			StdVector<VoxelGraphFunction::NodeTypeID> input_types;
			get_node_types(
					type_db,
					input_types,
					[](const NodeType &t) { //
						return t.category == CATEGORY_INPUT;
					}
			);

			StdVector<VoxelGraphFunction::NodeTypeID> output_types;
			get_node_types(
					type_db,
					output_types,
					[](const NodeType &t) { //
						return t.category == CATEGORY_OUTPUT;
					}
			);

			if (!allow_custom_io) {
				unordered_remove_value(input_types, VoxelGraphFunction::NODE_CUSTOM_INPUT);
				unordered_remove_value(output_types, VoxelGraphFunction::NODE_CUSTOM_OUTPUT);
			}

			for (int i = 0; i < input_count; ++i) {
				const VoxelGraphFunction::NodeTypeID input_type = input_types[rng.rand() % input_types.size()];
				const uint32_t n = g.create_node(input_type, Vector2());
				g.set_node_name(n, make_random_name(rng));
			}

			for (int i = 0; i < output_count; ++i) {
				const VoxelGraphFunction::NodeTypeID output_type = output_types[rng.rand() % output_types.size()];
				const uint32_t n = g.create_node(output_type, Vector2());
				g.set_node_name(n, make_random_name(rng));
			}

			StdVector<VoxelGraphFunction::NodeTypeID> node_types;
			get_node_types(
					type_db,
					node_types,
					[](const NodeType &t) { //
						return t.category != CATEGORY_OUTPUT && t.category != CATEGORY_INPUT;
					}
			);

			for (int i = 0; i < intermediary_node_count; ++i) {
				const VoxelGraphFunction::NodeTypeID type = node_types[rng.rand() % node_types.size()];
				g.create_node(type, Vector2());
			}

			PackedInt32Array node_ids = g.get_node_ids();
			if (node_ids.size() == 0) {
				VOXEL_PRINT_VERBOSE("Empty graph");
				return;
			}
			const int connection_attempts = rng.rand() % (node_ids.size() + 1);

			for (int i = 0; i < connection_attempts; ++i) {
				const int src_node_id = node_ids[rng.rand() % node_ids.size()];
				const int dst_node_id = node_ids[rng.rand() % node_ids.size()];

				const int src_output_count = g.get_node_output_count(src_node_id);
				const int dst_input_count = g.get_node_input_count(dst_node_id);

				if (src_output_count == 0 || dst_input_count == 0) {
					continue;
				}

				const int src_output_index = rng.rand() % src_output_count;
				const int dst_input_index = rng.rand() % dst_input_count;

				if (g.can_connect(src_node_id, src_output_index, dst_node_id, dst_input_index)) {
					g.add_connection(src_node_id, src_output_index, dst_node_id, dst_input_index);
				}
			}
		}
	};

	const int attempts = 1000;

	RandomPCG rng;
	rng.seed(131183);

	int successful_compiles_count = 0;

	// print_line("--- Begin of zone with possible errors ---");

	for (int i = 0; i < attempts; ++i) {
		VOXEL_PRINT_VERBOSE(format("Testing random graph #{}", i));
		Ref<VoxelGeneratorGraph> generator;
		generator.instantiate();
		L::make_random_graph(
				**generator->get_main_function(),
				rng,
				// 不允许自定义 I/O，因为 VoxelGeneratorGraph 目前无法处理它们
				false
		);
		pg::CompilationResult compilation_result = generator->compile(false);
		if (compilation_result.success) {
			generator->generate_single(Vector3i(1, 2, 3), VoxelBuffer::CHANNEL_SDF);
		} else {
			++successful_compiles_count;
		}
	}

	// print_line("--- End of zone with possible errors ---");
	print_line(format("Successful random compiles: {}/{}", successful_compiles_count, attempts));
}

void test_voxel_graph_sphere_on_plane() {
	static const float RADIUS = 6.f;
	struct L {
		static Ref<VoxelGeneratorGraph> create(bool debug) {
			Ref<VoxelGeneratorGraph> generator;
			generator.instantiate();
			load_graph_with_sphere_on_plane(**generator->get_main_function(), RADIUS);
			pg::CompilationResult compilation_result = generator->compile(debug);
			VOXEL_TEST_ASSERT_MSG(
					compilation_result.success,
					String("Failed to compile graph: {0}: {1}")
							.format(varray(compilation_result.node_id, compilation_result.message))
			);
			return generator;
		}

		static void test_locations(VoxelGeneratorGraph &g) {
			const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
			const float sd_sky_above_sphere = g.generate_single(Vector3i(0, RADIUS + 5, 0), channel).f;
			const float sd_sky_away_from_sphere = g.generate_single(Vector3i(100, RADIUS + 5, 0), channel).f;
			const float sd_ground_below_sphere = g.generate_single(Vector3i(0, -RADIUS - 5, 0), channel).f;
			const float sd_ground_away_from_sphere = g.generate_single(Vector3i(100, -RADIUS - 5, 0), channel).f;
			const float sd_at_sphere_center = g.generate_single(Vector3i(0, 0, 0), channel).f;
			const float sd_in_sphere_but_higher_than_center =
					g.generate_single(Vector3i(RADIUS / 2, RADIUS / 2, RADIUS / 2), channel).f;

			VOXEL_TEST_ASSERT(sd_sky_above_sphere > 0.f);
			VOXEL_TEST_ASSERT(sd_sky_away_from_sphere > 0.f);
			VOXEL_TEST_ASSERT(sd_ground_below_sphere < 0.f);
			VOXEL_TEST_ASSERT(sd_ground_away_from_sphere < 0.f);
			VOXEL_TEST_ASSERT(sd_at_sphere_center < 0.f);
			VOXEL_TEST_ASSERT(sd_in_sphere_but_higher_than_center < 0.f);
			VOXEL_TEST_ASSERT(sd_in_sphere_but_higher_than_center > sd_at_sphere_center);
		}
	};
	Ref<VoxelGeneratorGraph> generator_debug = L::create(true);
	Ref<VoxelGeneratorGraph> generator = L::create(false);
	VOXEL_ASSERT(check_graph_results_are_equal(**generator_debug, **generator));
	L::test_locations(**generator_debug);
	L::test_locations(**generator);
}

#ifdef VOXEL_ENABLE_FAST_NOISE_2

// https://github.com/Voxel/godot_voxel/issues/427
void test_voxel_graph_issue427() {
	Ref<VoxelGeneratorGraph> graph;
	graph.instantiate();
	VoxelGraphFunction &g = **graph->get_main_function();

	const uint32_t n_in_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2()); // 1
	const uint32_t n_sub = g.create_node(VoxelGraphFunction::NODE_SUBTRACT, Vector2()); // 2
	const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2()); // 3
	const uint32_t n_mul = g.create_node(VoxelGraphFunction::NODE_MULTIPLY, Vector2()); // 4
	const uint32_t n_fn2_2d = g.create_node(VoxelGraphFunction::NODE_FAST_NOISE_2_2D, Vector2()); // 5
	const uint32_t n_distance_3d = g.create_node(VoxelGraphFunction::NODE_DISTANCE_3D, Vector2()); // 6

	g.add_connection(n_in_y, 0, n_sub, 0);
	g.add_connection(n_sub, 0, n_out_sdf, 0);
	g.add_connection(n_fn2_2d, 0, n_mul, 0);
	g.add_connection(n_distance_3d, 0, n_mul, 1);
	// 添加这个连接后会崩溃
	g.add_connection(n_mul, 0, n_sub, 1);

	pg::CompilationResult result = graph->compile(true);
	VOXEL_TEST_ASSERT(result.success);
}

#ifdef TOOLS_ENABLED

void test_voxel_graph_hash() {
	Ref<VoxelGraphFunction> graph;
	graph.instantiate();
	VoxelGraphFunction &g = **graph;

	const uint32_t n_in_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2()); // 1
	const uint32_t n_add = g.create_node(VoxelGraphFunction::NODE_ADD, Vector2()); // 2
	const uint32_t n_mul = g.create_node(VoxelGraphFunction::NODE_MULTIPLY, Vector2()); // 3
	const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2()); // 4
	const uint32_t n_fn2_2d = g.create_node(VoxelGraphFunction::NODE_FAST_NOISE_2_2D, Vector2()); // 5

	// 初始哈希
	const uint64_t hash0 = g.get_output_graph_hash();

	// 在尚未连接到输出的节点上设置默认输入
	g.set_node_default_input(n_mul, 1, 2);
	const uint64_t hash1 = g.get_output_graph_hash();
	VOXEL_TEST_ASSERT(hash1 == hash0);

	// 逐步添加直到连接上输出
	g.add_connection(n_in_y, 0, n_add, 0);
	g.add_connection(n_fn2_2d, 0, n_add, 1);
	g.add_connection(n_add, 0, n_mul, 0);
	g.add_connection(n_mul, 0, n_out_sdf, 0);
	const uint64_t hash2 = g.get_output_graph_hash();
	VOXEL_TEST_ASSERT(hash2 != hash0);

	// 只添加一条连接，形成菱形
	g.add_connection(n_fn2_2d, 0, n_mul, 1);
	const uint64_t hash3 = g.get_output_graph_hash();
	VOXEL_TEST_ASSERT(hash3 != hash2);

	// 设置默认输入
	g.set_node_default_input(n_mul, 1, 4);
	const uint64_t hash4 = g.get_output_graph_hash();
	VOXEL_TEST_ASSERT(hash4 != hash3);

	// 设置噪声资源属性
	Ref<FastNoise2> noise = g.get_node_param(n_fn2_2d, 0);
	noise->set_period(noise->get_period() + 10.f);
	const uint64_t hash5 = g.get_output_graph_hash();
	VOXEL_TEST_ASSERT(hash5 != hash4);

	// 用相同属性设置一个不同的噪声实例
	Ref<FastNoise2> noise2 = noise->duplicate();
	g.set_node_param(n_fn2_2d, 0, noise2);
	const uint64_t hash6 = g.get_output_graph_hash();
	VOXEL_TEST_ASSERT(hash6 == hash5);
}

#endif // TOOLS_ENABLED
#endif // VOXEL_ENABLE_FAST_NOISE_2

#ifdef VOXEL_ENABLE_GPU
void test_voxel_graph_issue471() {
	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	Ref<VoxelGraphFunction> func = generator->get_main_function();
	VOXEL_ASSERT(func.is_valid());
	FixedArray<VoxelGraphFunction::Port, 1> inputs;
	inputs[0].name = "test_input";
	inputs[0].type = VoxelGraphFunction::NODE_INPUT_X;
	FixedArray<VoxelGraphFunction::Port, 1> outputs;
	outputs[0].name = "test_output";
	outputs[0].type = VoxelGraphFunction::NODE_OUTPUT_SDF;
	func->set_io_definitions(to_span(inputs), to_span(outputs));
	// 之前会崩溃，因为输入定义未得到满足（图是空的）。它本应以报错失败。
	VoxelGenerator::ShaderSourceData ssd;
	generator->get_shader_source(ssd);
}
#endif

// 曾有一个 bug：生成常规高度地形且同时带有纹理输出时，地下会出现完全或
// 部分填充空气的随机方块，而此类方块本应填充为实体。该问题仅在
// 存在纹理输出节点时才会发生。原因是生成器对离表面足够远的方块提前检测并填充了 SDF
// 的实体部分。但由于还带有纹理输出，生成器
// 仍会继续运行图，仅获取体积纹理数据（这是预期的），但随后又用未被计算的结果覆盖了 SDF，
// 实际是用垃圾数据填充了 SDF。
void test_voxel_graph_unused_single_texture_output() {
	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();

	{
		//               Plane
		//                    \
		// Noise2D --- Mul --- Sub --- OutSDF
		//
		//                             OutSingleTexture

		// 在 Y=0 附近略有起伏的地面，不会高于 10 或低于 -10 体素。

		Ref<VoxelGraphFunction> func = generator->get_main_function();
		VOXEL_ASSERT(func.is_valid());

		const uint32_t n_out_sdf = func->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		const uint32_t n_plane = func->create_node(VoxelGraphFunction::NODE_SDF_PLANE, Vector2());

		const uint32_t n_noise = func->create_node(VoxelGraphFunction::NODE_FAST_NOISE_2D, Vector2());
		Ref<Voxel_FastNoiseLite> fnl;
		fnl.instantiate();
		fnl->set_period(1024);
		fnl->set_fractal_type(Voxel_FastNoiseLite::FRACTAL_RIDGED);
		fnl->set_fractal_octaves(5);
		func->set_node_param(n_noise, 0, fnl);

		const uint32_t n_mul = func->create_node(VoxelGraphFunction::NODE_MULTIPLY, Vector2());
		func->set_node_default_input(n_mul, 1, 10.0);

		const uint32_t n_sub = func->create_node(VoxelGraphFunction::NODE_SUBTRACT, Vector2());

		// const uint32_t n_out_single_texture =
		func->create_node(VoxelGraphFunction::NODE_OUTPUT_SINGLE_TEXTURE, Vector2());

		func->add_connection(n_plane, 0, n_sub, 0);
		func->add_connection(n_noise, 0, n_mul, 0);
		func->add_connection(n_mul, 0, n_sub, 1);
		func->add_connection(n_sub, 0, n_out_sdf, 0);
	}

	CompilationResult result = generator->compile(false);
	VOXEL_TEST_ASSERT(result.success);

	StdVector<Vector3i> block_positions;
	{
		Vector3i bpos;
		for (bpos.z = -4; bpos.z < 4; ++bpos.z) {
			for (bpos.x = -4; bpos.x < 4; ++bpos.x) {
				for (bpos.y = -4; bpos.y < 4; ++bpos.y) {
					block_positions.push_back(bpos);
				}
			}
		}

		struct Comparer {
			inline bool operator()(const Vector3i a, const Vector3i b) const {
				return math::length(to_vec3f(a)) < math::length(to_vec3f(b));
			}
		};
		SortArray<Vector3i, Comparer> sorter;
		sorter.sort(block_positions.data(), block_positions.size());
	}

	VoxelBuffer voxels(VoxelBuffer::ALLOCATOR_DEFAULT);
	const int BLOCK_SIZE = 16;
	const int MIN_MARGIN = 1;
	const int MAX_MARGIN = 2;
	voxels.create(Vector3iUtil::create(BLOCK_SIZE + MIN_MARGIN + MAX_MARGIN));

	for (const Vector3i bpos : block_positions) {
		const Vector3i origin_in_voxels = bpos * BLOCK_SIZE - Vector3iUtil::create(MIN_MARGIN);
		generator->generate_block(VoxelGenerator::VoxelQueryData{ voxels, origin_in_voxels, 0 });

		if (bpos.y <= -2) {
			// 我们预期在此高度以下只有地面（以区块坐标表示）
			for (int z = 0; z < voxels.get_size().z; ++z) {
				for (int x = 0; x < voxels.get_size().x; ++x) {
					for (int y = 0; y < voxels.get_size().y; ++y) {
						const float sd = voxels.get_voxel_f(x, y, z, VoxelBuffer::CHANNEL_SDF);

						if (sd >= 0.f) {
							print_sdf_as_ascii(voxels);
						}

						VOXEL_TEST_ASSERT(sd < 0.f);
					}
				}
			}
		} else if (bpos.y >= 1) {
			// 我们预期在此高度以上只有空气（以区块坐标表示）
			for (int z = 0; z < voxels.get_size().z; ++z) {
				for (int x = 0; x < voxels.get_size().x; ++x) {
					for (int y = 0; y < voxels.get_size().y; ++y) {
						const float sd = voxels.get_voxel_f(x, y, z, VoxelBuffer::CHANNEL_SDF);
						VOXEL_TEST_ASSERT(sd > 0.f);
					}
				}
			}
		}
	}
}

// 曾有一个 bug：使用 Spots2D 节点选取的纹理索引在本应被优化掉的区域返回垃圾值。
// 若关闭本地执行映射优化则不会发生该问题。在这些
// 区域内不存在斑点：范围分析发现 Spots2D 始终返回 0，意味着 Select 忽略它并输出
// 一个常量。但实际表现为它返回了存在斑点的区域中所获得的最后取值。
void test_voxel_graph_spots2d_optimized_execution_map() {
	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();

	const float SPOT_RADIUS = 5.f;
	const float CELL_SIZE = 64.f;
	const float JITTER = 0.f; // 所有斑点都位于各自的格子中心
	const unsigned int TEX_INDEX0 = 0;
	const unsigned int TEX_INDEX1 = 1;

	{
		// generator = ResourceLoader::load("res://local_tests/smooth_materials/smooth_materials_generator_graph.tres");

		Ref<VoxelGraphFunction> func = generator->get_main_function();
		VOXEL_ASSERT(func.is_valid());

		const uint32_t n4_out_sdf = func->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2(), 4);

		const uint32_t n5_plane = func->create_node(VoxelGraphFunction::NODE_SDF_PLANE, Vector2(), 5);
		uint32_t height_input_index;
		VOXEL_ASSERT(
				pg::NodeTypeDB::get_singleton().try_get_input_index_from_name(
						VoxelGraphFunction::NODE_SDF_PLANE, "height", height_input_index
				)
		);
		func->set_node_default_input(n5_plane, height_input_index, 2.f);

		const uint32_t n6_fnl1 = func->create_node(VoxelGraphFunction::NODE_FAST_NOISE_2D, Vector2(), 6);

		const uint32_t n7_mul = func->create_node(VoxelGraphFunction::NODE_MULTIPLY, Vector2(), 7);
		func->set_node_default_input(n7_mul, 1, 1.f); // b

		const uint32_t n9_sub = func->create_node(VoxelGraphFunction::NODE_SUBTRACT, Vector2(), 9);
		/*const uint32_t n11_fnl2 =*/func->create_node(VoxelGraphFunction::NODE_FAST_NOISE_2D, Vector2(), 11);
		const uint32_t n12_out_tex = func->create_node(VoxelGraphFunction::NODE_OUTPUT_SINGLE_TEXTURE, Vector2(), 12);

		const uint32_t n13_select1 = func->create_node(VoxelGraphFunction::NODE_SELECT, Vector2(), 13);
		func->set_node_default_input(n13_select1, 0, 0.f); // a
		func->set_node_default_input(n13_select1, 1, 1.f); // b
		func->set_node_param(n13_select1, 0, 0.5f); // threshold

		const uint32_t n14_fnl3 = func->create_node(VoxelGraphFunction::NODE_FAST_NOISE_2D, Vector2(), 14);

		const uint32_t n15_select2 = func->create_node(VoxelGraphFunction::NODE_SELECT, Vector2(), 15);
		func->set_node_default_input(n15_select2, 0, 0.f); // a
		func->set_node_default_input(n15_select2, 1, 2.f); // b
		func->set_node_param(n15_select2, 0, 0.5f); // threshold

		const uint32_t n16_fnl4 = func->create_node(VoxelGraphFunction::NODE_FAST_NOISE_2D, Vector2(), 16);

		const uint32_t n18_select3 = func->create_node(VoxelGraphFunction::NODE_SELECT, Vector2(), 18);
		func->set_node_default_input(n18_select3, 0, 0.f); // a
		func->set_node_default_input(n18_select3, 1, 3.f); // b
		func->set_node_param(n18_select3, 0, 0.5f); // threshold

		const uint32_t n19_select4 = func->create_node(VoxelGraphFunction::NODE_SELECT, Vector2(), 19);
		func->set_node_default_input(n19_select4, 0, 0.f); // a
		func->set_node_default_input(n19_select4, 1, 4.f); // b
		func->set_node_param(n19_select4, 0, 0.5f); // threshold

		const uint32_t n20_fnl5 = func->create_node(VoxelGraphFunction::NODE_FAST_NOISE_2D, Vector2(), 20);
		const uint32_t n21_fnl6 = func->create_node(VoxelGraphFunction::NODE_FAST_NOISE_2D, Vector2(), 21);

		const uint32_t n22_select5 = func->create_node(VoxelGraphFunction::NODE_SELECT, Vector2(), 22);
		func->set_node_default_input(n22_select5, 0, 0.f); // a
		func->set_node_default_input(n22_select5, 1, 5.f); // b
		func->set_node_param(n22_select5, 0, 0.5f); // threshold

		const uint32_t n23_spots2d = func->create_node(VoxelGraphFunction::NODE_SPOTS_2D, Vector2(), 23);
		uint32_t cell_size_param_index;
		uint32_t jitter_param_index;
		VOXEL_ASSERT(
				pg::NodeTypeDB::get_singleton().try_get_param_index_from_name(
						VoxelGraphFunction::NODE_SPOTS_2D, "cell_size", cell_size_param_index
				)
		);
		VOXEL_ASSERT(
				pg::NodeTypeDB::get_singleton().try_get_param_index_from_name(
						VoxelGraphFunction::NODE_SPOTS_2D, "jitter", jitter_param_index
				)
		);
		func->set_node_param(n23_spots2d, cell_size_param_index, CELL_SIZE);
		func->set_node_param(n23_spots2d, jitter_param_index, JITTER);
		func->set_node_default_input(n23_spots2d, 2, SPOT_RADIUS);

		func->add_connection(n19_select4, 0, n22_select5, 0);
		func->add_connection(n13_select1, 0, n12_out_tex, 0);
		func->add_connection(n14_fnl3, 0, n15_select2, 2);
		func->add_connection(n15_select2, 0, n18_select3, 0);
		func->add_connection(n16_fnl4, 0, n18_select3, 2);
		func->add_connection(n18_select3, 0, n19_select4, 0);
		func->add_connection(n20_fnl5, 0, n19_select4, 2);
		func->add_connection(n21_fnl6, 0, n22_select5, 2);
		func->add_connection(n23_spots2d, 0, n13_select1, 2);
		func->add_connection(n5_plane, 0, n9_sub, 0);
		func->add_connection(n6_fnl1, 0, n7_mul, 0);
		func->add_connection(n7_mul, 0, n9_sub, 1);
		func->add_connection(n9_sub, 0, n4_out_sdf, 0);

		/*
		// Plane --- OutSDF
		//
		//           0
		//            \
		//       1 --- Select --- OutSingleTexture
		//            /
		//     Spots2D
		//
		// 带斑点的平坦地形

		Ref<VoxelGraphFunction> func = generator->get_main_function();
		VOXEL_ASSERT(func.is_valid());

		const uint32_t n_out_sdf = func->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());
		const uint32_t n_plane = func->create_node(VoxelGraphFunction::NODE_SDF_PLANE, Vector2());
		const uint32_t n_select = func->create_node(VoxelGraphFunction::NODE_SELECT, Vector2());
		const uint32_t n_spots2d = func->create_node(VoxelGraphFunction::NODE_SPOTS_2D, Vector2());
		const uint32_t n_out_single_texture =
				func->create_node(VoxelGraphFunction::NODE_OUTPUT_SINGLE_TEXTURE, Vector2());

		uint32_t height_input_index;
		VOXEL_ASSERT(pg::NodeTypeDB::get_singleton().try_get_input_index_from_name(
				VoxelGraphFunction::NODE_SDF_PLANE, "height", height_input_index));
		func->set_node_default_input(n_plane, height_input_index, 1.f);

		uint32_t cell_size_param_index;
		uint32_t jitter_param_index;
		VOXEL_ASSERT(pg::NodeTypeDB::get_singleton().try_get_param_index_from_name(
				VoxelGraphFunction::NODE_SPOTS_2D, "cell_size", cell_size_param_index));
		VOXEL_ASSERT(pg::NodeTypeDB::get_singleton().try_get_param_index_from_name(
				VoxelGraphFunction::NODE_SPOTS_2D, "jitter", jitter_param_index));
		func->set_node_param(n_spots2d, cell_size_param_index, CELL_SIZE);
		func->set_node_param(n_spots2d, jitter_param_index, JITTER);
		func->set_node_default_input(n_spots2d, 2, SPOT_RADIUS);

		func->set_node_default_input(n_select, 0, TEX_INDEX0);
		func->set_node_default_input(n_select, 1, TEX_INDEX1);
		func->set_node_param(n_select, 0, 0.5f); // Threshold

		func->add_connection(n_plane, 0, n_out_sdf, 0);
		func->add_connection(n_spots2d, 0, n_select, 2);
		func->add_connection(n_select, 0, n_out_single_texture, 0);
		//*/
	}

	CompilationResult result = generator->compile(false);
	VOXEL_TEST_ASSERT(result.success);

	struct L {
		static bool has_spot(const VoxelBuffer &vb) {
			Vector3i pos;
			for (pos.z = 0; pos.z < vb.get_size().z; ++pos.z) {
				for (pos.x = 0; pos.x < vb.get_size().x; ++pos.x) {
					for (pos.y = 0; pos.y < vb.get_size().y; ++pos.y) {
						const uint32_t encoded_indices = vb.get_voxel(pos, VoxelBuffer::CHANNEL_INDICES);
						const uint32_t encoded_weights = vb.get_voxel(pos, VoxelBuffer::CHANNEL_WEIGHTS);
						const FixedArray<uint8_t, 4> indices = mixel4::decode_indices_from_packed_u16(encoded_indices);
						const FixedArray<uint8_t, 4> weights = mixel4::decode_weights_from_packed_u16(encoded_weights);
						int indices_with_high_weight = 0;
						bool has_tex1 = false;
						for (unsigned int i = 0; i < 4; ++i) {
							if (weights[i] > 200) {
								const uint8_t ii = indices[i];
								VOXEL_TEST_ASSERT_MSG(
										ii == TEX_INDEX0 || ii == TEX_INDEX1,
										"Expected only one of our two indices with high weight"
								);
								++indices_with_high_weight;
								if (ii == TEX_INDEX1) {
									has_tex1 = true;
								}
							}
						}
						VOXEL_TEST_ASSERT(indices_with_high_weight == 1);
						if (has_tex1) {
							return true;
						}
					}
				}
			}
			return false;
		}

		static StdString print_u16_hex(uint16_t x) {
			const char *s_chars = "0123456789abcdef";
			StdString s;
			for (int i = 3; i >= 0; --i) {
				const unsigned int nibble = (x >> (i * 4)) & 0xf;
				s += s_chars[nibble];
			}
			return s;
		}

		static void print_indices_and_weights(const VoxelBuffer &vb, int y) {
			Vector3i pos(0, y, 0);
			StdString s;
			for (pos.z = 0; pos.z < vb.get_size().z; ++pos.z) {
				for (pos.x = 0; pos.x < vb.get_size().x; ++pos.x) {
					const uint16_t encoded_indices = vb.get_voxel(pos, VoxelBuffer::CHANNEL_INDICES);
					s += print_u16_hex(encoded_indices);
					s += " ";
				}
				s += " | ";
				for (pos.x = 0; pos.x < vb.get_size().x; ++pos.x) {
					const uint16_t encoded_weights = vb.get_voxel(pos, VoxelBuffer::CHANNEL_WEIGHTS);
					s += print_u16_hex(encoded_weights);
					s += " ";
				}
				s += "\n";
			}
			print_line(format("Indices and weights at Y={}:", y));
			print_line(s);
		}
	};

	const int BLOCK_SIZE = 16;

	VoxelBuffer voxels1(VoxelBuffer::ALLOCATOR_DEFAULT);
	voxels1.create(Vector3iUtil::create(BLOCK_SIZE));
	VoxelBuffer voxels2(VoxelBuffer::ALLOCATOR_DEFAULT);
	voxels2.create(Vector3iUtil::create(BLOCK_SIZE));

	// 先执行一次不带优化的运行
	generator->set_use_optimized_execution_map(false);
	{
		// 该区域的右上角有一个斑点
		generator->generate_block(VoxelGenerator::VoxelQueryData{ voxels1, Vector3i(16, 0, 16), 0 });
		// L::print_indices_and_weights(voxels1, 8);
		VOXEL_TEST_ASSERT(L::has_spot(voxels1));

		// 这里没有斑点
		generator->generate_block(VoxelGenerator::VoxelQueryData{ voxels2, Vector3i(0, 0, 0), 0 });
		// L::print_indices_and_weights(voxels2, 8);
		VOXEL_TEST_ASSERT(L::has_spot(voxels2) == false);
	}

	VoxelBuffer voxels3(VoxelBuffer::ALLOCATOR_DEFAULT);
	voxels3.create(Vector3iUtil::create(BLOCK_SIZE));
	VoxelBuffer voxels4(VoxelBuffer::ALLOCATOR_DEFAULT);
	voxels4.create(Vector3iUtil::create(BLOCK_SIZE));

	// 现在执行一次带优化的运行，结果必须相同
	generator->set_use_optimized_execution_map(true);
	{
		generator->generate_block(VoxelGenerator::VoxelQueryData{ voxels3, Vector3i(16, 0, 16), 0 });
		// L::print_indices_and_weights(voxels3, 8);
		VOXEL_TEST_ASSERT(L::has_spot(voxels3));
		VOXEL_TEST_ASSERT(voxels3.equals(voxels1));

		generator->generate_block(VoxelGenerator::VoxelQueryData{ voxels4, Vector3i(0, 0, 0), 0 });
		// L::print_indices_and_weights(voxels4, 8);
		VOXEL_TEST_ASSERT(L::has_spot(voxels4) == false);
		VOXEL_TEST_ASSERT(voxels4.equals(voxels2));
	}

	// 更广泛的测试
	/*{
		struct BlockTest {
			Vector3i origin;
			bool expect_spot;
		};

		StdVector<BlockTest> block_tests;

		generator->set_use_optimized_execution_map(false);

		Vector3i bpos;
		for (bpos.z = -4; bpos.z < 4; ++bpos.z) {
			for (bpos.x = -4; bpos.x < 4; ++bpos.x) {
				const Vector3i origin = bpos * BLOCK_SIZE;
				generator->generate_block(VoxelGenerator::VoxelQueryData{ voxels, origin, 0 });
				block_tests.push_back(BlockTest{ origin, L::has_spot(voxels) });
			}
		}

		generator->set_use_optimized_execution_map(true);

		for (unsigned int bti = 0; bti < block_tests.size(); ++bti) {
			const BlockTest bt = block_tests[bti];
			generator->generate_block(VoxelGenerator::VoxelQueryData{ voxels, bt.origin, 0 });
			const bool spot_found = L::has_spot(voxels);
			VOXEL_TEST_ASSERT(bt.expect_spot == spot_found);
		}
	}*/
}

void test_voxel_graph_unused_inner_output() {
	// 当编译的图在其某个内部节点（非 Output* 节点）存在未使用的输出时，在 debug 下编译
	// 会崩溃，因为它试图为 0 个使用者分配输出缓冲区，而这在 debug 下是明确允许的。
	// 为复现该问题，需要一个有多个输出的节点，且其中一个输出被用于
	// 图的输出。这样该节点会作为程序的一部分被编译，但会有一个未使用的输出。在非 debug 模式下
	// 该输出会作为一次性临时缓冲区分配；但在 debug 模式下无论缓冲区分配是否被优化，所有输出都会被分配。
	// 因为缓冲区分配不会被优化。

	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	{
		Ref<VoxelGraphFunction> g = generator->get_main_function();
		VOXEL_ASSERT(g.is_valid());

		//    X             OutSDF
		//     \           /
		// Y -- Normalize3D -- (unused `ny`)
		//     /         \ \
		//    Z           \ (unused `nz`)
		//                 \
		//                  (unused `len`)

		const uint32_t n_x = g->create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_y = g->create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2());
		const uint32_t n_z = g->create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2());
		const uint32_t n_normalize = g->create_node(VoxelGraphFunction::NODE_NORMALIZE_3D, Vector2());
		const uint32_t n_out = g->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());

		g->add_connection(n_x, 0, n_normalize, 0);
		g->add_connection(n_y, 0, n_normalize, 1);
		g->add_connection(n_z, 0, n_normalize, 2);
		g->add_connection(n_normalize, 0, n_out, 0);
		// 让输出 `ny`、`nz` 和 `len` 保持未使用
	}

	const CompilationResult result_debug = generator->compile(true);
	VOXEL_TEST_ASSERT(result_debug.success);

	const CompilationResult result_ndebug = generator->compile(true);
	VOXEL_TEST_ASSERT(result_ndebug.success);
}

void test_voxel_graph_function_execute() {
	Ref<VoxelGraphFunction> function;
	function.instantiate();

	// out = sin(x) + sin(z + PI/2.0) + y;

	//   X --- sin ----- + --- + --- out
	//                  /     /
	//   Z --- + --- sin     Y
	//        /
	//     PI/2

	{
		const uint32_t n_x = function->create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
		const uint32_t n_y = function->create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2());
		const uint32_t n_z = function->create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2());
		const uint32_t n_sin1 = function->create_node(VoxelGraphFunction::NODE_SIN, Vector2());
		const uint32_t n_sin2 = function->create_node(VoxelGraphFunction::NODE_SIN, Vector2());
		const uint32_t n_add1 = function->create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_add2 = function->create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_add3 = function->create_node(VoxelGraphFunction::NODE_ADD, Vector2());
		const uint32_t n_out_sd = function->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());

		function->add_connection(n_x, 0, n_sin1, 0);
		function->add_connection(n_z, 0, n_add1, 0);
		function->add_connection(n_add1, 0, n_sin2, 0);
		function->add_connection(n_sin1, 0, n_add2, 0);
		function->add_connection(n_sin2, 0, n_add2, 1);
		function->add_connection(n_add2, 0, n_add3, 0);
		function->add_connection(n_y, 0, n_add3, 1);
		function->add_connection(n_add3, 0, n_out_sd, 0);

		function->set_node_default_input(n_add1, 1, math::PI<float> / 2.f);

		function->auto_pick_inputs_and_outputs();
		const CompilationResult result = function->compile(false);
		VOXEL_TEST_ASSERT(result.success);
	}

	const Vector3i block_size(16, 18, 20);
	const size_t volume = Vector3iUtil::get_volume_u64(block_size);

	StdVector<float> x_buffer;
	StdVector<float> y_buffer;
	StdVector<float> z_buffer;
	StdVector<float> sd_buffer;

	x_buffer.resize(volume);
	y_buffer.resize(volume);
	z_buffer.resize(volume);
	sd_buffer.resize(volume);

	{
		unsigned int i = 0;
		for (int z = 0; z < block_size.z; ++z) {
			for (int x = 0; x < block_size.x; ++x) {
				for (int y = 0; y < block_size.y; ++y) {
					x_buffer[i] = x;
					y_buffer[i] = y;
					z_buffer[i] = z;
					++i;
				}
			}
		}
	}

	Span<const float> inputs[3] = { to_span(x_buffer), to_span(y_buffer), to_span(z_buffer) };
	Span<float> outputs = to_span(sd_buffer);
	function->execute(Span<const Span<const float>>(inputs, 3), Span<Span<float>>(&outputs, 1));

	for (size_t i = 0; i < volume; ++i) {
		const float obtained_result = sd_buffer[i];
		const float expected_result = Math::sin(x_buffer[i]) + Math::cos(z_buffer[i]) + y_buffer[i];
		VOXEL_TEST_ASSERT(Math::is_equal_approx(obtained_result, expected_result));
	}
}

void test_voxel_graph_image() {
	struct L {
		static void test_range(Ref<Image> image, Box3i box, math::Interval expected_bound) {
			VOXEL_ASSERT(image.is_valid());
			Ref<VoxelGeneratorGraph> generator;
			generator.instantiate();
			uint32_t n_image;
			{
				Ref<VoxelGraphFunction> g = generator->get_main_function();
				VOXEL_ASSERT(g.is_valid());

				//   X --- * --- Image --- + --- Sdf
				//               /        /
				//        Z --- *    SdfPlane

				const uint32_t n_x = g->create_node(VoxelGraphFunction::NODE_INPUT_X, Vector2());
				const uint32_t n_z = g->create_node(VoxelGraphFunction::NODE_INPUT_Z, Vector2());
				const uint32_t n_mul_x = g->create_node(VoxelGraphFunction::NODE_MULTIPLY, Vector2());
				const uint32_t n_mul_z = g->create_node(VoxelGraphFunction::NODE_MULTIPLY, Vector2());
				n_image = g->create_node(VoxelGraphFunction::NODE_IMAGE_2D, Vector2());
				const uint32_t n_plane = g->create_node(VoxelGraphFunction::NODE_SDF_PLANE, Vector2());
				const uint32_t n_add = g->create_node(VoxelGraphFunction::NODE_ADD, Vector2());
				const uint32_t n_out = g->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());

				g->set_node_param(n_image, 0, image);

				g->set_node_default_input(n_mul_x, 1, 0.25f);
				g->set_node_default_input(n_mul_z, 1, 0.25f);

				g->add_connection(n_x, 0, n_mul_x, 0);
				g->add_connection(n_z, 0, n_mul_z, 0);
				g->add_connection(n_mul_x, 0, n_image, 0);
				g->add_connection(n_mul_z, 0, n_image, 1);
				g->add_connection(n_image, 0, n_add, 0);
				g->add_connection(n_plane, 0, n_add, 1);
				g->add_connection(n_add, 0, n_out, 0);
			}

			CompilationResult result = generator->compile(true);
			VOXEL_TEST_ASSERT(result.success);

			generator->debug_analyze_range(box.position, box.position + box.size, true);

			uint32_t image_output_address;
			VOXEL_TEST_ASSERT(generator->try_get_output_port_address(
					ProgramGraph::PortLocation{ n_image, 0 }, image_output_address
			));

			const pg::Runtime::State &state = generator->get_last_state_from_current_thread();
			const math::Interval image_output_range = state.get_range(image_output_address);

			VOXEL_TEST_ASSERT(expected_bound.contains(image_output_range));
		}
	};

	{
		Ref<Image> image = voxel::godot::create_empty_image(64, 64, false, Image::FORMAT_R8);
		image->fill(Color(0.5f, 0, 0));
		L::test_range(
				image,
				Box3i(Vector3i(0, -8, 0), Vector3i(16, 16, 16)),
				math::Interval(0.5f, 0.5f)
						// 稍微补充一些，因为图像可能只有 8 位精度
						.padded(0.01f)
		);
	}
	{
		Ref<Image> image = voxel::godot::create_empty_image(64, 64, false, Image::FORMAT_R8);
		image->fill(Color(0.5f, 0, 0));
		L::test_range(
				image, Box3i(Vector3i(-24, -8, -8), Vector3i(16, 16, 16)), math::Interval(0.5f, 0.5f).padded(0.01f)
		);
	}
	{
		Ref<Image> image = voxel::godot::create_empty_image(64, 64, false, Image::FORMAT_R8);
		image->fill(Color(0.5f, 0, 0));
		image->set_pixel(8, 8, Color(0.7f, 0, 0));
		L::test_range(
				image, Box3i(Vector3i(-24, -8, -8), Vector3i(16, 16, 16)), math::Interval(0.5f, 0.5f).padded(0.01f)
		);
	}
}

void test_voxel_graph_many_weight_outputs() {
	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	static constexpr unsigned int USED_WEIGHTS_COUNT = 13;
	static constexpr unsigned int PEAKING_INDEX = 5;
	{
		Ref<VoxelGraphFunction> func = generator->get_main_function();
		VOXEL_ASSERT(func.is_valid());

		//
		//  Y --- Sdf
		//    |
		//    --- * --- OutputWeight[PEAKING_INDEX]
		//    |
		//    --- OutputWeight1
		//    |
		//    --- OutputWeight2
		//    |
		//    [...]

		const uint32_t n_y = func->create_node(VoxelGraphFunction::NODE_INPUT_Y, Vector2());
		const uint32_t n_out_sdf = func->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2());

		const uint32_t n_mul = func->create_node(VoxelGraphFunction::NODE_MULTIPLY, Vector2());
		func->set_node_default_input(n_mul, 1, 10.f);

		FixedArray<uint32_t, USED_WEIGHTS_COUNT> weight_outs;
		for (unsigned int i = 0; i < weight_outs.size(); ++i) {
			const unsigned int n_out = func->create_node(VoxelGraphFunction::NODE_OUTPUT_WEIGHT, Vector2());
			func->set_node_param(n_out, 0, i);
			weight_outs[i] = n_out;
		}

		func->add_connection(n_y, 0, n_out_sdf, 0);

		func->add_connection(n_y, 0, n_mul, 0);

		for (unsigned int i = 0; i < weight_outs.size(); ++i) {
			const uint32_t n_out = weight_outs[i];
			if (i == PEAKING_INDEX) {
				func->add_connection(n_mul, 0, n_out, 0);
			} else {
				func->add_connection(n_y, 0, n_out, 0);
			}
		}
	}

	// 之前会崩溃/失败，因为生成器试图计算多余索引，而当我们有超过 4 个时这样做并无实际意义
	//
	const CompilationResult result = generator->compile(false);
	VOXEL_TEST_ASSERT(result.success);

	// TODO 还要运行该图并测试输出吗？
}

void test_image_range_grid() {
	Ref<Image> image_ref = Image::create_empty(300, 400, false, Image::FORMAT_RF);
	Image &image = **image_ref;

	for (int y = 0; y < image.get_height(); ++y) {
		for (int x = 0; x < image.get_width(); ++x) {
			const float h = 10.f + 0.3 * x + 0.1 * y;
			image.set_pixel(x, y, Color(h, h, h));
		}
	}

	using namespace math;

	struct L {
		static Color get_pixel_repeat(const Image &im, const int x, const int y) {
			return im.get_pixel(math::wrap(x, im.get_width()), math::wrap(y, im.get_height()));
		}

		static Interval get_range_repeat(const Image &im, const Interval x_range, const Interval y_range) {
			const int min_x = Math::floor(x_range.min);
			const int min_y = Math::floor(y_range.min);
			const int max_x = Math::ceil(x_range.max);
			const int max_y = Math::ceil(y_range.max);

			Interval i = Interval::from_single_value(get_pixel_repeat(im, min_x, min_y).r);
			for (int y = min_y; y < max_y; ++y) {
				for (int x = min_x; x < max_x; ++x) {
					const float h = get_pixel_repeat(im, x, y).r;
					i.add_point(h);
				}
			}

			return i;
		}

		static void test_range(const Image &im, const ImageRangeGrid &range_grid, const Interval x, const Interval y) {
			const Interval accurate_range = L::get_range_repeat(im, x, y);
			const Interval estimated_range = range_grid.get_range_repeat(x, y);
			VOXEL_TEST_ASSERT(estimated_range.contains(accurate_range));
		}
	};

	voxel::ImageRangeGrid image_range_grid;
	image_range_grid.generate(image);

	const int image_width = image.get_width();
	const int image_height = image.get_height();

	L::test_range(image, image_range_grid, Interval(0, 20), Interval(0, 10));
	L::test_range(image, image_range_grid, Interval(50, 200), Interval(105, 240));
	// Decimal
	L::test_range(image, image_range_grid, Interval(100, 100.5), Interval(100, 100.5));
	// 二的幂
	L::test_range(image, image_range_grid, Interval(16, 32), Interval(64, 80));
	L::test_range(image, image_range_grid, Interval(0, image_width), Interval(0, image_height));
	// 大于图像尺寸
	L::test_range(image, image_range_grid, Interval(-image_width, image_width), Interval(-image_height, image_height));
	L::test_range(
			image,
			image_range_grid,
			Interval(-10 * image_width, 10 * image_width),
			Interval(-5 * image_height, 5 * image_height)
	);
	// 远处
	L::test_range(
			image,
			image_range_grid,
			Interval(-10 * image_width + 50, -10 * image_width + 100),
			Interval(-5 * image_height + 80, -5 * image_height + 90)
	);
	// 跨界
	L::test_range(
			image,
			image_range_grid,
			Interval(image_width - 10, image_width + 10),
			Interval(image_height - 5, image_height + 20)
	);
	L::test_range(
			image,
			image_range_grid,
			Interval(10 * image_width + image_width - 10, 10 * image_width + image_width + 10),
			Interval(5 * image_height + image_height - 5, 5 * image_height + image_height + 20)
	);
}

void test_voxel_graph_many_subdivisions() {
	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	{
		VoxelGraphFunction &g = **generator->get_main_function();

		//  FastNoise3D --- OutSDF

		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF, Vector2(0, 0));
		const uint32_t n_noise = g.create_node(VoxelGraphFunction::NODE_FAST_NOISE_3D, Vector2());

		Ref<Voxel_FastNoiseLite> noise;
		noise.instantiate();
		g.set_node_param(n_noise, 0, noise);

		g.add_connection(n_noise, 0, n_out_sdf, 0);

		CompilationResult result = generator->compile(false);
		VOXEL_TEST_ASSERT(result.success);
	}

	VoxelBuffer vb(VoxelBuffer::ALLOCATOR_DEFAULT);
	vb.create(16, 512, 16);

	// 仅检查它不会崩溃。
	// 子分块曾有一个问题：运行图之前我们将"所需输出"填入一个小数组，
	// 但在下一次子分块时没有重置该过程，最终导致数组越界。
	generator->generate_block(VoxelGenerator::VoxelQueryData{ vb, Vector3i(0, 0, 0), 0 });
}

void test_voxel_graph_non_square_image() {
	// 曾有一个 bug：Image 节点对 X 和 Y 使用了 with，而不是对 Y 使用 height。

	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();
	{
		VoxelGraphFunction &g = **generator->get_main_function();

		const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X);
		const uint32_t n_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y);
		const uint32_t n_z = g.create_node(VoxelGraphFunction::NODE_INPUT_Z);
		const uint32_t n_image = g.create_node(VoxelGraphFunction::NODE_IMAGE_2D);
		const uint32_t n_add = g.create_node(VoxelGraphFunction::NODE_ADD);
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

		Ref<Image> image = Image::create_empty(400, 300, false, Image::FORMAT_R8);
		image->fill(Color(1, 0, 0));
		g.set_node_param(n_image, 0, image);

		g.add_connection(n_x, 0, n_image, 0);
		g.add_connection(n_z, 0, n_image, 1);
		g.add_connection(n_y, 0, n_add, 0);
		g.add_connection(n_image, 0, n_add, 1);
		g.add_connection(n_add, 0, n_out_sdf, 0);

		CompilationResult result = generator->compile(false);
		VOXEL_TEST_ASSERT(result.success);
	}

	const VoxelSingleValue sd = generator->generate_single(Vector3i(405, 2, 305), VoxelBuffer::CHANNEL_SDF);
	VOXEL_TEST_ASSERT(sd.f > 2.9f && sd.f < 3.1);
}

void test_voxel_graph_4_default_weights() { // 与 issue #686 相关
	static constexpr uint32_t block_size = 16;

	struct L {
		// 思路是将数据生成到内存缓存中，若存在垃圾缓冲区可能会影响结果。
		// bug
		static void warmup() {
			Ref<VoxelGeneratorGraph> warmup_generator;
			warmup_generator.instantiate();

			VoxelGraphFunction &g = **warmup_generator->get_main_function();

			// X --- Sin --- Add --- OutSDF
			//              /
			//             Y

			const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X);
			const uint32_t n_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y);
			const uint32_t n_sin = g.create_node(VoxelGraphFunction::NODE_SIN);
			const uint32_t n_add = g.create_node(VoxelGraphFunction::NODE_ADD);
			const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

			g.add_connection(n_x, 0, n_sin, 0);
			g.add_connection(n_sin, 0, n_add, 0);
			g.add_connection(n_add, 0, n_out_sdf, 0);
			g.add_connection(n_y, 0, n_add, 1);

			CompilationResult result = warmup_generator->compile(false);
			VOXEL_TEST_ASSERT(result.success);

			VoxelBuffer buffer(VoxelBuffer::ALLOCATOR_DEFAULT);
			buffer.create(Vector3iUtil::create(block_size));

			VoxelGenerator::VoxelQueryData q{ buffer, Vector3i(0, 0, 0), 0 };
			warmup_generator->generate_block(q);
		}

		static void test(const float test_w0, const float test_w1, const float test_w2, const float test_w3) {
			const uint8_t test_w0b = math::clamp(static_cast<int>(255.0 * test_w0), 0, 255);
			const uint8_t test_w1b = math::clamp(static_cast<int>(255.0 * test_w1), 0, 255);
			const uint8_t test_w2b = math::clamp(static_cast<int>(255.0 * test_w2), 0, 255);
			const uint8_t test_w3b = math::clamp(static_cast<int>(255.0 * test_w3), 0, 255);
			const uint32_t test_ew = mixel4::encode_weights_to_packed_u16_lossy(test_w0b, test_w1b, test_w2b, test_w3b);

			Ref<VoxelGeneratorGraph> generator;
			generator.instantiate();
			{
				VoxelGraphFunction &g = **generator->get_main_function();

				// Y --- Plane --- OutSDF
				//                 OutWeight1
				//                 OutWeight2
				//                 OutWeight3
				//                 OutWeight4

				const uint32_t n_y = g.create_node(VoxelGraphFunction::NODE_INPUT_Y);
				const uint32_t n_plane = g.create_node(VoxelGraphFunction::NODE_SDF_PLANE);
				const uint32_t n_ow0 = g.create_node(VoxelGraphFunction::NODE_OUTPUT_WEIGHT);
				const uint32_t n_ow1 = g.create_node(VoxelGraphFunction::NODE_OUTPUT_WEIGHT);
				const uint32_t n_ow2 = g.create_node(VoxelGraphFunction::NODE_OUTPUT_WEIGHT);
				const uint32_t n_ow3 = g.create_node(VoxelGraphFunction::NODE_OUTPUT_WEIGHT);
				// 把该测试放在最后可以让 bug 暴露出来。不寻常的配置会增加熵。
				const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

				g.set_node_default_input(n_plane, 1, 1.0);

				g.set_node_default_input(n_ow0, 0, test_w0);
				g.set_node_default_input(n_ow1, 0, test_w1);
				g.set_node_default_input(n_ow2, 0, test_w2);
				g.set_node_default_input(n_ow3, 0, test_w3);

				g.set_node_param(n_ow0, 0, 0);
				g.set_node_param(n_ow1, 0, 1);
				g.set_node_param(n_ow2, 0, 2);
				g.set_node_param(n_ow3, 0, 3);

				g.add_connection(n_y, 0, n_plane, 0);
				g.add_connection(n_plane, 0, n_out_sdf, 0);

				CompilationResult result = generator->compile(false);
				VOXEL_TEST_ASSERT(result.success);
			}

			VoxelBuffer buffer(VoxelBuffer::ALLOCATOR_DEFAULT);
			buffer.create(Vector3iUtil::create(block_size));

			VoxelGenerator::VoxelQueryData q{ buffer, Vector3i(0, 0, 0), 0 };
			// generator->set_use_optimized_execution_map(false);
			generator->generate_block(q);

			const uint32_t ei = buffer.get_voxel(10, 0, 0, VoxelBuffer::CHANNEL_INDICES);
			const uint32_t ew = buffer.get_voxel(10, 0, 0, VoxelBuffer::CHANNEL_WEIGHTS);

			const FixedArray<uint8_t, 4> indices = mixel4::decode_indices_from_packed_u16(ei);
			// const FixedArray<uint8_t, 4> weights = decode_weights_from_packed_u16(ew);

			VOXEL_TEST_ASSERT(indices[0] == 0 && indices[1] == 1 && indices[2] == 2 && indices[3] == 3);
			VOXEL_TEST_ASSERT(ew == test_ew);
		}
	};

	// L::warmup();
	L::test(0.5, 0.2, 0.4, 0.8);
	L::test(0.5, 0.0, 0.25, 1.0);
	L::test(0.5, 0.2, 0.4, 0.8);
}

void test_voxel_graph_empty_image() {
	// 此处曾导致崩溃

	Ref<VoxelGeneratorGraph> generator;
	generator.instantiate();

	VoxelGraphFunction &g = **generator->get_main_function();

	const uint32_t n_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X);
	const uint32_t n_z = g.create_node(VoxelGraphFunction::NODE_INPUT_Z);
	const uint32_t n_image = g.create_node(VoxelGraphFunction::NODE_IMAGE_2D);
	const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

	Ref<Image> image;
	image.instantiate();
	g.set_node_param(n_image, 0, image);

	g.add_connection(n_x, 0, n_image, 0);
	g.add_connection(n_z, 0, n_image, 1);
	g.add_connection(n_image, 0, n_out_sdf, 0);

	CompilationResult result = generator->compile(false);

	// 在断言编译结果之前尝试生成。它应失败而不崩溃。
	generator->generate_single(Vector3i(405, 2, 305), VoxelBuffer::CHANNEL_SDF);

	VOXEL_TEST_ASSERT(result.success == false);
}

void test_voxel_graph_constant_reduction() {
	Ref<FastNoiseLite> fnl;
	fnl.instantiate();

	const float const1_value = 1.0;
	const float const2_value = 10.0;

	Ref<VoxelGraphFunction> graph;
	graph.instantiate();
	{
		VoxelGraphFunction &g = **graph;

		//                  X --- Add2 --- Out
		//                       /
		//  C1 --- Add1 --- Noise
		//        /
		//      C2

		const uint32_t n_in_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X);
		const uint32_t n_const1 = g.create_node(VoxelGraphFunction::NODE_CONSTANT);
		const uint32_t n_const2 = g.create_node(VoxelGraphFunction::NODE_CONSTANT);
		const uint32_t n_add1 = g.create_node(VoxelGraphFunction::NODE_ADD);
		const uint32_t n_add2 = g.create_node(VoxelGraphFunction::NODE_ADD);
		const uint32_t n_noise = g.create_node(VoxelGraphFunction::NODE_NOISE_2D);
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

		g.set_node_param(n_const1, 0, const1_value);
		g.set_node_param(n_const2, 0, const2_value);
		g.set_node_param(n_noise, 0, fnl);

		g.add_connection(n_const1, 0, n_add1, 0);
		g.add_connection(n_const2, 0, n_add1, 1);
		g.add_connection(n_add1, 0, n_noise, 0);
		g.add_connection(n_in_x, 0, n_add2, 0);
		g.add_connection(n_noise, 0, n_add2, 1);
		g.add_connection(n_add2, 0, n_out_sdf, 0);
	}

	Ref<VoxelGraphFunction> expected_graph;
	expected_graph.instantiate();
	{
		VoxelGraphFunction &g = **expected_graph;

		// X --- Add2 --- Out

		const uint32_t n_in_x = g.create_node(VoxelGraphFunction::NODE_INPUT_X);
		const uint32_t n_add2 = g.create_node(VoxelGraphFunction::NODE_ADD);
		const uint32_t n_out_sdf = g.create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

		const float n = fnl->get_noise_2d(const1_value + const2_value, 0.f);

		g.set_node_default_input(n_add2, 1, n);

		g.add_connection(n_in_x, 0, n_add2, 0);
		g.add_connection(n_add2, 0, n_out_sdf, 0);
	}

	// TODO 是否有专门针对 `equals` 的测试？
	VOXEL_TEST_ASSERT(graph->equals(**graph));

	const pg::CompilationResult res = graph->expand_and_reduce();
	VOXEL_TEST_ASSERT(res.success);

	VOXEL_TEST_ASSERT(graph->get_nodes_count() == 3);
	VOXEL_TEST_ASSERT(graph->equals(**expected_graph));
}

void test_voxel_graph_multiple_function_instances() {
	Ref<VoxelGraphFunction> function;
	function.instantiate();
	{
		// X --- Add --- Out
		//      /
		//     Y
		const uint32_t n_in_x = function->create_node(VoxelGraphFunction::NODE_INPUT_X);
		const uint32_t n_in_y = function->create_node(VoxelGraphFunction::NODE_INPUT_Y);
		const uint32_t n_add = function->create_node(VoxelGraphFunction::NODE_ADD);
		const uint32_t n_out = function->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

		function->add_connection(n_in_x, 0, n_add, 0);
		function->add_connection(n_in_y, 0, n_add, 1);
		function->add_connection(n_add, 0, n_out, 0);

		function->auto_pick_inputs_and_outputs();
	}

	Ref<VoxelGeneratorGraph> graph;
	graph.instantiate();
	{
		Ref<VoxelGraphFunction> mf = graph->get_main_function();

		//     X
		//      \
		// Y --- F1 --- Add --- Out
		//             /
		//     X --- F2
		//          /
		//         Y
		const uint32_t n_in_x = mf->create_node(VoxelGraphFunction::NODE_INPUT_X);
		const uint32_t n_in_y = mf->create_node(VoxelGraphFunction::NODE_INPUT_Y);
		const uint32_t n_f1 = mf->create_function_node(function);
		const uint32_t n_f2 = mf->create_function_node(function);
		const uint32_t n_add = mf->create_node(VoxelGraphFunction::NODE_ADD);
		const uint32_t n_out = mf->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

		mf->add_connection(n_in_x, 0, n_f1, 0);
		mf->add_connection(n_in_y, 0, n_f1, 1);
		mf->add_connection(n_in_x, 0, n_f2, 0);
		mf->add_connection(n_in_y, 0, n_f2, 1);
		mf->add_connection(n_f1, 0, n_add, 0);
		mf->add_connection(n_f2, 0, n_add, 1);
		mf->add_connection(n_add, 0, n_out, 0);

		const CompilationResult result_debug = graph->compile(true);
		VOXEL_TEST_ASSERT(result_debug.success);
	}
}

void test_voxel_graph_issue783() {
	Ref<VoxelGeneratorGraph> graph;
	graph.instantiate();
	{
		Ref<VoxelGraphFunction> mf = graph->get_main_function();

		//               ---
		//        A1 ---|    B1
		//      /        ---   \
		//     X                Add --- Out
		//      \        ---   /
		//        A2 ---|    B2
		//               ---
		//
		// A1 和 A2 等价
		// B1 和 B2 等价

		// TODO 该测试一定程度上依赖 StdUnorderedMap 的实现细节。
		// 输入条件能否满足我们所测内容取决于节点被添加
		// 的顺序。我们希望优化器的"等价合并"步骤先对 B1 和 B2 求值，再对 A1 和 A2 求值。
		// If A1 and A2 are checked first, they will be merged and input conditions will be different for B1 and B2.
		const uint32_t n_out = mf->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);
		const uint32_t n_add = mf->create_node(VoxelGraphFunction::NODE_ADD);
		const uint32_t n_b2 = mf->create_node(VoxelGraphFunction::NODE_ADD);
		const uint32_t n_b1 = mf->create_node(VoxelGraphFunction::NODE_ADD);
		const uint32_t n_a2 = mf->create_node(VoxelGraphFunction::NODE_ABS);
		const uint32_t n_a1 = mf->create_node(VoxelGraphFunction::NODE_ABS);
		const uint32_t n_in_x = mf->create_node(VoxelGraphFunction::NODE_INPUT_X);

		mf->add_connection(n_in_x, 0, n_a1, 0);
		mf->add_connection(n_in_x, 0, n_a2, 0);
		mf->add_connection(n_a1, 0, n_b1, 0);
		mf->add_connection(n_a1, 0, n_b1, 1);
		mf->add_connection(n_a2, 0, n_b2, 0);
		mf->add_connection(n_a2, 0, n_b2, 1);
		mf->add_connection(n_b1, 0, n_add, 0);
		mf->add_connection(n_b2, 0, n_add, 1);
		mf->add_connection(n_add, 0, n_out, 0);

		// TODO 此处不应报错。需要一种在打印错误时使测试失败的方法。
		const CompilationResult result_debug = graph->compile(true);
		VOXEL_TEST_ASSERT(result_debug.success);
	}
}

void test_voxel_graph_broad_block() {
	// generate_broad_block 在填充输出缓冲区时曾对 SDF 进行缩放，但其实不应如此，因为
	// VoxelBuffer 内部已进行缩放。所以当范围分析返回足够小的单值输出时
	// （例如在 Select 中使用 1.0 在某区域输出空气）会被四舍五入为 0，对 Transvoxel 而言这意味着
	// solid.

	Ref<VoxelGeneratorGraph> graph;
	graph.instantiate();

	{
		Ref<VoxelGraphFunction> mf = graph->get_main_function();

		// Y --- SdfPlane --- A
		//                    B Select --- Out
		//              X --- T

		const uint32_t n_x = mf->create_node(VoxelGraphFunction::NODE_INPUT_X);
		const uint32_t n_y = mf->create_node(VoxelGraphFunction::NODE_INPUT_Y);
		const uint32_t n_select = mf->create_node(VoxelGraphFunction::NODE_SELECT);
		const uint32_t n_plane = mf->create_node(VoxelGraphFunction::NODE_SDF_PLANE);
		const uint32_t n_out = mf->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

		mf->add_connection(n_y, 0, n_plane, 0);

		mf->add_connection(n_plane, 0, n_select, 0);
		mf->set_node_default_input(n_select, 1, 1.f);
		const uint32_t param_threshold = 0;
		mf->set_node_param(n_select, param_threshold, 50.f); // x < 50 ? A : B
		mf->add_connection(n_x, 0, n_select, 2);

		mf->add_connection(n_select, 0, n_out, 0);
	}

	const CompilationResult comp_result = graph->compile(false);
	VOXEL_TEST_ASSERT(comp_result.success);

	VoxelBuffer voxels(VoxelBuffer::ALLOCATOR_DEFAULT);
	voxels.create(Vector3i(16, 16, 16));

	VoxelGenerator::VoxelQueryData query{ voxels, Vector3i(224, -32, 0), 0 };
	const bool is_broad = graph->generate_broad_block(query);
	VOXEL_TEST_ASSERT(is_broad);
	VOXEL_TEST_ASSERT(voxels.get_channel_compression(VoxelBuffer::CHANNEL_SDF) == VoxelBuffer::COMPRESSION_UNIFORM);
	const float sd = voxels.get_voxel_f(Vector3i(0, 0, 0), VoxelBuffer::CHANNEL_SDF);
	VOXEL_TEST_ASSERT(sd > 0.f);
}

void test_voxel_graph_set_default_input_by_name() {
	{
		Ref<VoxelGraphFunction> func;
		func.instantiate();

		const uint32_t node_id = func->create_node(VoxelGraphFunction::NODE_SDF_PLANE);

		const float expected_value = 32.0;
		func->set_node_default_input_by_name(node_id, "height", expected_value);

		const uint32_t input_index = 1;
		const float value = func->get_node_default_input(node_id, input_index);
		VOXEL_TEST_ASSERT(value == expected_value);
	}
	{
		const StringName input0_name = "in0";
		const StringName input1_name = "in1";

		Ref<VoxelGraphFunction> sub_func;
		sub_func.instantiate();
		{
			const uint32_t n_input0 = sub_func->create_node(VoxelGraphFunction::NODE_CUSTOM_INPUT);
			const uint32_t n_input1 = sub_func->create_node(VoxelGraphFunction::NODE_CUSTOM_INPUT);
			const uint32_t n_add = sub_func->create_node(VoxelGraphFunction::NODE_ADD);
			const uint32_t n_output = sub_func->create_node(VoxelGraphFunction::NODE_OUTPUT_SDF);

			sub_func->set_node_name(n_input0, input0_name);
			sub_func->set_node_name(n_input1, input1_name);

			sub_func->add_connection(n_input0, 0, n_add, 0);
			sub_func->add_connection(n_input1, 0, n_add, 1);
			sub_func->add_connection(n_add, 0, n_output, 0);

			sub_func->auto_pick_inputs_and_outputs();
		}

		Ref<VoxelGraphFunction> func;
		func.instantiate();
		const uint32_t n_sub_func = func->create_function_node(sub_func);
		VOXEL_TEST_ASSERT(n_sub_func != ProgramGraph::NULL_ID);

		const float expected_value_0 = 32.0;
		const float expected_value_1 = 64.0;

		func->set_node_default_input_by_name(n_sub_func, input0_name, expected_value_0);
		func->set_node_default_input_by_name(n_sub_func, input1_name, expected_value_1);

		const uint32_t input0_index = 0;
		const uint32_t input1_index = 1;

		const float value0 = func->get_node_default_input(n_sub_func, input0_index);
		const float value1 = func->get_node_default_input(n_sub_func, input1_index);

		VOXEL_TEST_ASSERT(value0 == expected_value_0);
		VOXEL_TEST_ASSERT(value1 == expected_value_1);
	}
}

void test_voxel_graph_get_io_indices() {
	{
		Ref<VoxelGraphFunction> func;
		func.instantiate();
		const uint32_t node_id = func->create_node(VoxelGraphFunction::NODE_SDF_PLANE);
		const int index = func->get_node_input_index(node_id, "height");
		VOXEL_TEST_ASSERT(index == 1);
	}
	{
		const StringName input0_name = "in0";
		const StringName input1_name = "in1";

		const StringName output0_name = "out0";
		const StringName output1_name = "out1";

		Ref<VoxelGraphFunction> sub_func;
		sub_func.instantiate();
		{
			const uint32_t n_input0 = sub_func->create_node(VoxelGraphFunction::NODE_CUSTOM_INPUT);
			const uint32_t n_input1 = sub_func->create_node(VoxelGraphFunction::NODE_CUSTOM_INPUT);
			const uint32_t n_add = sub_func->create_node(VoxelGraphFunction::NODE_ADD);
			const uint32_t n_output0 = sub_func->create_node(VoxelGraphFunction::NODE_CUSTOM_OUTPUT);
			const uint32_t n_output1 = sub_func->create_node(VoxelGraphFunction::NODE_CUSTOM_OUTPUT);

			sub_func->set_node_name(n_input0, input0_name);
			sub_func->set_node_name(n_input1, input1_name);

			sub_func->set_node_name(n_output0, output0_name);
			sub_func->set_node_name(n_output1, output1_name);

			sub_func->add_connection(n_input0, 0, n_add, 0);
			sub_func->add_connection(n_input1, 0, n_add, 1);
			sub_func->add_connection(n_add, 0, n_output0, 0);
			sub_func->add_connection(n_add, 0, n_output1, 0);

			sub_func->auto_pick_inputs_and_outputs();
		}

		Ref<VoxelGraphFunction> func;
		func.instantiate();
		const uint32_t n_sub_func = func->create_function_node(sub_func);
		VOXEL_TEST_ASSERT(n_sub_func != ProgramGraph::NULL_ID);

		const uint32_t input0_index = func->get_node_input_index(n_sub_func, input0_name);
		const uint32_t input1_index = func->get_node_input_index(n_sub_func, input1_name);

		const uint32_t output0_index = func->get_node_output_index(n_sub_func, output0_name);
		const uint32_t output1_index = func->get_node_output_index(n_sub_func, output1_name);

		VOXEL_TEST_ASSERT(input0_index == 0);
		VOXEL_TEST_ASSERT(input1_index == 1);

		VOXEL_TEST_ASSERT(output0_index == 0);
		VOXEL_TEST_ASSERT(output1_index == 1);
	}
}

} // namespace voxel::tests
