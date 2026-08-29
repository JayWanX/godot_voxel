#ifndef VOXEL_HASH_FUNCS_H
#define VOXEL_HASH_FUNCS_H

#include "math/funcs.h"
#include <cstdint>

namespace voxel {

// 从 Godot 核心代码复制而来。
// TODO Godot 核心已提供类似函数，考虑替换或保留自定义实现

inline uint32_t hash_djb2_one_32(uint32_t p_in, uint32_t p_prev = 5381) {
	return ((p_prev << 5) + p_prev) ^ p_in;
}

inline uint64_t hash_djb2_one_64(uint64_t p_in, uint64_t p_prev = 5381) {
	return ((p_prev << 5) + p_prev) ^ p_in;
}

#define HASH_MURMUR3_SEED 0x7F07C65
// Murmurhash3 的 32 位版本。
// 所有 MurmurHash 版本均为公有领域软件，作者放弃其代码的所有版权。

inline uint32_t hash_murmur3_one_32(uint32_t p_in, uint32_t p_seed = HASH_MURMUR3_SEED) {
	p_in *= 0xcc9e2d51;
	p_in = (p_in << 15) | (p_in >> 17);
	p_in *= 0x1b873593;

	p_seed ^= p_in;
	p_seed = (p_seed << 13) | (p_seed >> 19);
	p_seed = p_seed * 5 + 0xe6546b64;

	return p_seed;
}

inline uint32_t hash_fmix32(uint32_t h) {
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

} // namespace voxel

#endif // VOXEL_HASH_FUNCS_H
