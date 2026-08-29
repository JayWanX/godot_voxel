#include "../../../constants/voxel_constants.h"
#include "../../../util/godot/classes/image.h"
#include "../../../util/profiling.h"
#include "../image_range_grid.h"
#include "../node_type_db.h"

namespace voxel::pg {

inline float get_pixel_repeat(const Image &im, int x, int y, int w, int h) {
	return im.get_pixel(math::wrap(x, w), math::wrap(y, h)).r;
}

inline float get_pixel_repeat_linear(const Image &im, float x, float y, int im_w, int im_h) {
	const int x0 = int(Math::floor(x));
	const int y0 = int(Math::floor(y));

	const float xf = x - x0;
	const float yf = y - y0;

	const float h00 = get_pixel_repeat(im, x0, y0, im_w, im_h);
	const float h10 = get_pixel_repeat(im, x0 + 1, y0, im_w, im_h);
	const float h01 = get_pixel_repeat(im, x0, y0 + 1, im_w, im_h);
	const float h11 = get_pixel_repeat(im, x0 + 1, y0 + 1, im_w, im_h);

	// 双线性滤波
	const float h = Math::lerp(Math::lerp(h00, h10, xf), Math::lerp(h01, h11, xf), yf);

	return h;
}

inline float skew3(float x) {
	return (x * x * x + x) * 0.5f;
}

inline math::Interval skew3(math::Interval x) {
	return (cubed(x) + x) * 0.5f;
}

// 这主要用于根据已有的高度图生成星球
inline float sdf_sphere_heightmap(
		float x,
		float y,
		float z,
		float r,
		float m,
		const Image &im,
		float min_h,
		float max_h,
		float norm_x,
		float norm_y
) {
	const float d = Math::sqrt(x * x + y * y + z * z) + 0.0001f;
	const float sd = d - r;
	// 当离高度图足够远时进行优化。
	// 这会引入不连续性，但对于受限存储来说应该没问题
	const float margin = 1.2f * (max_h - min_h);
	if (sd > max_h + margin || sd < min_h - margin) {
		return sd;
	}
	const float nx = x / d;
	const float ny = y / d;
	const float nz = z / d;
	// TODO 可以使用快速的 atan2，它不需要很精确
	// https://github.com/ducha-aiki/fast_atan2/blob/master/fast_atan.cpp
	const float uvx = -Math::atan2(nz, nx) * voxel::math::INV_TAU<float> + 0.5f;
	// 这是 asin(ny)/(PI/2) 的近似
	// TODO 不过在某些情况下可能希望使用真正的函数，
	// 例如当我们想对着色器中的同一地图进行组合时
	const float ys = skew3(ny);
	const float uvy = -0.5f * ys + 0.5f;
	// TODO 当图像以低于体素的分辨率采样时，可以使用双三次插值
	const float h = get_pixel_repeat_linear(im, uvx * norm_x, uvy * norm_y, im.get_width(), im.get_height());
	return sd - m * h;
}

inline math::Interval sdf_sphere_heightmap(
		math::Interval x,
		math::Interval y,
		math::Interval z,
		float r,
		float m,
		const ImageRangeGrid *im_range,
		float norm_x,
		float norm_y
) {
	using namespace math;

	const Interval d = get_length(x, y, z) + 0.0001f;
	const Interval sd = d - r;
	// TODO 由于普通函数中的优化，这里存在不连续性
	// 还不确定如何在这里实现它。最坏的情况下，我们把它去掉
	const Interval nx = x / d;
	const Interval ny = y / d;
	const Interval nz = z / d;

	const Interval ys = skew3(ny);
	const Interval uvy = -0.5f * ys + 0.5f;

	// atan2 返回 -PI 到 PI 之间的结果，但有时角度会环绕，我们必须考虑这一点
	OptionalInterval atan_r1;
	const Interval atan_r0 = atan2(nz, nx, &atan_r1);

	Interval h;
	{
		const Interval uvx = -atan_r0 * voxel::math::INV_TAU<float> + 0.5f;
		h = im_range->get_range_repeat(uvx * norm_x, uvy * norm_y);
	}
	if (atan_r1.valid) {
		const Interval uvx = -atan_r1.value * voxel::math::INV_TAU<float> + 0.5f;
		h.add_interval(im_range->get_range_repeat(uvx * norm_x, uvy * norm_y));
	}

	return sd - m * h;
}

void register_image_nodes(Span<NodeType> types) {
	using namespace math;

	{
		enum Filter : uint32_t { FILTER_NEAREST = 0, FILTER_BILINEAR };
		struct Params {
			const Image *image;
			const ImageRangeGrid *image_range_grid;
			Filter filter;
		};
		NodeType &t = types[VoxelGraphFunction::NODE_IMAGE_2D];
		t.name = "Image";
		t.category = CATEGORY_GENERATE;
		t.inputs.push_back(NodeType::Port("x", 0.f, VoxelGraphFunction::AUTO_CONNECT_X));
		t.inputs.push_back(NodeType::Port("y", 0.f, VoxelGraphFunction::AUTO_CONNECT_Z));
		t.outputs.push_back(NodeType::Port("out"));
		t.params.push_back(NodeType::Param("image", Image::get_class_static(), nullptr));

		t.params.push_back(NodeType::Param("filter", Variant::INT, FILTER_NEAREST));
		NodeType::Param &filter_param = t.params.back();
		filter_param.enum_items.push_back("Nearest");
		filter_param.enum_items.push_back("Bilinear");

		t.compile_func = [](CompileContext &ctx) {
			Ref<Image> image = ctx.get_param(0);
			if (image.is_null()) {
				ctx.make_error(String(VOXEL_TTR("{0} instance is null")).format(varray(Image::get_class_static())));
				return;
			}
			if (image->is_compressed()) {
				ctx.make_error(String(VOXEL_TTR("{0} has a compressed format, this is not supported"))
									   .format(varray(Image::get_class_static())));
				return;
			}
			if (image->is_empty()) {
				ctx.make_error(String(VOXEL_TTR("{0} is empty").format(varray(Image::get_class_static()))));
				return;
			}
			ImageRangeGrid *im_range = VOXEL_NEW(ImageRangeGrid);
			im_range->generate(**image);
			Params p;
			p.image = *image;
			p.image_range_grid = im_range;
			p.filter = static_cast<Filter>(static_cast<int>(ctx.get_param(1)));
			ctx.set_params(p);
			ctx.add_delete_cleanup(im_range);
		};
		t.process_buffer_func = [](Runtime::ProcessBufferContext &ctx) {
			VOXEL_PROFILE_SCOPE_NAMED("NODE_IMAGE_2D");
			const Runtime::Buffer &x = ctx.get_input(0);
			const Runtime::Buffer &y = ctx.get_input(1);
			Runtime::Buffer &out = ctx.get_output(0);
			const Params p = ctx.get_params<Params>();
			const Image &im = *p.image;
			// 缓存图像尺寸以减少 API 调用。
			const int w = im.get_width();
			const int h = im.get_height();
#ifdef DEBUG_ENABLED
			if (w == 0 || h == 0) {
				VOXEL_PRINT_ERROR_ONCE("Image is empty");
				return;
			}
#endif
			// TODO 为最常用的格式做优化路径，`get_pixel` 有点慢
			if (p.filter == FILTER_NEAREST) {
				for (uint32_t i = 0; i < out.size; ++i) {
					out.data[i] = get_pixel_repeat(im, x.data[i], y.data[i], w, h);
				}
			} else {
				for (uint32_t i = 0; i < out.size; ++i) {
					out.data[i] = get_pixel_repeat_linear(im, x.data[i], y.data[i], w, h);
				}
			}
		};
		t.range_analysis_func = [](Runtime::RangeAnalysisContext &ctx) {
			const Interval x = ctx.get_input(0);
			const Interval y = ctx.get_input(1);
			const Params p = ctx.get_params<Params>();
			if (p.filter == FILTER_NEAREST) {
				ctx.set_output(0, p.image_range_grid->get_range_repeat(x, y));
			} else {
				ctx.set_output(0, p.image_range_grid->get_range_repeat({ x.min, x.max + 1 }, { y.min, y.max + 1 }));
			}
		};
	}
	{
		struct Params {
			float radius;
			float factor;
			float min_height;
			float max_height;
			float norm_x;
			float norm_y;
			const Image *image;
			const ImageRangeGrid *image_range_grid;
		};

		NodeType &t = types[VoxelGraphFunction::NODE_SDF_SPHERE_HEIGHTMAP];
		t.name = "SdfSphereHeightmap";
		t.category = CATEGORY_SDF;
		t.inputs.push_back(NodeType::Port("x", 0.f, VoxelGraphFunction::AUTO_CONNECT_X));
		t.inputs.push_back(NodeType::Port("y", 0.f, VoxelGraphFunction::AUTO_CONNECT_Y));
		t.inputs.push_back(NodeType::Port("z", 0.f, VoxelGraphFunction::AUTO_CONNECT_Z));
		t.outputs.push_back(NodeType::Port("sdf"));
		t.params.push_back(NodeType::Param("image", Image::get_class_static(), nullptr));
		t.params.push_back(NodeType::Param("radius", Variant::FLOAT, 10.f));
		t.params.push_back(NodeType::Param("factor", Variant::FLOAT, 1.f));

		t.compile_func = [](CompileContext &ctx) {
			Ref<Image> image = ctx.get_param(0);
			if (image.is_null()) {
				ctx.make_error(String(VOXEL_TTR("{0} instance is null")).format(varray(Image::get_class_static())));
				return;
			}
			if (image->is_compressed()) {
				ctx.make_error(String(VOXEL_TTR("{0} has a compressed format, this is not supported"))
									   .format(varray(Image::get_class_static())));
				return;
			}
			ImageRangeGrid *im_range = VOXEL_NEW(ImageRangeGrid);
			im_range->generate(**image);
			const float factor = ctx.get_param(2);
			const Interval range = im_range->get_range() * factor;
			Params p;
			p.min_height = range.min;
			p.max_height = range.max;
			p.image = *image;
			p.image_range_grid = im_range;
			p.radius = ctx.get_param(1);
			p.factor = factor;
			p.norm_x = image->get_width();
			p.norm_y = image->get_height();
			ctx.set_params(p);
			ctx.add_delete_cleanup(im_range);
		};

		t.process_buffer_func = [](Runtime::ProcessBufferContext &ctx) {
			VOXEL_PROFILE_SCOPE_NAMED("NODE_SDF_SPHERE_HEIGHTMAP");
			const Runtime::Buffer &x = ctx.get_input(0);
			const Runtime::Buffer &y = ctx.get_input(1);
			const Runtime::Buffer &z = ctx.get_input(2);
			Runtime::Buffer &out = ctx.get_output(0);
			// TODO 允许使用双线性滤波？
			const Params p = ctx.get_params<Params>();
			const Image &im = *p.image;
			for (uint32_t i = 0; i < out.size; ++i) {
				out.data[i] = sdf_sphere_heightmap(
						x.data[i],
						y.data[i],
						z.data[i],
						p.radius,
						p.factor,
						im,
						p.min_height,
						p.max_height,
						p.norm_x,
						p.norm_y
				);
			}
		};

		t.range_analysis_func = [](Runtime::RangeAnalysisContext &ctx) {
			const Interval x = ctx.get_input(0);
			const Interval y = ctx.get_input(1);
			const Interval z = ctx.get_input(2);
			const Params p = ctx.get_params<Params>();
			ctx.set_output(
					0, sdf_sphere_heightmap(x, y, z, p.radius, p.factor, p.image_range_grid, p.norm_x, p.norm_y)
			);
		};
	}
}

} // namespace voxel::pg
