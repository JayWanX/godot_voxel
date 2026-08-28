#ifndef VOXEL_STD_UNORDERED_SET_H
#define VOXEL_STD_UNORDERED_SET_H

#include "../memory/std_allocator.h"
#include <unordered_set>

namespace voxel {

// Convenience alias that uses our own default allocator
template < //
		typename TValue, //
		typename THasher = std::hash<TValue>, //
		typename TEquator = std::equal_to<TValue>, //
		typename TAllocator = StdDefaultAllocator<TValue> //
		>
using StdUnorderedSet = std::unordered_set<TValue, THasher, TEquator, TAllocator>;

} // namespace voxel

#endif // VOXEL_STD_UNORDERED_SET_H
