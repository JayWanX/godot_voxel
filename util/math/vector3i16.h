#ifndef VOXEL_VECTOR3I16_H
#define VOXEL_VECTOR3I16_H

#include "vector3t.h"
#include <cstdint>
#include <functional>

namespace voxel {

typedef Vector3T<int16_t> Vector3i16;

inline size_t get_hash_st(const voxel::Vector3i16 &v) {
	// TODO Optimization: benchmark this hash, I just wanted one that works
	// static_assert(sizeof(voxel::Vector3i16) <= sizeof(uint64_t));
	const uint64_t m = v.x | (static_cast<uint64_t>(v.y) << 16) | (static_cast<uint64_t>(v.z) << 32);
	return std::hash<uint64_t>{}(m);
}

} // namespace voxel

// For STL
namespace std {
template <>
struct hash<voxel::Vector3i16> {
	size_t operator()(const voxel::Vector3i16 &v) const {
		return voxel::get_hash_st(v);
	}
};
} // namespace std

#endif // VOXEL_VECTOR3I16_H
