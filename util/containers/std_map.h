#ifndef VOXEL_STD_MAP_H
#define VOXEL_STD_MAP_H

#include "../memory/std_allocator.h"
#include <map>

namespace voxel {

// 使用我们自己的默认分配器的便捷别名
template < //
		typename TKey, //
		typename TValue, //
		typename TLess = std::less<TKey>, //
		typename TAllocator = StdDefaultAllocator<std::pair<const TKey, TValue>> //
		>
using StdMap = std::map<TKey, TValue, TLess, TAllocator>;

} // namespace voxel

#endif // VOXEL_STD_MAP_H
