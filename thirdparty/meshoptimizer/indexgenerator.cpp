// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

MESHOPTIMIZER_VOXEL_NAMESPACE_BEGIN

// 此作品基于：
// Matthias Teschner、Bruno Heidelberger、Matthias Mueller、Danat Pomeranets、Markus Gross。用于可变形物体碰撞检测的优化空间哈希。2003
// John McDonald, Mark Kilgard. 使用相邻边法线的无裂缝点法向三角形. 2010
// John Hable. 使用可见性缓冲区渲染的可变速率着色. 2024
namespace meshopt
{

static unsigned int hashUpdate4(unsigned int h, const unsigned char* key, size_t len)
{
	// MurmurHash2
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	while (len >= 4)
	{
		unsigned int k = *reinterpret_cast<const unsigned int*>(key);

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		key += 4;
		len -= 4;
	}

	return h;
}

struct VertexHasher
{
	const unsigned char* vertices;
	size_t vertex_size;
	size_t vertex_stride;

	size_t hash(unsigned int index) const
	{
		return hashUpdate4(0, vertices + index * vertex_stride, vertex_size);
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		return memcmp(vertices + lhs * vertex_stride, vertices + rhs * vertex_stride, vertex_size) == 0;
	}
};

struct VertexStreamHasher
{
	const meshopt_Stream* streams;
	size_t stream_count;

	size_t hash(unsigned int index) const
	{
		unsigned int h = 0;

		for (size_t i = 0; i < stream_count; ++i)
		{
			const meshopt_Stream& s = streams[i];
			const unsigned char* data = static_cast<const unsigned char*>(s.data);

			h = hashUpdate4(h, data + index * s.stride, s.size);
		}

		return h;
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		for (size_t i = 0; i < stream_count; ++i)
		{
			const meshopt_Stream& s = streams[i];
			const unsigned char* data = static_cast<const unsigned char*>(s.data);

			if (memcmp(data + lhs * s.stride, data + rhs * s.stride, s.size) != 0)
				return false;
		}

		return true;
	}
};

struct VertexCustomHasher
{
	const float* vertex_positions;
	size_t vertex_stride_float;

	int (*callback)(void*, unsigned int, unsigned int);
	void* context;

	size_t hash(unsigned int index) const
	{
		const unsigned int* key = reinterpret_cast<const unsigned int*>(vertex_positions + index * vertex_stride_float);

		unsigned int x = key[0], y = key[1], z = key[2];

		// 用零替换负零
		x = (x == 0x80000000) ? 0 : x;
		y = (y == 0x80000000) ? 0 : y;
		z = (z == 0x80000000) ? 0 : z;

		// 打乱位，以确保整数坐标在低位中具有熵
		x ^= x >> 17;
		y ^= y >> 17;
		z ^= z >> 17;

		// 用于可变形物体碰撞检测的优化空间哈希（Spatial Hashing）
		return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		const float* lp = vertex_positions + lhs * vertex_stride_float;
		const float* rp = vertex_positions + rhs * vertex_stride_float;

		if (lp[0] != rp[0] || lp[1] != rp[1] || lp[2] != rp[2])
			return false;

		return callback ? callback(context, lhs, rhs) : true;
	}
};

struct EdgeHasher
{
	const unsigned int* remap;

	size_t hash(unsigned long long edge) const
	{
		unsigned int e0 = unsigned(edge >> 32);
		unsigned int e1 = unsigned(edge);

		unsigned int h1 = remap[e0];
		unsigned int h2 = remap[e1];

		const unsigned int m = 0x5bd1e995;

		// MurmurHash64B 终结器
		h1 ^= h2 >> 18;
		h1 *= m;
		h2 ^= h1 >> 22;
		h2 *= m;
		h1 ^= h2 >> 17;
		h1 *= m;
		h2 ^= h1 >> 19;
		h2 *= m;

		return h2;
	}

