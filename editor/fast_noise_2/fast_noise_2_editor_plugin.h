#ifndef FAST_NOISE_2_EDITOR_PLUGIN_H
#define FAST_NOISE_2_EDITOR_PLUGIN_H

#include "../../util/godot/classes/editor_plugin.h"

namespace voxel {

class Voxel_NoiseAnalysisWindow;

class FastNoise2EditorPlugin : public voxel::godot::Voxel_EditorPlugin {
	GDCLASS(FastNoise2EditorPlugin, voxel::godot::Voxel_EditorPlugin)
public:
	FastNoise2EditorPlugin();

protected:
	String _voxel_get_plugin_name() const override {
		return "FastNoise2";
	}

private:
	void init();
	void _notification(int p_what);

	static void _bind_methods() {}

	Voxel_NoiseAnalysisWindow *_noise_analysis_window = nullptr;
};

} // namespace voxel

#endif // FAST_NOISE_2_EDITOR_PLUGIN_H
