#ifndef VOXEL_INSTANCER_QUICK_RELOADING_CACHE_H
#define VOXEL_INSTANCER_QUICK_RELOADING_CACHE_H

#include "../../util/containers/std_unordered_map.h"
#include <core/math/vector3i.h>
#include "../../util/memory/memory.h"
#include "../../util/thread/mutex.h"

namespace voxel {

struct InstanceBlockData;

// 临时存储刚刚卸载且即将异步保存的数据块。
// 如果在保存完成甚至开始之前需要再次加载这些数据块，则会改从该缓存中获取。
// 没有它，数据块可能在保存前就被重新加载，从而导致数据丢失。
// 听起来可能令人困惑，但因为保存和加载是多线程的，这种情况确实可能发生。
struct InstancerQuickReloadingCache {
	StdUnorderedMap<Vector3i, UniquePtr<InstanceBlockData>> map;
	Mutex mutex;
};

} // namespace voxel

#endif // VOXEL_INSTANCER_QUICK_RELOADING_CACHE_H