	bool equal(unsigned long long lhs, unsigned long long rhs) const
	{
		unsigned int l0 = unsigned(lhs >> 32);
		unsigned int l1 = unsigned(lhs);

		unsigned int r0 = unsigned(rhs >> 32);
		unsigned int r1 = unsigned(rhs);

		return remap[l0] == remap[r0] && remap[l1] == remap[r1];
	}
};

static size_t hashBuckets(size_t count)
{
	size_t buckets = 1;
	while (buckets < count + count / 4)
		buckets *= 2;

	return buckets;
}

template <typename T, typename Hash>
static T* hashLookup(T* table, size_t buckets, const Hash& hash, const T& key, const T& empty)
{
	assert(buckets > 0);
	assert((buckets & (buckets - 1)) == 0);

	size_t hashmod = buckets - 1;
	size_t bucket = hash.hash(key) & hashmod;

	for (size_t probe = 0; probe <= hashmod; ++probe)
	{
		T& item = table[bucket];

		if (item == empty)
			return &item;

		if (hash.equal(item, key))
			return &item;

		// 哈希冲突，使用二次探测（quadratic probing）
		bucket = (bucket + probe + 1) & hashmod;
	}

	assert(false && "Hash table is full"); // unreachable
	return NULL;
}

static void buildPositionRemap(unsigned int* remap, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, meshopt_Allocator& allocator)
{
	VertexHasher vertex_hasher = {reinterpret_cast<const unsigned char*>(vertex_positions), 3 * sizeof(float), vertex_positions_stride};

	size_t vertex_table_size = hashBuckets(vertex_count);
	unsigned int* vertex_table = allocator.allocate<unsigned int>(vertex_table_size);
	memset(vertex_table, -1, vertex_table_size * sizeof(unsigned int));

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int index = unsigned(i);
		unsigned int* entry = hashLookup(vertex_table, vertex_table_size, vertex_hasher, index, ~0u);

		if (*entry == ~0u)
			*entry = index;

		remap[index] = *entry;
	}

	allocator.deallocate(vertex_table);
}

template <typename Hash>
static size_t generateVertexRemap(unsigned int* remap, const unsigned int* indices, size_t index_count, size_t vertex_count, const Hash& hash, meshopt_Allocator& allocator)
{
	memset(remap, -1, vertex_count * sizeof(unsigned int));

	size_t table_size = hashBuckets(vertex_count);
	unsigned int* table = allocator.allocate<unsigned int>(table_size);
	memset(table, -1, table_size * sizeof(unsigned int));

	unsigned int next_vertex = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices ? indices[i] : unsigned(i);
		assert(index < vertex_count);

		if (remap[index] != ~0u)
			continue;

		unsigned int* entry = hashLookup(table, table_size, hash, index, ~0u);

		if (*entry == ~0u)
		{
			*entry = index;
			remap[index] = next_vertex++;
		}
		else
		{
			assert(remap[*entry] != ~0u);
			remap[index] = remap[*entry];
		}
	}

	assert(next_vertex <= vertex_count);
	return next_vertex;
}

template <size_t BlockSize>
static void remapVertices(void* destination, const void* vertices, size_t vertex_count, size_t vertex_size, const unsigned int* remap)
{
	size_t block_size = BlockSize == 0 ? vertex_size : BlockSize;
	assert(block_size == vertex_size);

	for (size_t i = 0; i < vertex_count; ++i)
		if (remap[i] != ~0u)
		{
			assert(remap[i] < vertex_count);
			memcpy(static_cast<unsigned char*>(destination) + remap[i] * block_size, static_cast<const unsigned char*>(vertices) + i * block_size, block_size);
		}
}

template <typename Hash>
static void generateShadowBuffer(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, const Hash& hash, meshopt_Allocator& allocator)
{
	unsigned int* remap = allocator.allocate<unsigned int>(vertex_count);
	memset(remap, -1, vertex_count * sizeof(unsigned int));

	size_t table_size = hashBuckets(vertex_count);
	unsigned int* table = allocator.allocate<unsigned int>(table_size);
	memset(table, -1, table_size * sizeof(unsigned int));

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];
		assert(index < vertex_count);

		if (remap[index] == ~0u)
		{
			unsigned int* entry = hashLookup(table, table_size, hash, index, ~0u);

			if (*entry == ~0u)
				*entry = index;

			remap[index] = *entry;
		}

		destination[i] = remap[index];
	}
}

} // namespace meshopt

