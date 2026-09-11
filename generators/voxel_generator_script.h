#ifndef VOXEL_GENERATOR_SCRIPT_H
#define VOXEL_GENERATOR_SCRIPT_H

#include <core/object/gdvirtual.gen.h>
#include "voxel_generator.h"


namespace voxel {

// 基于脚本的生成器，如 GDScript、C# 或 NativeScript。
// 脚本应正确处理多线程。
class VoxelGeneratorScript : public VoxelGenerator {
	GDCLASS(VoxelGeneratorScript, VoxelGenerator)
public:
	VoxelGeneratorScript();

	// 生成单个数据块的体素数据
	Result generate_block(VoxelGenerator::VoxelQueryData input) override;
	// 获取生成器使用的通道掩码
	int get_used_channels_mask() const override;

	// 当前配置是否可运行
	bool is_runnable() const override;

protected:
	GDVIRTUAL3(_generate_block, Ref<godot::VoxelBuffer>, Vector3i, int)
	GDVIRTUAL0RC(int, _get_used_channels_mask) // 我想 `C` 表示 `const`？

private:
	static void _bind_methods();
};

} // namespace voxel

#endif // VOXEL_GENERATOR_SCRIPT_H