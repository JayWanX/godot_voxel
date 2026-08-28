#ifndef VOXEL_SPOT_NOISE_EDITOR_INSPECTOR_PLUGIN_H
#define VOXEL_SPOT_NOISE_EDITOR_INSPECTOR_PLUGIN_H

#include "../../util/godot/classes/editor_inspector_plugin.h"
#include "../../util/macros.h"

namespace voxel {

class VOXEL_SpotNoiseEditorInspectorPlugin : public voxel::godot::VOXEL_EditorInspectorPlugin {
	GDCLASS(VOXEL_SpotNoiseEditorInspectorPlugin, voxel::godot::VOXEL_EditorInspectorPlugin)
protected:
	bool _voxel_can_handle(const Object *p_object) const override;
	void _voxel_parse_begin(Object *p_object) override;

private:
	// When compiling with GodotCpp, `_bind_methods` isn't optional.
	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_SPOT_NOISE_EDITOR_INSPECTOR_PLUGIN_H
