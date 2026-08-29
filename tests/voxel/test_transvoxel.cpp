#include "test_transvoxel.h"
#include "../../meshers/transvoxel/voxel_mesher_transvoxel.h"
#include "../../util/testing/test_macros.h"

namespace voxel::tests {

void test_transvoxel_issue772() {
	// 当纹理模式为 SINGLE_S4 时，对分量索引值存在错误的断言检查

	VoxelBuffer voxels(VoxelBuffer::ALLOCATOR_DEFAULT);
	voxels.set_channel_depth(VoxelBuffer::CHANNEL_INDICES, VoxelBuffer::DEPTH_8_BIT);
	voxels.create(Vector3iUtil::create(8));
	{
		Vector3i pos;

		const float h = voxels.get_size().y / 2.f + 0.1f;

		for (pos.z = 0; pos.z < voxels.get_size().z; ++pos.z) {
			for (pos.x = 0; pos.x < voxels.get_size().x; ++pos.x) {
				for (pos.y = 0; pos.y < voxels.get_size().y; ++pos.y) {
					const float gy = pos.y;
					const float sd = gy - h;
					voxels.set_voxel_f(sd, pos, VoxelBuffer::CHANNEL_SDF);
					if (sd < 1.f) {
						const uint8_t material_index = (pos.x + pos.y + pos.z) & 0xff;
						voxels.set_voxel(material_index, pos, VoxelBuffer::CHANNEL_INDICES);
					}
				}
			}
		}
	}

	Ref<VoxelMesherTransvoxel> mesher;
	mesher.instantiate();
	mesher->set_texturing_mode(VoxelMesherTransvoxel::TEXTURES_SINGLE_S4);
	VoxelMesher::Output output;
	// 曾经会导致崩溃
	mesher->build(output, VoxelMesher::Input{ voxels, nullptr, Vector3i(), 0, false, false, false });

	VOXEL_TEST_ASSERT(!VoxelMesher::is_mesh_empty(output.surfaces));
}

} // namespace voxel::tests
