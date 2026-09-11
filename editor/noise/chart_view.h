#include <scene/gui/control.h>
#include <core/variant/variant.h>
#ifndef VOXEL_CHART_VIEW_H
#define VOXEL_CHART_VIEW_H

#include "../../util/containers/span.h"
#include <scene/gui/control.h>
#include <core/variant/variant.h>
#include "../../util/godot/macros.h"

class Line2D;

namespace voxel {

class Voxel_ChartView : public Control {
	GDCLASS(Voxel_ChartView, Control)
public:
	Voxel_ChartView();

	void set_points(Span<const Vector2> points);
	void auto_fit_view(Vector2 margin_ratios);

private:
	static void _bind_methods() {}

	void _notification(int p_what);
	void on_draw();

private:
	PackedVector2Array _points;
	PackedVector2Array _visual_points;
	Vector2 _view_min;
	Vector2 _view_max;
	Line2D *_line_renderer = nullptr;
};

} // namespace voxel

#endif // VOXEL_CHART_VIEW_H
