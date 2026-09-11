#include <scene/gui/control.h>
#include "../../util/noise/fast_noise_lite/fast_noise_lite.h"
#include <core/input/input_event.h>
#ifndef VOXEL_FAST_NOISE_LITE_VIEWER_H
#define VOXEL_FAST_NOISE_LITE_VIEWER_H

#include <scene/gui/control.h>
#include "../../util/godot/macros.h"
#include <modules/noise/fastnoise_lite.h>

// 头文件中需要用到虚方法声明，因此必须包含此头文件。
#include <core/input/input_event.h>

class TextureRect;
class PopupMenu;

namespace voxel {

class Voxel_NoiseAnalysisWindow;

class Voxel_FastNoiseLiteViewer : public Control {
	GDCLASS(Voxel_FastNoiseLiteViewer, Control)
public:
	static const int PREVIEW_WIDTH = 300;
	static const int PREVIEW_HEIGHT = 150;

	enum ContextMenuActions { //
		MENU_ANALYZE = 0
	};

	Voxel_FastNoiseLiteViewer();

	void set_noise(Ref<Voxel_FastNoiseLite> noise);
	void set_noise_gradient(Ref<Voxel_FastNoiseLiteGradient> noise_gradient);

	void set_noise_analysis_window(Voxel_NoiseAnalysisWindow *win) {
		_noise_analysis_window = win;
	}

	void gui_input(const Ref<InputEvent> &p_event) override;

private:
	void _on_noise_changed();
	void _notification(int p_what);
	void on_context_menu_id_pressed(int id);

	void update_context_menu();

	void update_preview();

	static void _bind_methods();

	Ref<Voxel_FastNoiseLite> _noise;
	Ref<Voxel_FastNoiseLiteGradient> _noise_gradient;
	float _time_before_update = -1.f;
	TextureRect *_texture_rect = nullptr;
	PopupMenu *_context_menu = nullptr;
	Voxel_NoiseAnalysisWindow *_noise_analysis_window = nullptr;
};

} // namespace voxel

#endif // VOXEL_FAST_NOISE_LITE_VIEWER_H
