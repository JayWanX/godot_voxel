#ifndef VOXEL_FAST_NOISE_LITE_EDITOR_PLUGIN_H
#define VOXEL_FAST_NOISE_LITE_EDITOR_PLUGIN_H

#include "../../util/godot/classes/editor_plugin.h"

namespace voxel {

class VOXEL_NoiseAnalysisWindow;

class VOXEL_FastNoiseLiteEditorPlugin : public voxel::godot::VOXEL_EditorPlugin {
	GDCLASS(VOXEL_FastNoiseLiteEditorPlugin, voxel::godot::VOXEL_EditorPlugin)
public:
	VOXEL_FastNoiseLiteEditorPlugin();

protected:
	String _voxel_get_plugin_name() const override;

private:
	void _notification(int p_what);

	static void _bind_methods() {}

	VOXEL_NoiseAnalysisWindow *_noise_analysis_window = nullptr;
};

} // namespace voxel

#endif // VOXEL_FAST_NOISE_LITE_EDITOR_PLUGIN_H
