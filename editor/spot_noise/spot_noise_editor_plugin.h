#ifndef VOXEL_SPOT_NOISE_EDITOR_PLUGIN_H
#define VOXEL_SPOT_NOISE_EDITOR_PLUGIN_H

#include "../../util/godot/classes/editor_plugin.h"

namespace voxel {

class Voxel_SpotNoiseEditorPlugin : public voxel::godot::Voxel_EditorPlugin {
	GDCLASS(Voxel_SpotNoiseEditorPlugin, voxel::godot::Voxel_EditorPlugin)
public:
	Voxel_SpotNoiseEditorPlugin();

protected:
	String _voxel_get_plugin_name() const override;

private:
	static void _bind_methods() {}
};

} // namespace voxel

#endif // VOXEL_SPOT_NOISE_EDITOR_PLUGIN_H
