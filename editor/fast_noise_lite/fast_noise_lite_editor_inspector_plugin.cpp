#include "fast_noise_lite_editor_inspector_plugin.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite.h"
#include "../../util/noise/fast_noise_lite/fast_noise_lite_gradient.h"

namespace voxel {

bool VOXEL_FastNoiseLiteEditorInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return Object::cast_to<VOXEL_FastNoiseLite>(p_object) != nullptr ||
			Object::cast_to<VOXEL_FastNoiseLiteGradient>(p_object) != nullptr;
}

void VOXEL_FastNoiseLiteEditorInspectorPlugin::_voxel_parse_begin(Object *p_object) {
	const VOXEL_FastNoiseLite *noise_ptr = Object::cast_to<VOXEL_FastNoiseLite>(p_object);
	if (noise_ptr != nullptr) {
		Ref<VOXEL_FastNoiseLite> noise(noise_ptr);

		VOXEL_FastNoiseLiteViewer *viewer = memnew(VOXEL_FastNoiseLiteViewer);
		viewer->set_noise(noise);
		viewer->set_noise_analysis_window(_noise_analysis_window);
		add_custom_control(viewer);
		return;
	}
	const VOXEL_FastNoiseLiteGradient *noise_gradient_ptr = Object::cast_to<VOXEL_FastNoiseLiteGradient>(p_object);
	if (noise_gradient_ptr != nullptr) {
		Ref<VOXEL_FastNoiseLiteGradient> noise_gradient(noise_gradient_ptr);

		VOXEL_FastNoiseLiteViewer *viewer = memnew(VOXEL_FastNoiseLiteViewer);
		viewer->set_noise_gradient(noise_gradient);
		add_custom_control(viewer);
		return;
	}
}

} // namespace voxel
