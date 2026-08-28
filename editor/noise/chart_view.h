#ifndef VOXEL_CHART_VIEW_H
#define VOXEL_CHART_VIEW_H

#include "../../util/containers/span.h"
#include "../../util/godot/classes/control.h"
#include "../../util/godot/core/packed_vector2_array.h"
#include "../../util/godot/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class Line2D)

namespace voxel {

class VOXEL_ChartView : public Control {
	GDCLASS(VOXEL_ChartView, Control)
public:
	VOXEL_ChartView();

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
