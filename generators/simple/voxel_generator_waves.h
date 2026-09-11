#ifndef VOXEL_GENERATOR_WAVES_H
#define VOXEL_GENERATOR_WAVES_H

#include <core/math/vector2.h>
#include "../../util/thread/rw_lock.h"
#include "voxel_generator_heightmap.h"

namespace voxel {

class VoxelGeneratorWaves : public VoxelGeneratorHeightmap {
	GDCLASS(VoxelGeneratorWaves, VoxelGeneratorHeightmap)

public:
	VoxelGeneratorWaves();
	~VoxelGeneratorWaves();

	// 生成单个数据块的体素数据
	Result generate_block(VoxelGenerator::VoxelQueryData input) override;

	// 波浪图案的大小
	Vector2 get_pattern_size() const;
	void set_pattern_size(Vector2 size);

	// 波浪图案的偏移
	Vector2 get_pattern_offset() const;
	void set_pattern_offset(Vector2 offset);

private:
	static void _bind_methods();

	struct Parameters {
		Vector2 pattern_size;
		Vector2 pattern_offset;
	};

	Parameters _parameters;
	RWLock _parameters_lock;
};

} // namespace voxel

#endif // VOXEL_GENERATOR_WAVES_H