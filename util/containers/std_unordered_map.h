#ifndef VOXEL_STD_UNORDERED_MAP_H
#define VOXEL_STD_UNORDERED_MAP_H

#include "../memory/std_allocator.h"
#include <unordered_map>

namespace voxel {

// 使用我们自己的默认分配器的便捷别名
template < //
		typename TKey, //
		typename TValue, //
		typename THasher = std::hash<TKey>, //
		typename TEquator = std::equal_to<TKey>, //
		typename TAllocator = StdDefaultAllocator<std::pair<const TKey, TValue>> //
		>
using StdUnorderedMap = std::unordered_map<TKey, TValue, THasher, TEquator, TAllocator>;

} // namespace voxel

#endif // VOXEL_STD_UNORDERED_MAP_H
