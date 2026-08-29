#ifndef NOISE_ANALYSIS_WINDOW_H
#define NOISE_ANALYSIS_WINDOW_H

#include "../../util/godot/classes/accept_dialog.h"
#include "../../util/godot/core/random_pcg.h"
#include "../../util/godot/macros.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite.h"
#include "noise_adapter.h"

VOXEL_GODOT_FORWARD_DECLARE(class SpinBox)
VOXEL_GODOT_FORWARD_DECLARE(class LineEdit)
VOXEL_GODOT_FORWARD_DECLARE(class ProgressBar)
VOXEL_GODOT_FORWARD_DECLARE(class OptionButton)

namespace voxel {

class Voxel_ChartView;

// 这是一个实验性工具，用于经验性地检查噪声属性，
// 通过大量采样并观察最小值和最大值。
class Voxel_NoiseAnalysisWindow : public AcceptDialog {
	GDCLASS(Voxel_NoiseAnalysisWindow, AcceptDialog)
public:
	Voxel_NoiseAnalysisWindow();

#ifdef VOXEL_ENABLE_FAST_NOISE_2
	void set_noise(Ref<FastNoise2> noise);
#endif
	void set_noise(Ref<Voxel_FastNoiseLite> noise);

private:
	enum Dimension { //
		DIMENSION_2D = 0,
		DIMENSION_3D,
		_DIMENSION_COUNT
	};

	void _on_calculate_button_pressed();
	void _notification(int p_what);
	void process();

	static void _bind_methods();

	NoiseAdapter _adapter;

	OptionButton *_dimension_option_button = nullptr;
	SpinBox *_step_count_spinbox = nullptr;
	SpinBox *_step_minimum_length_spinbox = nullptr;
	SpinBox *_step_maximum_length_spinbox = nullptr;
	SpinBox *_area_size_spinbox = nullptr;
	SpinBox *_samples_count_spinbox = nullptr;

	Voxel_ChartView *_chart_view = nullptr;

	ProgressBar *_progress_bar = nullptr;

	LineEdit *_minimum_value_line_edit = nullptr;
	LineEdit *_maximum_value_line_edit = nullptr;
	LineEdit *_maximum_derivative_line_edit = nullptr;

	Button *_calculate_button = nullptr;

	struct AnalysisParams {
		Dimension dimension;
		int step_count;
		float step_minimum_length;
		float step_maximum_length;
		float area_size;
		int samples_count;
	};

	AnalysisParams _analysis_params;
	int _current_step = -1;

	struct AnalysisResults {
		float minimum_value;
		float maximum_value;
		float maximum_derivative;
		PackedVector2Array maximum_derivative_per_step_length;
	};

	AnalysisResults _results;

	RandomPCG _rng;
};

} // namespace voxel

#endif // NOISE_ANALYSIS_WINDOW_H
