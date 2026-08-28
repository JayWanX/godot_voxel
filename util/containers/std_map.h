#ifndef VOXEL_STD_MAP_H
#define VOXEL_STD_MAP_H

#include "../memory/std_allocator.h"
#include <map>

namespace voxel {

// Convenience alias that uses our own default allocator
template < //
		typename TKey, //
		typename TValue, //
		typename TLess = std::less<TKey>, //
		typename TAllocator = StdDefaultAllocator<std::pair<const TKey, TValue>> //
		>
using StdMap = std::map<TKey, TValue, TLess, TAllocator>;

} // namespace voxel

#endif // VOXEL_STD_MAP_H
