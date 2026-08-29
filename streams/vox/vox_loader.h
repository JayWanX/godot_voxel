#ifndef VOX_LOADER_H
#define VOX_LOADER_H

#include "../../storage/voxel_buffer_gd.h"
#include "../../util/godot/classes/ref_counted.h"

namespace voxel {

class VoxelColorPalette;

// MagicaVoxel 的简单加载器
class VoxelVoxLoader : public RefCounted {
	GDCLASS(VoxelVoxLoader, RefCounted);

public:
	static int /*Error*/ load_from_file(
			String fpath,
			Ref<godot::VoxelBuffer> p_voxels,
			Ref<VoxelColorPalette> palette,
			godot::VoxelBuffer::ChannelId dst_channel
	);
	// TODO 采用分块加载以获得更好的内存使用
	// TODO 保存

private:
	static void _bind_methods();
};

} // namespace voxel

#endif // VOX_LOADER_H
