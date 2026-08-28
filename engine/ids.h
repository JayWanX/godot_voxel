#ifndef VOXEL_VOLUME_ID_H
#define VOXEL_VOLUME_ID_H

#include "../util/containers/slot_map.h"

namespace voxel {

typedef SlotMapKey<uint16_t, uint16_t> VolumeID;
typedef SlotMapKey<uint16_t, uint16_t> ViewerID;

} // namespace voxel

namespace voxel {

class TextWriter;

TextWriter &operator<<(TextWriter &w, const SlotMapKey<uint16_t, uint16_t> &v);

} // namespace voxel

#endif // VOXEL_VOLUME_ID_H
