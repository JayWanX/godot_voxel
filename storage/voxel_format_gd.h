#ifndef VOXEL_FORMAT_GD_H
#define VOXEL_FORMAT_GD_H

#include "../util/godot/classes/resource.h"
#include "voxel_buffer_gd.h"
#include "voxel_format.h"

namespace voxel::godot {

class VoxelFormat : public Resource {
	GDCLASS(VoxelFormat, Resource)
public:
	// 设置通道的位深
	void set_channel_depth(const VoxelBuffer::ChannelId channel_index, const VoxelBuffer::Depth depth);
	// 获取通道的位深
	VoxelBuffer::Depth get_channel_depth(const VoxelBuffer::ChannelId channel_index) const;

	// 按此格式配置缓冲区的通道位深
	void configure_buffer(Ref<VoxelBuffer> buffer) const;
	// 按此格式创建配置好的缓冲区
	Ref<VoxelBuffer> create_buffer(const Vector3i size) const;

	// 获取内部格式数据
	voxel::VoxelFormat get_internal() const {
		return _internal;
	}

private:
	void _b_set_data(const Array &data);
	Array _b_get_data() const;

	static void _bind_methods();

	voxel::VoxelFormat _internal;
};

} // namespace voxel::godot

#endif // VOXEL_FORMAT_GD_H
