#include "spot_noise_editor_plugin.h"
#include "../../util/noise/spot_noise.h"
#include "spot_noise_editor_inspector_plugin.h"

namespace voxel {

VOXEL_SpotNoiseEditorPlugin::VOXEL_SpotNoiseEditorPlugin() {
	Ref<VOXEL_SpotNoiseEditorInspectorPlugin> plugin;
	plugin.instantiate();
	add_inspector_plugin(plugin);
}

String VOXEL_SpotNoiseEditorPlugin::_voxel_get_plugin_name() const {
	return VOXEL_SpotNoiseEditorPlugin::get_class_static();
}

} // namespace voxel
