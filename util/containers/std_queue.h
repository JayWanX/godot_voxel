#ifndef VOXEL_STD_QUEUE_H
#define VOXEL_STD_QUEUE_H

#include "../memory/std_allocator.h"
#include <deque>
#include <queue>

namespace voxel {

// 使用我们自己的默认分配器的便捷别名
template <typename TValue, typename TAllocator = StdDefaultAllocator<TValue>>
using StdQueue = std::queue<TValue, std::deque<TValue, TAllocator>>;

} // namespace voxel

#endif // VOXEL_STD_QUEUE_H