size_t meshopt_generateVertexRemap(unsigned int* destination, const unsigned int* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	using namespace meshopt;

	assert(indices || index_count == vertex_count);
	assert(!indices || index_count % 3 == 0);
	assert(vertex_size > 0 && vertex_size <= 256);

	meshopt_Allocator allocator;
	VertexHasher hasher = {static_cast<const unsigned char*>(vertices), vertex_size, vertex_size};

	return generateVertexRemap(destination, indices, index_count, vertex_count, hasher, allocator);
}

size_t meshopt_generateVertexRemapMulti(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, const struct meshopt_Stream* streams, size_t stream_count)
{
	using namespace meshopt;

	assert(indices || index_count == vertex_count);
	assert(index_count % 3 == 0);
	assert(stream_count > 0 && stream_count <= 16);

	for (size_t i = 0; i < stream_count; ++i)
	{
		assert(streams[i].size > 0 && streams[i].size <= 256);
		assert(streams[i].size <= streams[i].stride);
	}

	meshopt_Allocator allocator;
	VertexStreamHasher hasher = {streams, stream_count};

	return generateVertexRemap(destination, indices, index_count, vertex_count, hasher, allocator);
}

size_t meshopt_generateVertexRemapCustom(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, int (*callback)(void*, unsigned int, unsigned int), void* context)
{
	using namespace meshopt;

	assert(indices || index_count == vertex_count);
	assert(!indices || index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

	meshopt_Allocator allocator;
	VertexCustomHasher hasher = {vertex_positions, vertex_positions_stride / sizeof(float), callback, context};

	return generateVertexRemap(destination, indices, index_count, vertex_count, hasher, allocator);
}

void meshopt_remapVertexBuffer(void* destination, const void* vertices, size_t vertex_count, size_t vertex_size, const unsigned int* remap)
{
	using namespace meshopt;

	assert(vertex_size > 0 && vertex_size <= 256);

	meshopt_Allocator allocator;

	// 支持原地重映射
	if (destination == vertices)
	{
		unsigned char* vertices_copy = allocator.allocate<unsigned char>(vertex_count * vertex_size);
		memcpy(vertices_copy, vertices, vertex_count * vertex_size);
		vertices = vertices_copy;
	}

	// 针对常见顶点大小特化循环，确保 memcpy 被编译为内联内置函数
	switch (vertex_size)
	{
	case 4:
		return remapVertices<4>(destination, vertices, vertex_count, vertex_size, remap);

	case 8:
		return remapVertices<8>(destination, vertices, vertex_count, vertex_size, remap);

	case 12:
		return remapVertices<12>(destination, vertices, vertex_count, vertex_size, remap);

	case 16:
		return remapVertices<16>(destination, vertices, vertex_count, vertex_size, remap);

	default:
		return remapVertices<0>(destination, vertices, vertex_count, vertex_size, remap);
	}
}

void meshopt_remapIndexBuffer(unsigned int* destination, const unsigned int* indices, size_t index_count, const unsigned int* remap)
{
	assert(index_count % 3 == 0);

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices ? indices[i] : unsigned(i);
		assert(remap[index] != ~0u);

		destination[i] = remap[index];
	}
}

void meshopt_generateShadowIndexBuffer(unsigned int* destination, const unsigned int* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size, size_t vertex_stride)
{
	using namespace meshopt;

	assert(indices);
	assert(index_count % 3 == 0);
	assert(vertex_size > 0 && vertex_size <= 256);
	assert(vertex_size <= vertex_stride);

	meshopt_Allocator allocator;
	VertexHasher hasher = {static_cast<const unsigned char*>(vertices), vertex_size, vertex_stride};

	generateShadowBuffer(destination, indices, index_count, vertex_count, hasher, allocator);
}

void meshopt_generateShadowIndexBufferMulti(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, const struct meshopt_Stream* streams, size_t stream_count)
{
	using namespace meshopt;

	assert(indices);
	assert(index_count % 3 == 0);
	assert(stream_count > 0 && stream_count <= 16);

	for (size_t i = 0; i < stream_count; ++i)
	{
		assert(streams[i].size > 0 && streams[i].size <= 256);
		assert(streams[i].size <= streams[i].stride);
	}

	meshopt_Allocator allocator;
	VertexStreamHasher hasher = {streams, stream_count};

	generateShadowBuffer(destination, indices, index_count, vertex_count, hasher, allocator);
}

