#ifndef VOXEL_SPOT_NOISE_EDITOR_PLUGIN_H
#define VOXEL_SPOT_NOISE_EDITOR_PLUGIN_H

#include "../../util/godot/classes/editor_plugin.h"

namespace voxel {

class VOXEL_SpotNoiseEditorPlugin : public voxel::godot::VOXEL_EditorPlugin {
	GDCLASS(VOXEL_SpotNoiseEditorPlugin, voxel::godot::VOXEL_EditorPlugin)
public:
	VOXEL_SpotNoiseEditorPlugin();

protected:
	String _voxel_get_plugin_name() const override;

private:
	// When compiling with GodotCpp, `_bind_methods` is not optional
	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_SPOT_NOISE_EDITOR_PLUGIN_H
