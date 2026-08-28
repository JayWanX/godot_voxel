#ifndef VOXEL_FAST_NOISE_LITE_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_FAST_NOISE_LITE_EDITOR_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"
#include "../../util/macros.h"
#include "fast_noise_lite_viewer.h"

namespace voxel {

class VOXEL_NoiseAnalysisWindow;

class VOXEL_FastNoiseLiteEditorInspectorPlugin : public voxel::godot::VOXEL_EditorInspectorPlugin {
	GDCLASS(VOXEL_FastNoiseLiteEditorInspectorPlugin, voxel::godot::VOXEL_EditorInspectorPlugin)
public:
	void set_noise_analysis_window(VOXEL_NoiseAnalysisWindow *noise_analysis_window) {
		_noise_analysis_window = noise_analysis_window;
	}

protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	void _voxel_parse_begin(Object *p_object) override;

private:
	// When compiling with GodotCpp, `_bind_methods` isn't optional.
	static void _bind_methods() {}

	VOXEL_NoiseAnalysisWindow *_noise_analysis_window = nullptr;
};

} // namespace voxel

#endif // VOXEL_FAST_NOISE_LITE_EDITOR_INSPECTOR_PLUGIN_H