void meshopt_generatePositionRemap(unsigned int* destination, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	using namespace meshopt;

	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

	meshopt_Allocator allocator;
	VertexCustomHasher hasher = {vertex_positions, vertex_positions_stride / sizeof(float), NULL, NULL};

	size_t table_size = hashBuckets(vertex_count);
	unsigned int* table = allocator.allocate<unsigned int>(table_size);
	memset(table, -1, table_size * sizeof(unsigned int));

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int* entry = hashLookup(table, table_size, hasher, unsigned(i), ~0u);

		if (*entry == ~0u)
			*entry = unsigned(i);

		destination[i] = *entry;
	}
}

void meshopt_generateAdjacencyIndexBuffer(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

	meshopt_Allocator allocator;

	static const int next[4] = {1, 2, 0, 1};

	// 构建位置重映射：对于每个顶点，它映射到哪个其它（规范的）顶点？
	unsigned int* remap = allocator.allocate<unsigned int>(vertex_count);
	buildPositionRemap(remap, vertex_positions, vertex_count, vertex_positions_stride, allocator);

	// build edge set; this stores all triangle edges but we can look these up by any other wedge
	EdgeHasher edge_hasher = {remap};

	size_t edge_table_size = hashBuckets(index_count);
	unsigned long long* edge_table = allocator.allocate<unsigned long long>(edge_table_size);
	unsigned int* edge_vertex_table = allocator.allocate<unsigned int>(edge_table_size);

	memset(edge_table, -1, edge_table_size * sizeof(unsigned long long));
	memset(edge_vertex_table, -1, edge_table_size * sizeof(unsigned int));

	for (size_t i = 0; i < index_count; i += 3)
	{
		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];
			unsigned int i2 = indices[i + next[e + 1]];
			assert(i0 < vertex_count && i1 < vertex_count && i2 < vertex_count);

			unsigned long long edge = ((unsigned long long)i0 << 32) | i1;
			unsigned long long* entry = hashLookup(edge_table, edge_table_size, edge_hasher, edge, ~0ull);

			if (*entry == ~0ull)
			{
				*entry = edge;

				// 存储与边相对的顶点
				edge_vertex_table[entry - edge_table] = i2;
			}
		}
	}

	// 构建结果索引缓冲区：每个输入三角形 6 个索引
	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int patch[6];

		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];
			assert(i0 < vertex_count && i1 < vertex_count);

			// 注意：这指的是对边！
			unsigned long long edge = ((unsigned long long)i1 << 32) | i0;
			unsigned long long* oppe = hashLookup(edge_table, edge_table_size, edge_hasher, edge, ~0ull);

			patch[e * 2 + 0] = i0;
			patch[e * 2 + 1] = (*oppe == ~0ull) ? i0 : edge_vertex_table[oppe - edge_table];
		}

		memcpy(destination + i * 2, patch, sizeof(patch));
	}
}

void meshopt_generateTessellationIndexBuffer(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

	meshopt_Allocator allocator;

	static const int next[3] = {1, 2, 0};

	// 构建位置重映射：对于每个顶点，它映射到哪个其它（规范的）顶点？
	unsigned int* remap = allocator.allocate<unsigned int>(vertex_count);
	buildPositionRemap(remap, vertex_positions, vertex_count, vertex_positions_stride, allocator);

	// build edge set; this stores all triangle edges but we can look these up by any other wedge
	EdgeHasher edge_hasher = {remap};

	size_t edge_table_size = hashBuckets(index_count);
	unsigned long long* edge_table = allocator.allocate<unsigned long long>(edge_table_size);
	memset(edge_table, -1, edge_table_size * sizeof(unsigned long long));

	for (size_t i = 0; i < index_count; i += 3)
	{
		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];
			assert(i0 < vertex_count && i1 < vertex_count);

			unsigned long long edge = ((unsigned long long)i0 << 32) | i1;
			unsigned long long* entry = hashLookup(edge_table, edge_table_size, edge_hasher, edge, ~0ull);

			if (*entry == ~0ull)
				*entry = edge;
		}
	}

	// 构建结果索引缓冲区：每个输入三角形 12 个索引
	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int patch[12];

		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];
			assert(i0 < vertex_count && i1 < vertex_count);

			// 注意：这指的是对边！
			unsigned long long edge = ((unsigned long long)i1 << 32) | i0;
			unsigned long long oppe = *hashLookup(edge_table, edge_table_size, edge_hasher, edge, ~0ull);

			// use the same edge if opposite edge doesn't exist (border)
			oppe = (oppe == ~0ull) ? edge : oppe;

			// triangle index (0, 1, 2)
			patch[e] = i0;

			// opposite edge (3, 4; 5, 6; 7, 8)
			patch[3 + e * 2 + 0] = unsigned(oppe);
			patch[3 + e * 2 + 1] = unsigned(oppe >> 32);

			// dominant vertex (9, 10, 11)
			patch[9 + e] = remap[i0];
		}

		memcpy(destination + i * 4, patch, sizeof(patch));
	}
}

