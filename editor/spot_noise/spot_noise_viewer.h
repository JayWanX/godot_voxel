#ifndef VOXEL_SPOT_NOISE_VIEWER_H
#define VOXEL_SPOT_NOISE_VIEWER_H

#include "../../util/godot/classes/control.h"
#include "../../util/godot/macros.h"
#include "../../util/noise/spot_noise_gd.h"

VOXEL_GODOT_FORWARD_DECLARE(class TextureRect)

namespace voxel {

class Voxel_SpotNoiseViewer : public Control {
	GDCLASS(Voxel_SpotNoiseViewer, Control)
public:
	static const int PREVIEW_WIDTH = 300;
	static const int PREVIEW_HEIGHT = 150;

	Voxel_SpotNoiseViewer();

	void set_noise(Ref<Voxel_SpotNoise> noise);

private:
	void _on_noise_changed();
	void _notification(int p_what);

	void update_preview();

	static void _bind_methods();

	Ref<Voxel_SpotNoise> _noise;
	float _time_before_update = -1.f;
	TextureRect *_texture_rect = nullptr;
};

} // namespace voxel

#endif // VOXEL_SPOT_NOISE_VIEWER_H
