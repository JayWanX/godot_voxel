#include "fast_noise_lite_editor_plugin.h"
#include "../../util/godot/classes/editor_interface.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite_gradient.h"
#include "../noise/noise_analysis_window.h"
#include "fast_noise_lite_editor_inspector_plugin.h"
#include "fast_noise_lite_viewer.h"

namespace voxel {

VOXEL_FastNoiseLiteEditorPlugin::VOXEL_FastNoiseLiteEditorPlugin() {}

String VOXEL_FastNoiseLiteEditorPlugin::_voxel_get_plugin_name() const {
	return VOXEL_FastNoiseLite::get_class_static();
}

void VOXEL_FastNoiseLiteEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			Control *base_control = get_editor_interface()->get_base_control();

			_noise_analysis_window = memnew(VOXEL_NoiseAnalysisWindow);
			base_control->add_child(_noise_analysis_window);

			Ref<VOXEL_FastNoiseLiteEditorInspectorPlugin> plugin;
			plugin.instantiate();
			plugin->set_noise_analysis_window(_noise_analysis_window);
			add_inspector_plugin(plugin);
		} break;

		default:
			break;
	}
}

} // namespace voxel