size_t meshopt_generateProvokingIndexBuffer(unsigned int* destination, unsigned int* reorder, const unsigned int* indices, size_t index_count, size_t vertex_count)
{
	assert(index_count % 3 == 0);

	meshopt_Allocator allocator;

	unsigned int* remap = allocator.allocate<unsigned int>(vertex_count);
	memset(remap, -1, vertex_count * sizeof(unsigned int));

	// compute vertex valence; this is used to prioritize least used corner
	// note: we use 8-bit counters for performance; for outlier vertices the valence is incorrect but that just affects the heuristic
	unsigned char* valence = allocator.allocate<unsigned char>(vertex_count);
	memset(valence, 0, vertex_count);

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];
		assert(index < vertex_count);

		valence[index]++;
	}

	unsigned int reorder_offset = 0;

	// assign provoking vertices; leave the rest for the next pass
	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int a = indices[i + 0], b = indices[i + 1], c = indices[i + 2];
		assert(a < vertex_count && b < vertex_count && c < vertex_count);

		// 尝试旋转三角形，使触发顶点此前未被见过
		// if multiple vertices are new, prioritize the one with least valence
		// 这降低未来某个三角形三个顶点都已见过的风险
		unsigned int va = remap[a] == ~0u ? valence[a] : ~0u;
		unsigned int vb = remap[b] == ~0u ? valence[b] : ~0u;
		unsigned int vc = remap[c] == ~0u ? valence[c] : ~0u;

		if (vb != ~0u && vb <= va && vb <= vc)
		{
			// abc -> bca
			unsigned int t = a;
			a = b, b = c, c = t;
		}
		else if (vc != ~0u && vc <= va && vc <= vb)
		{
			// abc -> cab
			unsigned int t = c;
			c = b, b = a, a = t;
		}

		unsigned int newidx = reorder_offset;

		// 此时 remap[a] = ~0u，或三个顶点都是旧的
		// 记录 remap[a] 使未来对同一索引的引用可被重映射，从而节省空间
		if (remap[a] == ~0u)
			remap[a] = newidx;

		// 需要克隆触发顶点以获得唯一索引
		// if all three are used the choice is arbitrary since no future triangle will be able to reuse any of these
		reorder[reorder_offset++] = a;

		// 注意：第一个顶点是最终的，另外两个将在下一趟中修正
		destination[i + 0] = newidx;
		destination[i + 1] = b;
		destination[i + 2] = c;

		// 为角点启发式更新顶点度数
		valence[a]--;
		valence[b]--;
		valence[c]--;
	}

	// remap or clone non-provoking vertices (iterating to skip provoking vertices)
	int step = 1;

	for (size_t i = 1; i < index_count; i += step, step ^= 3)
	{
		unsigned int index = destination[i];

		if (remap[index] == ~0u)
		{
			// 此前未将该顶点作为触发顶点见过
			// 为保持对原始顶点的引用，需要克隆它
			unsigned int newidx = reorder_offset;

			remap[index] = newidx;
			reorder[reorder_offset++] = index;
		}

		destination[i] = remap[index];
	}

	assert(reorder_offset <= vertex_count + index_count / 3);
	return reorder_offset;
}

MESHOPTIMIZER_VOXEL_NAMESPACE_END