#include "spot_noise_editor_plugin.h"
#include "../../util/noise/spot_noise.h"
#include "spot_noise_editor_inspector_plugin.h"

namespace voxel {

Voxel_SpotNoiseEditorPlugin::Voxel_SpotNoiseEditorPlugin() {
	Ref<Voxel_SpotNoiseEditorInspectorPlugin> plugin;
	plugin.instantiate();
	add_inspector_plugin(plugin);
}

String Voxel_SpotNoiseEditorPlugin::_voxel_get_plugin_name() const {
	return Voxel_SpotNoiseEditorPlugin::get_class_static();
}

} // namespace voxel
