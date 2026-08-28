#include "spot_noise_editor_inspector_plugin.h"
#include "../../util/noise/spot_noise_gd.h"
#include "spot_noise_viewer.h"

namespace voxel {

bool Voxel_SpotNoiseEditorInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return Object::cast_to<Voxel_SpotNoise>(p_object) != nullptr;
}

void Voxel_SpotNoiseEditorInspectorPlugin::_voxel_parse_begin(Object *p_object) {
	const Voxel_SpotNoise *noise_ptr = Object::cast_to<Voxel_SpotNoise>(p_object);
	if (noise_ptr != nullptr) {
		Ref<Voxel_SpotNoise> noise(noise_ptr);

		Voxel_SpotNoiseViewer *viewer = memnew(Voxel_SpotNoiseViewer);
		viewer->set_noise(noise);
		add_custom_control(viewer);
		return;
	}
}

} // namespace voxel
