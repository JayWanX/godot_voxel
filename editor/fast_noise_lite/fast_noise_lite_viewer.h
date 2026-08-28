#ifndef VOXEL_FAST_NOISE_LITE_VIEWER_H
#define VOXEL_FAST_NOISE_LITE_VIEWER_H

#include "../../util/godot/classes/control.h"
#include "../../util/godot/macros.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite.h"

// Required in header for GDExtension builds, due to how virtual methods are implemented
#include "../../util/godot/classes/input_event.h"

VOXEL_GODOT_FORWARD_DECLARE(class TextureRect)
VOXEL_GODOT_FORWARD_DECLARE(class PopupMenu)

namespace voxel {

class VOXEL_NoiseAnalysisWindow;

class VOXEL_FastNoiseLiteViewer : public Control {
	GDCLASS(VOXEL_FastNoiseLiteViewer, Control)
public:
	static const int PREVIEW_WIDTH = 300;
	static const int PREVIEW_HEIGHT = 150;

	enum ContextMenuActions { //
		MENU_ANALYZE = 0
	};

	VOXEL_FastNoiseLiteViewer();

	void set_noise(Ref<VOXEL_FastNoiseLite> noise);
	void set_noise_gradient(Ref<VOXEL_FastNoiseLiteGradient> noise_gradient);

	void set_noise_analysis_window(VOXEL_NoiseAnalysisWindow *win) {
		_noise_analysis_window = win;
	}

#ifdef VOXEL_GODOT
	void gui_input(const Ref<InputEvent> &p_event) override;
#elif defined(VOXEL_GODOT_EXTENSION)
	void _gui_input(const Ref<InputEvent> &p_event) override;
#endif

private:
	void _on_noise_changed();
	void _notification(int p_what);
	void on_context_menu_id_pressed(int id);

	void update_context_menu();

	void update_preview();

	static void _bind_methods();

	Ref<VOXEL_FastNoiseLite> _noise;
	Ref<VOXEL_FastNoiseLiteGradient> _noise_gradient;
	float _time_before_update = -1.f;
	TextureRect *_texture_rect = nullptr;
	PopupMenu *_context_menu = nullptr;
	VOXEL_NoiseAnalysisWindow *_noise_analysis_window = nullptr;
};

} // namespace voxel

#endif // VOXEL_FAST_NOISE_LITE_VIEWER_H
