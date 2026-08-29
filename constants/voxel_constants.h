#ifndef VOXEL_CONSTANTS_H
#define VOXEL_CONSTANTS_H

#include "../util/math/constants.h"
#include <cstdint>

namespace voxel::constants {

// 选择这些常量是为了避免意外消耗过多的资源
static const float MINIMUM_LOD_DISTANCE = 16.f;
static const float MAXIMUM_LOD_DISTANCE = 128.f;

static const unsigned int MIN_BLOCK_SIZE = 16;
static const unsigned int MAX_BLOCK_SIZE = 32;

static const unsigned int MAX_BLOCK_COUNT_PER_REQUEST = 4 * 4 * 4;

// 24 应该基本够用。
// 若块大小为 32 个体素，且 1 个体素为 1 米，
// 那么最大的块将跨越 268,435.456 公里，大约是地球直径的 20 倍。
// 使用更高的最大值会在计算尺寸时导致 int32 溢出。目前没有这样的使用场景。
static const unsigned int MAX_LOD = 24;

static const int MAX_VOLUME_EXTENT = 0x1fffffff;
static const int MAX_VOLUME_SIZE = 2 * MAX_VOLUME_EXTENT; // 1,073,741,822 个体素

static const float INV_0x7f = 1.f / 0x7f;
static const float INV_0x7fff = 1.f / 0x7fff;

// 在 32 位以下，通道被归一化到 -1..1 范围内，只能表示有限数量的值。
// 对于存储 SDF，我们需要超出该范围的值域，尤其是为了更好的 LOD。
// 因此我们可以对其进行缩放以更好地匹配分辨率。这些缩放比例是随意选择的，但应该能很好地适用于
// 相应的精度。
static const float QUANTIZED_SDF_8_BITS_SCALE = 0.1f;
static const float QUANTIZED_SDF_8_BITS_SCALE_INV = 1.f / 0.1f;
// static const float QUANTIZED_SDF_8_BITS_MIN = -10.f;
// static const float QUANTIZED_SDF_8_BITS_MAX = 10.f;
// static const float QUANTIZED_SDF_8_BITS_DECODE_SCALE = QUANTIZED_SDF_8_BITS_SCALE / 127.f;
// static const float QUANTIZED_SDF_8_BITS_ENCODE_SCALE = 127.f / QUANTIZED_SDF_8_BITS_SCALE;

static const float QUANTIZED_SDF_16_BITS_SCALE = 0.002f;
static const float QUANTIZED_SDF_16_BITS_SCALE_INV = 1.f / 0.002f;
// static const float QUANTIZED_SDF_16_BITS_MIN = -500.f;
// static const float QUANTIZED_SDF_16_BITS_MAX = 500.f;
// static const float QUANTIZED_SDF_16_BITS_DECODE_SCALE = QUANTIZED_SDF_16_BITS_SCALE / 32767.f;
// static const float QUANTIZED_SDF_16_BITS_ENCODE_SCALE = 32767.f / QUANTIZED_SDF_16_BITS_SCALE;

// 有时会采用捷径将体素填充为实体或空气。这并非一致的 SDF，但在那些
// 场景中可能没问题。声明它便于在代码中追踪此类用法，以便日后找到更好的
// 解决方案。
static const float SDF_FAR_OUTSIDE = 100.f;
static const float SDF_FAR_INSIDE = -100.f;

static const unsigned int DEFAULT_BLOCK_SIZE_PO2 = 4;

static const float DEFAULT_COLLISION_MARGIN = 0.04f;

// 默认情况下，任务首先按 band2 的值排序。
// 相同时按 band1 排序，band1 通常取决于 LOD。
// 再相同时按 band0 排序，band0 取决于与观察者的距离（在相关时）。
// band3 优先级高于 band2，但目前使用不多。
static const uint8_t TASK_PRIORITY_MESH_BAND2 = 10;
static const uint8_t TASK_PRIORITY_GENERATE_BAND2 = 10;
static const uint8_t TASK_PRIORITY_LOAD_BAND2 = 10;
static const uint8_t TASK_PRIORITY_SAVE_BAND2 = 9;
static const uint8_t TASK_PRIORITY_DETAIL_TEXTURES_BAND2 = 8; // 在网格之后

static const uint8_t TASK_PRIORITY_BAND3_DEFAULT = 10;

} // namespace voxel::constants

#endif // VOXEL_CONSTANTS_H
