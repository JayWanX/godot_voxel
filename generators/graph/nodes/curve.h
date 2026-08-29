#include "../../../util/godot/classes/curve.h"
#include "../../../util/profiling.h"
#include "../curve_utility.h"
#include "../node_type_db.h"

namespace voxel::pg {

void register_curve_node(Span<NodeType> types) {
	using namespace math;

	{
		struct Params {
			// TODO 本应是 `const`，但由于它会自动烘焙而不能如此，这对多线程是个隐患
			Curve *curve;
			const CurveRangeData *curve_range_data;
		};
		NodeType &t = types[VoxelGraphFunction::NODE_CURVE];
		t.name = "Curve";
		t.category = CATEGORY_CONVERT;
		t.inputs.push_back(NodeType::Port("x"));
		t.outputs.push_back(NodeType::Port("out"));
		t.params.push_back(NodeType::Param("curve", Curve::get_class_static(), []() {
			Ref<Curve> curve;
			curve.instantiate();
			// 创建 Curve 时默认的预设不方便使用。
			// 让我们使用线性预设。
			curve->add_point(Vector2(0, 0));
			curve->add_point(Vector2(1, 1));
			curve->set_point_right_mode(0, Curve::TANGENT_LINEAR);
			curve->set_point_left_mode(1, Curve::TANGENT_LINEAR);
			return Variant(curve);
		}));
		t.compile_func = [](CompileContext &ctx) {
			Ref<Curve> curve = ctx.get_param(0);
			if (curve.is_null()) {
				ctx.make_error(String(VOXEL_TTR("{0} instance is null")).format(varray(Curve::get_class_static())));
				return;
			}
			// 确保它已烘焙。我们不希望多线程因为 `interpolate_baked` 中的
			// 写操作而中止……
			curve->bake();
			CurveRangeData *curve_range_data = VOXEL_NEW(CurveRangeData);
			get_curve_monotonic_sections(**curve, curve_range_data->sections);
			Params p;
			p.curve_range_data = curve_range_data;
			p.curve = *curve;
			ctx.set_params(p);
			ctx.add_delete_cleanup(curve_range_data);
		};
		t.process_buffer_func = [](Runtime::ProcessBufferContext &ctx) {
			VOXEL_PROFILE_SCOPE_NAMED("NODE_CURVE");
			const Runtime::Buffer &a = ctx.get_input(0);
			Runtime::Buffer &out = ctx.get_output(0);
			const Params p = ctx.get_params<Params>();
			for (uint32_t i = 0; i < out.size; ++i) {
				out.data[i] = p.curve->sample_baked(a.data[i]);
			}
		};
		t.range_analysis_func = [](Runtime::RangeAnalysisContext &ctx) {
			const Interval a = ctx.get_input(0);
			const Params p = ctx.get_params<Params>();
			if (a.is_single_value()) {
				const float v = p.curve->sample_baked(a.min);
				ctx.set_output(0, Interval::from_single_value(v));
			} else {
				const Interval r = get_curve_range(*p.curve, p.curve_range_data->sections, a);
				ctx.set_output(0, r);
			}
		};
#ifdef VOXEL_ENABLE_GPU
		t.shader_gen_func = [](ShaderGenContext &ctx) {
			Ref<Curve> curve = ctx.get_param(0);
			if (curve.is_null()) {
				ctx.make_error(String(VOXEL_TTR("{0} instance is null")).format(varray(Curve::get_class_static())));
				return;
			}
			std::shared_ptr<ComputeShaderResource> res = ComputeShaderResourceFactory::create_texture_2d(curve);
			const StdString uniform_texture = ctx.add_uniform(std::move(res));

			// 在 Godot 4.4 中，Curve 可以定义超出 0..1 的范围
			const Interval curve_domain = voxel::godot::get_curve_domain(**curve);
			const float curve_domain_range = curve_domain.length();
			const float x_remap_a = 1.f / math::max(curve_domain_range, 0.0001f);
			const float x_remap_b = -curve_domain.min * x_remap_a;

			// 我们偏移 X 以匹配 Godot 的 Curve 所做的插值，因为默认的线性
			// 插值采样器会偏移半个像素
			ctx.add_format(
					"{} = texture({}, vec2({} * {} + {} + 0.5 / float(textureSize({}, 0).x), 0.0)).r;\n",
					ctx.get_output_name(0),
					uniform_texture,
					x_remap_a,
					ctx.get_input_name(0),
					x_remap_b,
					uniform_texture
			);
		};
#endif
	}
}

} // namespace voxel::pg
