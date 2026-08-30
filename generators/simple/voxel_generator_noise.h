#ifndef VOXEL_GENERATOR_NOISE_H
#define VOXEL_GENERATOR_NOISE_H

#include "../../storage/voxel_buffer.h"
#include "../../storage/voxel_buffer_gd.h"
#include "../../util/godot/macros.h"
#include "../../util/thread/rw_lock.h"
#include "../voxel_generator.h"

VOXEL_GODOT_FORWARD_DECLARE(class FastNoiseLite)

namespace voxel {

class VoxelGeneratorNoise : public VoxelGenerator {
	GDCLASS(VoxelGeneratorNoise, VoxelGenerator)

public:
	VoxelGeneratorNoise();
	~VoxelGeneratorNoise();

	// 生成数据所在的通道
	void set_channel(VoxelBuffer::ChannelId p_channel);
	VoxelBuffer::ChannelId get_channel() const;

	// 获取生成器使用的通道掩码
	int get_used_channels_mask() const override;

	// 用于生成地形的噪声源
	void set_noise(Ref<FastNoiseLite> noise);
	Ref<FastNoiseLite> get_noise() const;

	// 高度起始值
	void set_height_start(real_t y);
	real_t get_height_start() const;

	// 高度范围
	void set_height_range(real_t hrange);
	real_t get_height_range() const;

	// 生成单个数据块的体素数据
	Result generate_block(VoxelGenerator::VoxelQueryData input) override;

private:
	void _on_noise_changed();

	void _b_set_channel(godot::VoxelBuffer::ChannelId p_channel);
	godot::VoxelBuffer::ChannelId _b_get_channel() const;

	static void _bind_methods();

	Ref<FastNoiseLite> _noise;

	struct Parameters {
		VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
		Ref<FastNoiseLite> noise;
		float height_start = -100;
		float height_range = 200;
	};

	Parameters _parameters;
	RWLock _parameters_lock;
};

} // namespace voxel

#endif // VOXEL_GENERATOR_NOISE_H