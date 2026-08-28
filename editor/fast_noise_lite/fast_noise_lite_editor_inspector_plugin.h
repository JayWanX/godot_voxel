#ifndef VOXEL_FAST_NOISE_LITE_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_FAST_NOISE_LITE_EDITOR_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"
#include "../../util/macros.h"
#include "fast_noise_lite_viewer.h"

namespace voxel {

class Voxel_NoiseAnalysisWindow;

class Voxel_FastNoiseLiteEditorInspectorPlugin : public voxel::godot::Voxel_EditorInspectorPlugin {
	GDCLASS(Voxel_FastNoiseLiteEditorInspectorPlugin, voxel::godot::Voxel_EditorInspectorPlugin)
public:
	void set_noise_analysis_window(Voxel_NoiseAnalysisWindow *noise_analysis_window) {
		_noise_analysis_window = noise_analysis_window;
	}

protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	void _voxel_parse_begin(Object *p_object) override;

private:
	static void _bind_methods() {}

	Voxel_NoiseAnalysisWindow *_noise_analysis_window = nullptr;
};

} // namespace voxel

#endif // VOXEL_FAST_NOISE_LITE_EDITOR_INSPECTOR_PLUGIN_H
