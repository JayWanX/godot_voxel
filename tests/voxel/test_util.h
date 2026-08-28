#ifndef VOXEL_TEST_UTIL_H
#define VOXEL_TEST_UTIL_H

namespace voxel {

class VoxelBuffer;

namespace tests {

bool sd_equals_approx(const VoxelBuffer &vb1, const VoxelBuffer &vb2);
void print_channel_as_ascii(const VoxelBuffer &vb, unsigned int channel, const unsigned int padding);

} // namespace tests
} // namespace voxel

#endif // VOXEL_TEST_UTIL_H
