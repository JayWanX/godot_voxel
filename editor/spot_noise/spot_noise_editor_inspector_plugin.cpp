#include "spot_noise_editor_inspector_plugin.h"
#include "../../util/noise/spot_noise_gd.h"
#include "spot_noise_viewer.h"

namespace voxel {

bool VOXEL_SpotNoiseEditorInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return Object::cast_to<VOXEL_SpotNoise>(p_object) != nullptr;
}

void VOXEL_SpotNoiseEditorInspectorPlugin::_voxel_parse_begin(Object *p_object) {
	const VOXEL_SpotNoise *noise_ptr = Object::cast_to<VOXEL_SpotNoise>(p_object);
	if (noise_ptr != nullptr) {
		Ref<VOXEL_SpotNoise> noise(noise_ptr);

		VOXEL_SpotNoiseViewer *viewer = memnew(VOXEL_SpotNoiseViewer);
		viewer->set_noise(noise);
		add_custom_control(viewer);
		return;
	}
}

} // namespace voxel
