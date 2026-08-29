#include "chart_view.h"
#include "../../util/godot/classes/font.h"
#include "../../util/godot/classes/line_2d.h"
#include "../../util/godot/core/packed_arrays.h"
#include "../../util/godot/editor_scale.h"
#include "../../util/godot/string_names.h"
#include "../../util/math/funcs.h"

namespace voxel {

Voxel_ChartView::Voxel_ChartView() {
	_line_renderer = memnew(Line2D);
	add_child(_line_renderer);

	set_clip_contents(true);

	_view_min = Vector2(-1, -1);
	_view_max = Vector2(1, 1);
}

void Voxel_ChartView::set_points(Span<const Vector2> points) {
	_points.resize(points.size());
	points.copy_to(to_span(_points));
	queue_redraw();
}

void Voxel_ChartView::auto_fit_view(Vector2 margin_ratios) {
	if (_points.size() > 0) {
		Vector2 min_point = _points[0];
		Vector2 max_point = min_point;
		for (int i = 1; i < _points.size(); ++i) {
			const Vector2 p = _points[i];
			min_point.x = math::min(min_point.x, p.x);
			min_point.y = math::min(min_point.y, p.y);
			max_point.x = math::max(max_point.x, p.x);
			max_point.y = math::max(max_point.y, p.y);
		}
		_view_min = min_point;
		_view_max = max_point;
	}

	const Vector2 view_size = _view_max - _view_min;
	_view_min -= view_size * margin_ratios;
	_view_max += view_size * margin_ratios;

	queue_redraw();
}

void Voxel_ChartView::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW:
			on_draw();
			break;

			// case NOTIFICATION_RESIZED:
			// 	update();
			// 	break;

		default:
			break;
	}
}

void Voxel_ChartView::on_draw() {
	const Color line_color(Color(0.8, 0.8, 0.8, 1.0));
	const Color x_axis_color(Color(1.0, 1.0, 1.0, 0.5));
	const Color y_axis_color(Color(1.0, 1.0, 1.0, 0.5));

	const Vector2 view_size_pixels = get_rect().size;

	// 背景

	const voxel::godot::StringNames &sn = voxel::godot::StringNames::get_singleton();

	draw_style_box(get_theme_stylebox(sn.bg, sn.Tree), Rect2(Point2(), view_size_pixels));

	if (_view_min.is_equal_approx(_view_max) || _points.size() == 0) {
		return;
	}

	const Vector2 view_size_units = _view_max - _view_min;
	const Vector2 unit_to_pixels = view_size_pixels / view_size_units;

	const Transform2D m = Transform2D( //
			Vector2(unit_to_pixels.x, 0.0), // X 轴
			Vector2(0.0, -unit_to_pixels.y), // Y 轴
			Vector2(-_view_min.x * unit_to_pixels.x,
					view_size_pixels.y + _view_min.y * unit_to_pixels.y) // 像素偏移
	);

	// draw_set_transform_matrix();
	//  不能使用这个，因为与 Godot 3 不同，`draw_polyline` 绘制线条的粗细会被视图变换“适当地”
	//  缩放和拉伸。也不可能指定不同的宽度来
	//  补偿这一点，因为缩放因子不是均匀的。所以相反，我们手动应用视图变换。

	// 线

	if (_visual_points.size() != _points.size()) {
		_visual_points.resize(_points.size());
	}
	// 通过 Span 写入底层的打包数组。
	Span<Vector2> visual_points = to_span(_visual_points);
	for (int i = 0; i < _points.size(); ++i) {
		visual_points[i] = m.xform(_points[i]);
	}

	// draw_polyline(_visual_points, line_color, 2.0, true);
	//  即使线宽为 2 并开启抗锯齿，`draw_polyline` 看起来也很差（点状、锯齿、线宽不一致）。
	//  Line2D 一直更好。
	_line_renderer->set_width(2.f);
	_line_renderer->set_begin_cap_mode(Line2D::LINE_CAP_NONE);
	_line_renderer->set_end_cap_mode(Line2D::LINE_CAP_NONE);
	_line_renderer->set_antialiased(true);
	_line_renderer->set_joint_mode(Line2D::LINE_JOINT_BEVEL);
	_line_renderer->set_default_color(line_color);
	_line_renderer->set_points(_visual_points);

	// 坐标轴

	draw_line(m.xform(Vector2(_view_min.x, 0.0)), m.xform(Vector2(_view_max.x, 0.0)), x_axis_color);
	draw_line(m.xform(Vector2(0.0, _view_min.y)), m.xform(Vector2(0.0, _view_max.y)), y_axis_color);

	// 刻度

	Ref<Font> font = get_theme_font(sn.font, sn.Label);
	const int font_size = get_theme_font_size(sn.font_size, sn.Label);
	const Color text_color = get_theme_color(sn.font_color, sn.Editor);

	const int font_height = font->get_height(font_size);
	const Vector2 text_offset(2.f * EDSCALE, -2.f * EDSCALE);
	draw_string(
			font,
			Vector2(0, font_height) + text_offset,
			String::num_real(_view_max.y),
			HORIZONTAL_ALIGNMENT_LEFT,
			-1.f,
			font_size,
			text_color
	);
	draw_string(
			font,
			Vector2(0, view_size_pixels.y) + text_offset,
			String::num_real(_view_min.y),
			HORIZONTAL_ALIGNMENT_LEFT,
			-1.f,
			font_size,
			text_color
	);

	// TODO 绘制悬停值
}

} // namespace voxel
