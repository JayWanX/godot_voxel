// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>

#ifndef TRACE
#define TRACE 0
#endif

#if TRACE
#include <stdio.h>
#endif

#if TRACE
#define TRACESTATS(i) stats[i]++;
#else
#define TRACESTATS(i) (void)0
#endif

MESHOPTIMIZER_VOXEL_NAMESPACE_BEGIN

// 此作品基于：
// Michael Garland 和 Paul S. Heckbert. 使用二次误差度量简化曲面. 1997
// Michael Garland. 基于二次的多边形曲面简化. 1999
// Peter Lindstrom. 大型多边形模型的外存（Out-of-Core）简化. 2000
// Matthias Teschner、Bruno Heidelberger、Matthias Mueller、Danat Pomeranets、Markus Gross。用于可变形物体碰撞检测的优化空间哈希。2003
// Peter Van Sandt, Yannis Chronis, Jignesh M. Patel. 高效搜索内存中的有序数组：插值搜索的反击？2019
// Hugues Hoppe. 带外观属性的网格简化的新二次度量. 1999
// Hugues Hoppe, Steve Marschner. 带外观属性的网格简化的新二次度量的高效最小化. 2000
namespace meshopt
{

struct EdgeAdjacency
{
	struct Edge
	{
		unsigned int next;
		unsigned int prev;
	};

	unsigned int* offsets;
	Edge* data;
};

static void prepareEdgeAdjacency(EdgeAdjacency& adjacency, size_t index_count, size_t vertex_count, meshopt_Allocator& allocator)
{
	adjacency.offsets = allocator.allocate<unsigned int>(vertex_count + 1);
	adjacency.data = allocator.allocate<EdgeAdjacency::Edge>(index_count);
}

static void updateEdgeAdjacency(EdgeAdjacency& adjacency, const unsigned int* indices, size_t index_count, size_t vertex_count, const unsigned int* remap)
{
	size_t face_count = index_count / 3;
	unsigned int* offsets = adjacency.offsets + 1;
	EdgeAdjacency::Edge* data = adjacency.data;

	// 填充边计数
	memset(offsets, 0, vertex_count * sizeof(unsigned int));

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int v = remap ? remap[indices[i]] : indices[i];
		assert(v < vertex_count);

		offsets[v]++;
	}

	// 填充偏移表
	unsigned int offset = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int count = offsets[i];
		offsets[i] = offset;
		offset += count;
	}

	assert(offset == index_count);

	// 填充边数据
	for (size_t i = 0; i < face_count; ++i)
	{
		unsigned int a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];

		if (remap)
		{
			a = remap[a];
			b = remap[b];
			c = remap[c];
		}

		data[offsets[a]].next = b;
		data[offsets[a]].prev = c;
		offsets[a]++;

		data[offsets[b]].next = c;
		data[offsets[b]].prev = a;
		offsets[b]++;

		data[offsets[c]].next = a;
		data[offsets[c]].prev = b;
		offsets[c]++;
	}

	// 确定最终偏移量
	adjacency.offsets[0] = 0;
	assert(adjacency.offsets[vertex_count] == index_count);
}

struct PositionHasher
{
	const float* vertex_positions;
	size_t vertex_stride_float;
	const unsigned int* sparse_remap;

	size_t hash(unsigned int index) const
	{
		unsigned int ri = sparse_remap ? sparse_remap[index] : index;
		const unsigned int* key = reinterpret_cast<const unsigned int*>(vertex_positions + ri * vertex_stride_float);

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
		unsigned int li = sparse_remap ? sparse_remap[lhs] : lhs;
		unsigned int ri = sparse_remap ? sparse_remap[rhs] : rhs;

		const float* lv = vertex_positions + li * vertex_stride_float;
		const float* rv = vertex_positions + ri * vertex_stride_float;

		return lv[0] == rv[0] && lv[1] == rv[1] && lv[2] == rv[2];
	}
};

struct RemapHasher
{
	unsigned int* remap;

	size_t hash(unsigned int id) const
	{
		return id * 0x5bd1e995;
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		return remap[lhs] == rhs;
	}
};

static size_t hashBuckets2(size_t count)
{
	size_t buckets = 1;
	while (buckets < count + count / 4)
		buckets *= 2;

	return buckets;
}

template <typename T, typename Hash>
static T* hashLookup2(T* table, size_t buckets, const Hash& hash, const T& key, const T& empty)
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

static void buildPositionRemap(unsigned int* remap, unsigned int* wedge, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, const unsigned int* sparse_remap, meshopt_Allocator& allocator)
{
	PositionHasher hasher = {vertex_positions_data, vertex_positions_stride / sizeof(float), sparse_remap};

	size_t table_size = hashBuckets2(vertex_count);
	unsigned int* table = allocator.allocate<unsigned int>(table_size);
	memset(table, -1, table_size * sizeof(unsigned int));

	// 构建前向重映射：每个顶点映射到哪个其他（规范）顶点？
	// 我们为此使用位置等价，并将顶点重映射到其他已存在的顶点
	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int index = unsigned(i);
		unsigned int* entry = hashLookup2(table, table_size, hasher, index, ~0u);

		if (*entry == ~0u)
			*entry = index;

		remap[index] = *entry;
	}

	allocator.deallocate(table);

	if (!wedge)
		return;

	// 构建楔（wedge）表：对每个顶点，哪个其他顶点是同时映射到同一顶点的下一个楔？
	// entries in table form a (cyclic) wedge loop per vertex; for manifold vertices, wedge[i] == remap[i] == i
	for (size_t i = 0; i < vertex_count; ++i)
		wedge[i] = unsigned(i);

	for (size_t i = 0; i < vertex_count; ++i)
		if (remap[i] != i)
		{
			unsigned int r = remap[i];

			wedge[i] = wedge[r];
			wedge[r] = unsigned(i);
		}
}

static unsigned int* buildSparseRemap(unsigned int* indices, size_t index_count, size_t vertex_count, size_t* out_vertex_count, meshopt_Allocator& allocator)
{
	// 使用位集计算唯一顶点的精确数量
	unsigned char* filter = allocator.allocate<unsigned char>((vertex_count + 7) / 8);
	memset(filter, 0, (vertex_count + 7) / 8);

	size_t unique = 0;
	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];
		assert(index < vertex_count);

		unique += (filter[index / 8] & (1 << (index % 8))) == 0;
		filter[index / 8] |= 1 << (index % 8);
	}

	unsigned int* remap = allocator.allocate<unsigned int>(unique);
	size_t offset = 0;

	// temporary map dense => sparse; we allocate it last so that we can deallocate it
	size_t revremap_size = hashBuckets2(unique);
	unsigned int* revremap = allocator.allocate<unsigned int>(revremap_size);
	memset(revremap, -1, revremap_size * sizeof(unsigned int));

	// 以 revremap 为辅助填充 remap，并在同一趟中重写索引
	RemapHasher hasher = {remap};

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];

		unsigned int* entry = hashLookup2(revremap, revremap_size, hasher, index, ~0u);

		if (*entry == ~0u)
		{
			remap[offset] = index;
			*entry = unsigned(offset);
			offset++;
		}

		indices[i] = *entry;
	}

	allocator.deallocate(revremap);

	assert(offset == unique);
	*out_vertex_count = unique;

	return remap;
}

enum VertexKind
{
	Kind_Manifold, // 不在属性接缝上，也不在任意边界上
	Kind_Border,   // 不在属性接缝上，恰好有两条开边
	Kind_Seam,     // 在恰好有两条属性接缝边的属性接缝上
	Kind_Complex,  // none of the above; these vertices can move as long as all wedges move to the target vertex
	Kind_Locked,   // none of the above; these vertices can't move

	Kind_Count
};

// 流形顶点可塌缩到任意顶点
// 边界/接缝顶点可分别塌缩到边界/接缝顶点，或被锁定
// 复杂顶点可塌缩到复杂/锁定顶点
// 经验法则是：将类型 A 塌缩进类型 B 会在目标顶点中保留类型 B
// 例如，虽然我们可以将 Complex 塌缩进 Manifold，但这意味着目标顶点不再是无歧义流形
const unsigned char kCanCollapse[Kind_Count][Kind_Count] = {
    {1, 1, 1, 1, 1},
    {0, 1, 0, 0, 1},
    {0, 0, 1, 0, 1},
    {0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0},
};

// if a vertex is manifold or seam, adjoining edges are guaranteed to have an opposite edge
// 注意：对接缝边而言，基于属性的拓扑中不存在对边
// 但若考虑仅位置网格变体，则存在对边
// 虽然许多复杂塌缩都有对边，但由于复杂顶点会塌缩到
// 同一楔，通过考虑两个目标来保持对边分开可提升质量
const unsigned char kHasOpposite[Kind_Count][Kind_Count] = {
    {1, 1, 1, 1, 1},
    {1, 0, 1, 0, 0},
    {1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0},
    {1, 0, 1, 0, 0},
};

static bool hasEdge(const EdgeAdjacency& adjacency, unsigned int a, unsigned int b)
{
	unsigned int count = adjacency.offsets[a + 1] - adjacency.offsets[a];
	const EdgeAdjacency::Edge* edges = adjacency.data + adjacency.offsets[a];

	for (size_t i = 0; i < count; ++i)
		if (edges[i].next == b)
			return true;

	return false;
}

static bool hasEdge(const EdgeAdjacency& adjacency, unsigned int a, unsigned int b, const unsigned int* remap, const unsigned int* wedge)
{
	unsigned int v = a;

	do
	{
		unsigned int count = adjacency.offsets[v + 1] - adjacency.offsets[v];
		const EdgeAdjacency::Edge* edges = adjacency.data + adjacency.offsets[v];

		for (size_t i = 0; i < count; ++i)
			if (remap[edges[i].next] == remap[b])
				return true;

		v = wedge[v];
	} while (v != a);

	return false;
}

static void classifyVertices(unsigned char* result, unsigned int* loop, unsigned int* loopback, size_t vertex_count, const EdgeAdjacency& adjacency, const unsigned int* remap, const unsigned int* wedge, const unsigned char* vertex_lock, const unsigned int* sparse_remap, unsigned int options)
{
	memset(loop, -1, vertex_count * sizeof(unsigned int));
	memset(loopback, -1, vertex_count * sizeof(unsigned int));

	// 出入开边：若无开边则为 ~0u，若有多个则为 i
	// note that this is the same data as required in loop[] arrays; loop[] data is only valid for border/seam
	// 但此处也可为其他类型的顶点填充数据
	unsigned int* openinc = loopback;
	unsigned int* openout = loop;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int vertex = unsigned(i);

		unsigned int count = adjacency.offsets[vertex + 1] - adjacency.offsets[vertex];
		const EdgeAdjacency::Edge* edges = adjacency.data + adjacency.offsets[vertex];

		for (size_t j = 0; j < count; ++j)
		{
			unsigned int target = edges[j].next;

			if (target == vertex)
			{
				// 退化三角形有两条不同的边（而非三条），以及自边
				// is bi-directional by definition; this can break border/seam classification by "closing"
				// 来自另一三角形的开边会被错误地标记为流形顶点
				// 我们改为将该顶点标记为具有 >1 条开边，使其变为锁定/复杂
				openinc[vertex] = openout[vertex] = vertex;
			}
			else if (!hasEdge(adjacency, target, vertex))
			{
				openinc[target] = (openinc[target] == ~0u) ? vertex : target;
				openout[vertex] = (openout[vertex] == ~0u) ? target : vertex;
			}
		}
	}

#if TRACE
	size_t stats[4] = {};
#endif

	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (remap[i] == i)
		{
			if (wedge[i] == i)
			{
				// 无属性接缝，需检查是否为流形
				unsigned int openi = openinc[i], openo = openout[i];

				// 注意：我们将任何无开边的顶点分类为流形
				// 这在技术上不正确 - 若 4 个三角形共享一条边，我们会把顶点分类为流形
				// 这在实践中是否为问题尚不清楚
				if (openi == ~0u && openo == ~0u)
				{
					result[i] = Kind_Manifold;
				}
				else if (openi != ~0u && openo != ~0u && remap[openi] == remap[openo] && openi != i)
				{
					// classify half-seams as seams (the branch below would mis-classify them as borders)
					// 半接缝是同时连接到潜在接缝两个顶点的一个顶点
					// 将这些视为接缝，可将"完整"接缝顶点塌缩到它们上面
					result[i] = Kind_Seam;
				}
				else if (openi != i && openo != i)
				{
					result[i] = Kind_Border;
				}
				else
				{
					result[i] = Kind_Locked;
					TRACESTATS(0);
				}
			}
			else if (wedge[wedge[i]] == i)
			{
				// attribute seam; need to distinguish between Seam and Locked
				unsigned int w = wedge[i];
				unsigned int openiv = openinc[i], openov = openout[i];
				unsigned int openiw = openinc[w], openow = openout[w];

				// 接缝的每个顶点应有一条开半边，且这些边需要"连接" - 重映射后指向同一顶点
				if (openiv != ~0u && openiv != i && openov != ~0u && openov != i &&
				    openiw != ~0u && openiw != w && openow != ~0u && openow != w)
				{
					if (remap[openiv] == remap[openow] && remap[openov] == remap[openiw] && remap[openiv] != remap[openov])
					{
						result[i] = Kind_Seam;
					}
					else
					{
						result[i] = Kind_Locked;
						TRACESTATS(1);
					}
				}
				else
				{
					result[i] = Kind_Locked;
					TRACESTATS(2);
				}
			}
			else
			{
				// more than one vertex maps to this one; we don't have classification available
				result[i] = Kind_Locked;
				TRACESTATS(3);
			}
		}
		else
		{
			assert(remap[i] < i);

			result[i] = result[remap[i]];
		}
	}

	if (options & meshopt_SimplifyPermissive)
		for (size_t i = 0; i < vertex_count; ++i)
			if (result[i] == Kind_Seam || result[i] == Kind_Locked)
			{
				if (remap[i] != i)
				{
					// only process primary vertices; wedges will be updated to match the primary vertex
					result[i] = result[remap[i]];
					continue;
				}

				bool protect = false;

				// vertex_lock 可能保护任何楔，而不只是主顶点，因此仅当所有楔都不受保护时才切换到复杂
				unsigned int v = unsigned(i);
				do
				{
					unsigned int rv = sparse_remap ? sparse_remap[v] : v;
					protect |= vertex_lock && (vertex_lock[rv] & meshopt_SimplifyVertex_Protect) != 0;
					v = wedge[v];
				} while (v != i);

				// protect if any adjoining edge doesn't have an opposite edge (indicating vertex is on the border)
				do
				{
					const EdgeAdjacency::Edge* edges = &adjacency.data[adjacency.offsets[v]];
					size_t count = adjacency.offsets[v + 1] - adjacency.offsets[v];

					for (size_t j = 0; j < count; ++j)
						protect |= !hasEdge(adjacency, edges[j].next, v, remap, wedge);
					v = wedge[v];
				} while (v != i);

				result[i] = protect ? result[i] : int(Kind_Complex);
			}

	if (vertex_lock)
	{
		// vertex_lock 可能锁定任何楔，而不只是主顶点，因此需锁定主顶点并重新锁定所有楔
		for (size_t i = 0; i < vertex_count; ++i)
		{
			unsigned int ri = sparse_remap ? sparse_remap[i] : unsigned(i);

			if (vertex_lock[ri] & meshopt_SimplifyVertex_Lock)
				result[remap[i]] = Kind_Locked;
		}

		for (size_t i = 0; i < vertex_count; ++i)
			if (result[remap[i]] == Kind_Locked)
				result[i] = Kind_Locked;
	}

	if (options & meshopt_SimplifyLockBorder)
		for (size_t i = 0; i < vertex_count; ++i)
			if (result[i] == Kind_Border)
				result[i] = Kind_Locked;

#if TRACE
	printf("locked: many open edges %d, disconnected seam %d, many seam edges %d, many wedges %d\n",
	    int(stats[0]), int(stats[1]), int(stats[2]), int(stats[3]));
#endif
}

struct Vector3
{
	float x, y, z;
};

static float rescalePositions(Vector3* result, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, const unsigned int* sparse_remap = NULL, float* out_offset = NULL)
{
	size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

	float minv[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
	float maxv[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int ri = sparse_remap ? sparse_remap[i] : unsigned(i);
		const float* v = vertex_positions_data + ri * vertex_stride_float;

		if (result)
		{
			result[i].x = v[0];
			result[i].y = v[1];
			result[i].z = v[2];
		}

		for (int j = 0; j < 3; ++j)
		{
			float vj = v[j];

			minv[j] = minv[j] > vj ? vj : minv[j];
			maxv[j] = maxv[j] < vj ? vj : maxv[j];
		}
	}

	float extent = 0.f;

	extent = (maxv[0] - minv[0]) < extent ? extent : (maxv[0] - minv[0]);
	extent = (maxv[1] - minv[1]) < extent ? extent : (maxv[1] - minv[1]);
	extent = (maxv[2] - minv[2]) < extent ? extent : (maxv[2] - minv[2]);

	if (result)
	{
		float scale = extent == 0 ? 0.f : 1.f / extent;

		for (size_t i = 0; i < vertex_count; ++i)
		{
			result[i].x = (result[i].x - minv[0]) * scale;
			result[i].y = (result[i].y - minv[1]) * scale;
			result[i].z = (result[i].z - minv[2]) * scale;
		}
	}

	if (out_offset)
	{
		out_offset[0] = minv[0];
		out_offset[1] = minv[1];
		out_offset[2] = minv[2];
	}

	return extent;
}

static void rescaleAttributes(float* result, const float* vertex_attributes_data, size_t vertex_count, size_t vertex_attributes_stride, const float* attribute_weights, size_t attribute_count, const unsigned int* attribute_remap, const unsigned int* sparse_remap)
{
	size_t vertex_attributes_stride_float = vertex_attributes_stride / sizeof(float);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int ri = sparse_remap ? sparse_remap[i] : unsigned(i);

		for (size_t k = 0; k < attribute_count; ++k)
		{
			unsigned int rk = attribute_remap[k];
			float a = vertex_attributes_data[ri * vertex_attributes_stride_float + rk];

			result[i * attribute_count + k] = a * attribute_weights[rk];
		}
	}
}

static void finalizeVertices(float* vertex_positions_data, size_t vertex_positions_stride, float* vertex_attributes_data, size_t vertex_attributes_stride, const float* attribute_weights, size_t attribute_count, size_t vertex_count, const Vector3* vertex_positions, const float* vertex_attributes, const unsigned int* sparse_remap, const unsigned int* attribute_remap, float vertex_scale, const float* vertex_offset, const unsigned char* vertex_update)
{
	size_t vertex_positions_stride_float = vertex_positions_stride / sizeof(float);
	size_t vertex_attributes_stride_float = vertex_attributes_stride / sizeof(float);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (!vertex_update[i])
			continue;

		unsigned int ri = sparse_remap ? sparse_remap[i] : unsigned(i);

		const Vector3& p = vertex_positions[i];
		float* v = vertex_positions_data + ri * vertex_positions_stride_float;

		v[0] = p.x * vertex_scale + vertex_offset[0];
		v[1] = p.y * vertex_scale + vertex_offset[1];
		v[2] = p.z * vertex_scale + vertex_offset[2];

		if (attribute_count)
		{
			const float* sa = vertex_attributes + i * attribute_count;
			float* va = vertex_attributes_data + ri * vertex_attributes_stride_float;

			for (size_t k = 0; k < attribute_count; ++k)
			{
				unsigned int rk = attribute_remap[k];

				va[rk] = sa[k] / attribute_weights[rk];
			}
		}
	}
}

static const size_t kMaxAttributes = 32;

struct Quadric
{
	// a00*x^2 + a11*y^2 + a22*z^2 + 2*a10*xy + 2*a20*xz + 2*a21*yz + 2*b0*x + 2*b1*y + 2*b2*z + c
	float a00, a11, a22;
	float a10, a20, a21;
	float b0, b1, b2, c;
	float w;
};

struct QuadricGrad
{
	// gx*x + gy*y + gz*z + gw
	float gx, gy, gz, gw;
};

struct Reservoir
{
	float x, y, z;
	float r, g, b;
	float w;
};

struct Collapse
{
	unsigned int v0;
	unsigned int v1;

	union
	{
		unsigned int bidi;
		float error;
		unsigned int errorui;
	};
};

static float normalize(Vector3& v)
{
	float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

	if (length > 0)
	{
		v.x /= length;
		v.y /= length;
		v.z /= length;
	}

	return length;
}

static void quadricAdd(Quadric& Q, const Quadric& R)
{
	Q.a00 += R.a00;
	Q.a11 += R.a11;
	Q.a22 += R.a22;
	Q.a10 += R.a10;
	Q.a20 += R.a20;
	Q.a21 += R.a21;
	Q.b0 += R.b0;
	Q.b1 += R.b1;
	Q.b2 += R.b2;
	Q.c += R.c;
	Q.w += R.w;
}

static void quadricAdd(QuadricGrad& G, const QuadricGrad& R)
{
	G.gx += R.gx;
	G.gy += R.gy;
	G.gz += R.gz;
	G.gw += R.gw;
}

static void quadricAdd(QuadricGrad* G, const QuadricGrad* R, size_t attribute_count)
{
	for (size_t k = 0; k < attribute_count; ++k)
	{
		G[k].gx += R[k].gx;
		G[k].gy += R[k].gy;
		G[k].gz += R[k].gz;
		G[k].gw += R[k].gw;
	}
}

static float quadricEval(const Quadric& Q, const Vector3& v)
{
	float rx = Q.b0;
	float ry = Q.b1;
	float rz = Q.b2;

	rx += Q.a10 * v.y;
	ry += Q.a21 * v.z;
	rz += Q.a20 * v.x;

	rx *= 2;
	ry *= 2;
	rz *= 2;

	rx += Q.a00 * v.x;
	ry += Q.a11 * v.y;
	rz += Q.a22 * v.z;

	float r = Q.c;
	r += rx * v.x;
	r += ry * v.y;
	r += rz * v.z;

	return r;
}

static float quadricError(const Quadric& Q, const Vector3& v)
{
	float r = quadricEval(Q, v);
	float s = Q.w == 0.f ? 0.f : 1.f / Q.w;

	return fabsf(r) * s;
}

static float quadricError(const Quadric& Q, const QuadricGrad* G, size_t attribute_count, const Vector3& v, const float* va)
{
	float r = quadricEval(Q, v);

	// see quadricFromAttributes for general derivation; here we need to add the parts of (eval(pos) - attr)^2 that depend on attr
	for (size_t k = 0; k < attribute_count; ++k)
	{
		float a = va[k];
		float g = v.x * G[k].gx + v.y * G[k].gy + v.z * G[k].gz + G[k].gw;

		r += a * (a * Q.w - 2 * g);
	}

	// 注意：与位置误差不同，我们不按 Q.w 归一化，以保留 quadricFromAttributes 中描述的边缩放
	return fabsf(r);
}

static void quadricFromPlane(Quadric& Q, float a, float b, float c, float d, float w)
{
	float aw = a * w;
	float bw = b * w;
	float cw = c * w;
	float dw = d * w;

	Q.a00 = a * aw;
	Q.a11 = b * bw;
	Q.a22 = c * cw;
	Q.a10 = a * bw;
	Q.a20 = a * cw;
	Q.a21 = b * cw;
	Q.b0 = a * dw;
	Q.b1 = b * dw;
	Q.b2 = c * dw;
	Q.c = d * dw;
	Q.w = w;
}

static void quadricFromPoint(Quadric& Q, float x, float y, float z, float w)
{
	Q.a00 = Q.a11 = Q.a22 = w;
	Q.a10 = Q.a20 = Q.a21 = 0;
	Q.b0 = -x * w;
	Q.b1 = -y * w;
	Q.b2 = -z * w;
	Q.c = (x * x + y * y + z * z) * w;
	Q.w = w;
}

static void quadricFromTriangle(Quadric& Q, const Vector3& p0, const Vector3& p1, const Vector3& p2, float weight)
{
	Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
	Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

	// normal = cross(p1 - p0, p2 - p0)
	Vector3 normal = {p10.y * p20.z - p10.z * p20.y, p10.z * p20.x - p10.x * p20.z, p10.x * p20.y - p10.y * p20.x};
	float area = normalize(normal);

	float distance = normal.x * p0.x + normal.y * p0.y + normal.z * p0.z;

	// we use sqrtf(area) so that the error is scaled linearly; this tends to improve silhouettes
	quadricFromPlane(Q, normal.x, normal.y, normal.z, -distance, sqrtf(area) * weight);
}

static void quadricFromTriangleEdge(Quadric& Q, const Vector3& p0, const Vector3& p1, const Vector3& p2, float weight)
{
	Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};

	// edge length; keep squared length around for projection correction
	float lengthsq = p10.x * p10.x + p10.y * p10.y + p10.z * p10.z;
	float length = sqrtf(lengthsq);

	// p20p = length of projection of p2-p0 onto p1-p0; note that p10 is unnormalized so we need to correct it later
	Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};
	float p20p = p20.x * p10.x + p20.y * p10.y + p20.z * p10.z;

	// perp = 从 p2 到线段 p1-p0 的垂直向量
	// note: since p10 is unnormalized we need to correct the projection; we scale p20 instead to take advantage of normalize below
	Vector3 perp = {p20.x * lengthsq - p10.x * p20p, p20.y * lengthsq - p10.y * p20p, p20.z * lengthsq - p10.z * p20p};
	normalize(perp);

	float distance = perp.x * p0.x + perp.y * p0.y + perp.z * p0.z;

	// note: the weight is scaled linearly with edge length; this has to match the triangle weight
	quadricFromPlane(Q, perp.x, perp.y, perp.z, -distance, length * weight);
}

static void quadricFromAttributes(Quadric& Q, QuadricGrad* G, const Vector3& p0, const Vector3& p1, const Vector3& p2, const float* va0, const float* va1, const float* va2, size_t attribute_count)
{
	// 对每个属性，我们要把以下函数编码进二次（quadric）：
	// (eval(pos) - attr)^2
	// 其中 eval(pos) 在三角形上以如下方式插值属性：
	// eval(pos) = pos.x * gx + pos.y * gy + pos.z * gz + gw
	// 其中 gx/gy/gz/gw 是梯度
	Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
	Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

	// normal = cross(p1 - p0, p2 - p0)
	Vector3 normal = {p10.y * p20.z - p10.z * p20.y, p10.z * p20.x - p10.x * p20.z, p10.x * p20.y - p10.y * p20.x};
	float area = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z) * 0.5f;

	// quadric is weighted with the square of edge length (= area)
	// this equalizes the units with the positional error (which, after normalization, is a square of distance)
	// 因此，加权属性沿距离 d 变化 1，约等价于位置变化 d
	float w = area;

	// we compute gradients using barycentric coordinates; barycentric coordinates can be computed as follows:
	// v = (d11 * d20 - d01 * d21) / denom
	// w = (d00 * d21 - d01 * d20) / denom
	// u = 1 - v - w
	// here v0, v1 are triangle edge vectors, v2 is a vector from point to triangle corner, and dij = dot(vi, vj)
	// note: v2 and d20/d21 can not be evaluated here as v2 is effectively an unknown variable; we need these only as variables for derivation of gradients
	const Vector3& v0 = p10;
	const Vector3& v1 = p20;
	float d00 = v0.x * v0.x + v0.y * v0.y + v0.z * v0.z;
	float d01 = v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
	float d11 = v1.x * v1.x + v1.y * v1.y + v1.z * v1.z;
	float denom = d00 * d11 - d01 * d01;
	float denomr = denom == 0 ? 0.f : 1.f / denom;

	// 预计算梯度因子
	// 这些通过直接对 eval(pos) = a0 * u + a1 * v + a2 * w 求导，并提取各属性间共享的表达式得到
	float gx1 = (d11 * v0.x - d01 * v1.x) * denomr;
	float gx2 = (d00 * v1.x - d01 * v0.x) * denomr;
	float gy1 = (d11 * v0.y - d01 * v1.y) * denomr;
	float gy2 = (d00 * v1.y - d01 * v0.y) * denomr;
	float gz1 = (d11 * v0.z - d01 * v1.z) * denomr;
	float gz2 = (d00 * v1.z - d01 * v0.z) * denomr;

	memset(&Q, 0, sizeof(Quadric));

	Q.w = w;

	for (size_t k = 0; k < attribute_count; ++k)
	{
		float a0 = va0[k], a1 = va1[k], a2 = va2[k];

		// 对 x/y/z/w 计算 eval(pos) 的梯度
		// 下面的公式通过对 eval(pos) = a0 * u + a1 * v + a2 * w 直接求导得到
		float gx = gx1 * (a1 - a0) + gx2 * (a2 - a0);
		float gy = gy1 * (a1 - a0) + gy2 * (a2 - a0);
		float gz = gz1 * (a1 - a0) + gz2 * (a2 - a0);
		float gw = a0 - p0.x * gx - p0.y * gy - p0.z * gz;

		// quadric encodes (eval(pos)-attr)^2; this means that the resulting expansion needs to compute, for example, pos.x * pos.y * K
		// 由于 quadric 已编码 pos.x * pos.y 的因子，几乎所有内容都可在基础 quadric 字段中累积
		// 注意：为简单起见，我们在此将所有因子乘以权重，而非在循环外
		Q.a00 += w * (gx * gx);
		Q.a11 += w * (gy * gy);
		Q.a22 += w * (gz * gz);

		Q.a10 += w * (gy * gx);
		Q.a20 += w * (gz * gx);
		Q.a21 += w * (gz * gy);

		Q.b0 += w * (gx * gw);
		Q.b1 += w * (gy * gw);
		Q.b2 += w * (gz * gw);

		Q.c += w * (gw * gw);

		// the only remaining sum components are ones that depend on attr; these will be addded during error evaluation, see quadricError
		G[k].gx = w * gx;
		G[k].gy = w * gy;
		G[k].gz = w * gz;
		G[k].gw = w * gw;
	}
}

static void quadricVolumeGradient(QuadricGrad& G, const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	Vector3 p10 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
	Vector3 p20 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};

	// normal = cross(p1 - p0, p2 - p0)
	Vector3 normal = {p10.y * p20.z - p10.z * p20.y, p10.z * p20.x - p10.x * p20.z, p10.x * p20.y - p10.y * p20.x};
	float area = normalize(normal) * 0.5f;

	G.gx = normal.x * area;
	G.gy = normal.y * area;
	G.gz = normal.z * area;
	G.gw = (-p0.x * normal.x - p0.y * normal.y - p0.z * normal.z) * area;
}

static bool quadricSolve(Vector3& p, const Quadric& Q, const QuadricGrad& GV)
{
	// 求解 A*p = -b，其中 A 是 quadric 矩阵，b 是线性项
	float a00 = Q.a00, a11 = Q.a11, a22 = Q.a22;
	float a10 = Q.a10, a20 = Q.a20, a21 = Q.a21;
	float x0 = -Q.b0, x1 = -Q.b1, x2 = -Q.b2;

	float eps = 1e-6f * Q.w;

	// LDL 分解：A = LDL^T
	float d0 = a00;
	float l10 = a10 / d0;
	float l20 = a20 / d0;

	float d1 = a11 - a10 * l10;
	float dl21 = a21 - a20 * l10;
	float l21 = dl21 / d1;

	float d2 = a22 - a20 * l20 - dl21 * l21;

	// solve L*y = x
	float y0 = x0;
	float y1 = x1 - l10 * y0;
	float y2 = x2 - l20 * y0 - l21 * y1;

	// solve D*z = y
	float z0 = y0 / d0;
	float z1 = y1 / d1;
	float z2 = y2 / d2;

	// 使用拉格朗日乘子，以线性约束 GV 扩充方程组
	float a30 = GV.gx, a31 = GV.gy, a32 = GV.gz;
	float x3 = -GV.gw;

	float l30 = a30 / d0;
	float dl31 = a31 - a30 * l10;
	float l31 = dl31 / d1;
	float dl32 = a32 - a30 * l20 - dl31 * l21;
	float l32 = dl32 / d2;
	float d3 = 0.f - a30 * l30 - dl31 * l31 - dl32 * l32;

	float y3 = x3 - l30 * y0 - l31 * y1 - l32 * y2;
	float z3 = fabsf(d3) > eps ? y3 / d3 : 0.f; // if d3 is zero, we can ignore the constraint

	// substitute L^T*p = z
	float lambda = z3;
	float pz = z2 - l32 * lambda;
	float py = z1 - l21 * pz - l31 * lambda;
	float px = z0 - l10 * py - l20 * pz - l30 * lambda;

	p.x = px;
	p.y = py;
	p.z = pz;

	return fabsf(d0) > eps && fabsf(d1) > eps && fabsf(d2) > eps;
}

static void quadricReduceAttributes(Quadric& Q, const Quadric& A, const QuadricGrad* G, size_t attribute_count)
{
	// update vertex quadric with attribute quadric; multiply by vertex weight to minimize normalized error
	Q.a00 += A.a00 * Q.w;
	Q.a11 += A.a11 * Q.w;
	Q.a22 += A.a22 * Q.w;
	Q.a10 += A.a10 * Q.w;
	Q.a20 += A.a20 * Q.w;
	Q.a21 += A.a21 * Q.w;
	Q.b0 += A.b0 * Q.w;
	Q.b1 += A.b1 * Q.w;
	Q.b2 += A.b2 * Q.w;

	float iaw = A.w == 0 ? 0.f : Q.w / A.w;

	// update linear system based on attribute gradients (BB^T/a)
	for (size_t k = 0; k < attribute_count; ++k)
	{
		const QuadricGrad& g = G[k];

		Q.a00 -= (g.gx * g.gx) * iaw;
		Q.a11 -= (g.gy * g.gy) * iaw;
		Q.a22 -= (g.gz * g.gz) * iaw;
		Q.a10 -= (g.gx * g.gy) * iaw;
		Q.a20 -= (g.gx * g.gz) * iaw;
		Q.a21 -= (g.gy * g.gz) * iaw;

		Q.b0 -= (g.gx * g.gw) * iaw;
		Q.b1 -= (g.gy * g.gw) * iaw;
		Q.b2 -= (g.gz * g.gw) * iaw;
	}
}

static void fillFaceQuadrics(Quadric* vertex_quadrics, QuadricGrad* volume_gradients, const unsigned int* indices, size_t index_count, const Vector3* vertex_positions, const unsigned int* remap)
{
	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int i0 = indices[i + 0];
		unsigned int i1 = indices[i + 1];
		unsigned int i2 = indices[i + 2];

		Quadric Q;
		quadricFromTriangle(Q, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2], 1.f);

		quadricAdd(vertex_quadrics[remap[i0]], Q);
		quadricAdd(vertex_quadrics[remap[i1]], Q);
		quadricAdd(vertex_quadrics[remap[i2]], Q);

		if (volume_gradients)
		{
			QuadricGrad GV;
			quadricVolumeGradient(GV, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2]);

			quadricAdd(volume_gradients[remap[i0]], GV);
			quadricAdd(volume_gradients[remap[i1]], GV);
			quadricAdd(volume_gradients[remap[i2]], GV);
		}
	}
}

static void fillVertexQuadrics(Quadric* vertex_quadrics, const Vector3* vertex_positions, size_t vertex_count, const unsigned int* remap, unsigned int options)
{
	// 默认情况下，我们使用一个非常小的权重来改善三角剖分和数值稳定性，而不影响形状或误差
	float factor = (options & meshopt_SimplifyRegularize) ? 1e-1f : 1e-7f;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (remap[i] != i)
			continue;

		const Vector3& p = vertex_positions[i];
		float w = vertex_quadrics[i].w * factor;

		Quadric Q;
		quadricFromPoint(Q, p.x, p.y, p.z, w);

		quadricAdd(vertex_quadrics[i], Q);
	}
}

static void fillEdgeQuadrics(Quadric* vertex_quadrics, const unsigned int* indices, size_t index_count, const Vector3* vertex_positions, const unsigned int* remap, const unsigned char* vertex_kind, const unsigned int* loop, const unsigned int* loopback)
{
	for (size_t i = 0; i < index_count; i += 3)
	{
		static const int next[4] = {1, 2, 0, 1};

		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];

			unsigned char k0 = vertex_kind[i0];
			unsigned char k1 = vertex_kind[i1];

			// 检查 i0 或 i1 是否为 border/seam，并且位于同一边缘环上
			// 注意：即使对于连接 border 与 locked 等类型的边，我们也需要累加误差
			// if we don't do that, the adjacent border->border edge won't have correct errors for corners
			if (k0 != Kind_Border && k0 != Kind_Seam && k1 != Kind_Border && k1 != Kind_Seam)
				continue;

			if ((k0 == Kind_Border || k0 == Kind_Seam) && loop[i0] != i1)
				continue;

			if ((k1 == Kind_Border || k1 == Kind_Seam) && loopback[i1] != i0)
				continue;

			unsigned int i2 = indices[i + next[e + 1]];

			// we try hard to maintain border edge geometry; seam edges can move more freely
			// 由于折叠受到拓扑限制，seam 二次型略能改善折叠结构，但并非关键
			const float kEdgeWeightSeam = 0.5f; // 由于相对的边，需应用两次
			const float kEdgeWeightBorder = 10.f;

			float edgeWeight = (k0 == Kind_Border || k1 == Kind_Border) ? kEdgeWeightBorder : kEdgeWeightSeam;

			Quadric Q;
			quadricFromTriangleEdge(Q, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2], edgeWeight);

			Quadric QT;
			quadricFromTriangle(QT, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2], edgeWeight);

			// mix edge quadric with triangle quadric to stabilize collapses in both directions; both quadrics inherit edge weight so that their error is added
			QT.w = 0;
			quadricAdd(Q, QT);

			quadricAdd(vertex_quadrics[remap[i0]], Q);
			quadricAdd(vertex_quadrics[remap[i1]], Q);
		}
	}
}

static void fillAttributeQuadrics(Quadric* attribute_quadrics, QuadricGrad* attribute_gradients, const unsigned int* indices, size_t index_count, const Vector3* vertex_positions, const float* vertex_attributes, size_t attribute_count)
{
	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int i0 = indices[i + 0];
		unsigned int i1 = indices[i + 1];
		unsigned int i2 = indices[i + 2];

		Quadric QA;
		QuadricGrad G[kMaxAttributes];
		quadricFromAttributes(QA, G, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2], &vertex_attributes[i0 * attribute_count], &vertex_attributes[i1 * attribute_count], &vertex_attributes[i2 * attribute_count], attribute_count);

		quadricAdd(attribute_quadrics[i0], QA);
		quadricAdd(attribute_quadrics[i1], QA);
		quadricAdd(attribute_quadrics[i2], QA);

		quadricAdd(&attribute_gradients[i0 * attribute_count], G, attribute_count);
		quadricAdd(&attribute_gradients[i1 * attribute_count], G, attribute_count);
		quadricAdd(&attribute_gradients[i2 * attribute_count], G, attribute_count);
	}
}

// 当 C 被替换为 D 时，三角形 ABC 是否会发生翻转？
static bool hasTriangleFlip(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d)
{
	Vector3 eb = {b.x - a.x, b.y - a.y, b.z - a.z};
	Vector3 ec = {c.x - a.x, c.y - a.y, c.z - a.z};
	Vector3 ed = {d.x - a.x, d.y - a.y, d.z - a.z};

	Vector3 nbc = {eb.y * ec.z - eb.z * ec.y, eb.z * ec.x - eb.x * ec.z, eb.x * ec.y - eb.y * ec.x};
	Vector3 nbd = {eb.y * ed.z - eb.z * ed.y, eb.z * ed.x - eb.x * ed.z, eb.x * ed.y - eb.y * ed.x};

	float ndp = nbc.x * nbd.x + nbc.y * nbd.y + nbc.z * nbd.z;
	float abc = nbc.x * nbc.x + nbc.y * nbc.y + nbc.z * nbc.z;
	float abd = nbd.x * nbd.x + nbd.y * nbd.y + nbd.z * nbd.z;

	// scale is cos(angle); somewhat arbitrarily set to ~75 degrees
	// 注意："纯"检查是 ndp <= 0（90 度截止），但这允许通过一系列接近 90 度的折叠发生翻转
	return ndp <= 0.25f * sqrtf(abc * abd);
}

static bool hasTriangleFlips(const EdgeAdjacency& adjacency, const Vector3* vertex_positions, const unsigned int* collapse_remap, unsigned int i0, unsigned int i1)
{
	assert(collapse_remap[i0] == i0);
	assert(collapse_remap[i1] == i1);

	const Vector3& v0 = vertex_positions[i0];
	const Vector3& v1 = vertex_positions[i1];

	const EdgeAdjacency::Edge* edges = &adjacency.data[adjacency.offsets[i0]];
	size_t count = adjacency.offsets[i0 + 1] - adjacency.offsets[i0];

	for (size_t i = 0; i < count; ++i)
	{
		unsigned int a = collapse_remap[edges[i].next];
		unsigned int b = collapse_remap[edges[i].prev];

		// 跳过那些会因 i0->i1 折叠而被折叠或先前已折叠的三角形
		if (a == i1 || b == i1 || a == b)
			continue;

		// 当因折叠而导致至少一个三角形翻转时提前退出
		if (hasTriangleFlip(vertex_positions[a], vertex_positions[b], v0, v1))
		{
#if TRACE >= 2
			printf("edge block %d -> %d: flip welded %d %d %d\n", i0, i1, a, i0, b);
#endif

			return true;
		}
	}

	return false;
}

static bool hasTriangleFlips(const EdgeAdjacency& adjacency, const Vector3* vertex_positions, unsigned int i0, const Vector3& v1)
{
	const Vector3& v0 = vertex_positions[i0];

	const EdgeAdjacency::Edge* edges = &adjacency.data[adjacency.offsets[i0]];
	size_t count = adjacency.offsets[i0 + 1] - adjacency.offsets[i0];

	for (size_t i = 0; i < count; ++i)
	{
		unsigned int a = edges[i].next, b = edges[i].prev;

		if (hasTriangleFlip(vertex_positions[a], vertex_positions[b], v0, v1))
			return true;
	}

	return false;
}

static float getNeighborhoodRadius(const EdgeAdjacency& adjacency, const Vector3* vertex_positions, unsigned int i0)
{
	const Vector3& v0 = vertex_positions[i0];

	const EdgeAdjacency::Edge* edges = &adjacency.data[adjacency.offsets[i0]];
	size_t count = adjacency.offsets[i0 + 1] - adjacency.offsets[i0];

	float result = 0.f;

	for (size_t i = 0; i < count; ++i)
	{
		unsigned int a = edges[i].next, b = edges[i].prev;

		const Vector3& va = vertex_positions[a];
		const Vector3& vb = vertex_positions[b];

		float da = (va.x - v0.x) * (va.x - v0.x) + (va.y - v0.y) * (va.y - v0.y) + (va.z - v0.z) * (va.z - v0.z);
		float db = (vb.x - v0.x) * (vb.x - v0.x) + (vb.y - v0.y) * (vb.y - v0.y) + (vb.z - v0.z) * (vb.z - v0.z);

		result = result < da ? da : result;
		result = result < db ? db : result;
	}

	return sqrtf(result);
}

static size_t boundEdgeCollapses(const EdgeAdjacency& adjacency, size_t vertex_count, size_t index_count, unsigned char* vertex_kind)
{
	size_t dual_count = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned char k = vertex_kind[i];
		unsigned int e = adjacency.offsets[i + 1] - adjacency.offsets[i];

		dual_count += (k == Kind_Manifold || k == Kind_Seam) ? e : 0;
	}

	assert(dual_count <= index_count);

	// 将容量填充 3，以便可以每三角形一次进行溢出检查，而不是每边一次
	return (index_count - dual_count / 2) + 3;
}

static size_t pickEdgeCollapses(Collapse* collapses, size_t collapse_capacity, const unsigned int* indices, size_t index_count, const unsigned int* remap, const unsigned char* vertex_kind, const unsigned int* loop, const unsigned int* loopback)
{
	size_t collapse_count = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		static const int next[3] = {1, 2, 0};

		// 这不应发生，因为 boundEdgeCollapses 应给出折叠次数的上界，但在极少数情况下，我们只需丢弃多余的折叠
		if (collapse_count + 3 > collapse_capacity)
			break;

		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];

			// 这可能发生在输入具有零长度边时，或当我们为 complex 拓扑执行折叠时
			// 连同 seams，将连接两个 wedge 的 manifold 顶点折叠到其中一个上
			// 我们会保留此类边，因为它们对保持网格完整性可能很重要
			if (remap[i0] == remap[i1])
				continue;

			unsigned char k0 = vertex_kind[i0];
			unsigned char k1 = vertex_kind[i1];

			// 该边必须至少在某个方向上可折叠
			if (!(kCanCollapse[k0][k1] | kCanCollapse[k1][k0]))
				continue;

			// manifold 和 seam 边应出现两次（i0->i1 和 i1->i0）——跳过多余的边
			if (kHasOpposite[k0][k1] && remap[i1] > remap[i0])
				continue;

			// 两个顶点位于 border 或 seam 上，但两者之间没有直接的边
			// 这表明它们属于两个不同的边环，我们不应折叠这条边
			// loop[] 和 loopback[] 跟踪半边，因此我们只需检查其中一个
			if ((k0 == Kind_Border || k0 == Kind_Seam) && k1 != Kind_Manifold && loop[i0] != i1)
				continue;
			if ((k1 == Kind_Border || k1 == Kind_Seam) && k0 != Kind_Manifold && loopback[i1] != i0)
				continue;

			// 边可以在任一方向上折叠——我们将选择误差最小的那一个
			// 注意：我们稍后在折叠排序阶段评估误差，这里仅将边标记为双向
			if (kCanCollapse[k0][k1] & kCanCollapse[k1][k0])
			{
				Collapse c = {i0, i1, {/* bidi= */ 1}};
				collapses[collapse_count++] = c;
			}
			else
			{
				// 边只能在一个方向上折叠
				unsigned int e0 = kCanCollapse[k0][k1] ? i0 : i1;
				unsigned int e1 = kCanCollapse[k0][k1] ? i1 : i0;

				Collapse c = {e0, e1, {/* bidi= */ 0}};
				collapses[collapse_count++] = c;
			}
		}
	}

	return collapse_count;
}

static void rankEdgeCollapses(Collapse* collapses, size_t collapse_count, const Vector3* vertex_positions, const float* vertex_attributes, const Quadric* vertex_quadrics, const Quadric* attribute_quadrics, const QuadricGrad* attribute_gradients, size_t attribute_count, const unsigned int* remap, const unsigned int* wedge, const unsigned char* vertex_kind, const unsigned int* loop, const unsigned int* loopback)
{
	for (size_t i = 0; i < collapse_count; ++i)
	{
		Collapse& c = collapses[i];

		unsigned int i0 = c.v0;
		unsigned int i1 = c.v1;
		bool bidi = c.bidi;

		float ei = quadricError(vertex_quadrics[remap[i0]], vertex_positions[i1]);
		float ej = bidi ? quadricError(vertex_quadrics[remap[i1]], vertex_positions[i0]) : FLT_MAX;

#if TRACE >= 3
		float di = ei, dj = ej;
#endif

		if (attribute_count)
		{
			ei += quadricError(attribute_quadrics[i0], &attribute_gradients[i0 * attribute_count], attribute_count, vertex_positions[i1], &vertex_attributes[i1 * attribute_count]);
			ej += bidi ? quadricError(attribute_quadrics[i1], &attribute_gradients[i1 * attribute_count], attribute_count, vertex_positions[i0], &vertex_attributes[i0 * attribute_count]) : 0;

			// seam 边需要聚合 primary 与 secondary 边之间的属性误差，因为属性二次型是分开的
			if (vertex_kind[i0] == Kind_Seam)
			{
				// for seam collapses we need to find the seam pair; this is a bit tricky since we need to rely on edge loops as target vertex may be locked (and thus have more than two wedges)
				unsigned int s0 = wedge[i0];
				unsigned int s1 = loop[i0] == i1 ? loopback[s0] : loop[s0];

				assert(wedge[s0] == i0); // 对于半个接缝（half-seam），s0 可能等于 i0
				assert(s1 != ~0u && remap[s1] == remap[i1]);

				// note: this should never happen due to the assertion above, but when disabled if we ever hit this case we'll get a memory safety issue; for now play it safe
				s1 = (s1 != ~0u) ? s1 : wedge[i1];

				ei += quadricError(attribute_quadrics[s0], &attribute_gradients[s0 * attribute_count], attribute_count, vertex_positions[s1], &vertex_attributes[s1 * attribute_count]);
				ej += bidi ? quadricError(attribute_quadrics[s1], &attribute_gradients[s1 * attribute_count], attribute_count, vertex_positions[s0], &vertex_attributes[s0 * attribute_count]) : 0;
			}
			else
			{
				// complex 边可以有多个 wedge，因此我们需要聚合所有 wedge 的误差
				// 这与 seams（按两两聚合）不同，因为所有 wedge 都折叠到同一目标上
				if (vertex_kind[i0] == Kind_Complex)
					for (unsigned int v = wedge[i0]; v != i0; v = wedge[v])
						ei += quadricError(attribute_quadrics[v], &attribute_gradients[v * attribute_count], attribute_count, vertex_positions[i1], &vertex_attributes[i1 * attribute_count]);

				if (vertex_kind[i1] == Kind_Complex && bidi)
					for (unsigned int v = wedge[i1]; v != i1; v = wedge[v])
						ej += quadricError(attribute_quadrics[v], &attribute_gradients[v * attribute_count], attribute_count, vertex_positions[i0], &vertex_attributes[i0 * attribute_count]);
			}
		}

		// pick edge direction with minimal error (branchless)
		bool rev = bidi & (ej < ei);

		c.v0 = rev ? i1 : i0;
		c.v1 = rev ? i0 : i1;
		c.error = ej < ei ? ej : ei;

#if TRACE >= 3
		if (bidi)
			printf("edge eval %d -> %d: error %f (pos %f, attr %f); reverse %f (pos %f, attr %f)\n",
			    rev ? i1 : i0, rev ? i0 : i1,
			    sqrtf(rev ? ej : ei), sqrtf(rev ? dj : di), sqrtf(rev ? ej - dj : ei - di),
			    sqrtf(rev ? ei : ej), sqrtf(rev ? di : dj), sqrtf(rev ? ei - di : ej - dj));
		else
			printf("edge eval %d -> %d: error %f (pos %f, attr %f)\n", i0, i1, sqrtf(c.error), sqrtf(di), sqrtf(ei - di));
#endif
	}
}

static void sortEdgeCollapses(unsigned int* sort_order, const Collapse* collapses, size_t collapse_count)
{
	// we use counting sort to order collapses by error; since the exact sort order is not as critical,
	// 仅使用 exponent+mantissa 的高 12 位（8 位指数和 4 位尾数）。
	// 为避免过多的栈开销，我们限制指数范围，因为误差远高于 1 的折叠没有意义。
	const unsigned int sort_bits = 12;
	const unsigned int sort_bins = 2048 + 512; // exponent range [-127, 32)

	// 为计数排序填充直方图
	unsigned int histogram[sort_bins];
	memset(histogram, 0, sizeof(histogram));

	for (size_t i = 0; i < collapse_count; ++i)
	{
		// 由于误差非负，跳过符号位
		unsigned int error = collapses[i].errorui;
		unsigned int key = (error << 1) >> (32 - sort_bits);
		key = key < sort_bins ? key : sort_bins - 1;

		histogram[key]++;
	}

	// 基于直方图数据计算偏移
	size_t histogram_sum = 0;

	for (size_t i = 0; i < sort_bins; ++i)
	{
		size_t count = histogram[i];
		histogram[i] = unsigned(histogram_sum);
		histogram_sum += count;
	}

	assert(histogram_sum == collapse_count);

	// 基于偏移计算排序顺序
	for (size_t i = 0; i < collapse_count; ++i)
	{
		// 由于误差非负，跳过符号位
		unsigned int error = collapses[i].errorui;
		unsigned int key = (error << 1) >> (32 - sort_bits);
		key = key < sort_bins ? key : sort_bins - 1;

		sort_order[histogram[key]++] = unsigned(i);
	}
}

static size_t performEdgeCollapses(unsigned int* collapse_remap, unsigned char* collapse_locked, const Collapse* collapses, size_t collapse_count, const unsigned int* collapse_order, const unsigned int* remap, const unsigned int* wedge, const unsigned char* vertex_kind, const unsigned int* loop, const unsigned int* loopback, const Vector3* vertex_positions, const EdgeAdjacency& adjacency, size_t triangle_collapse_goal, float error_limit, float& result_error)
{
	size_t edge_collapses = 0;
	size_t triangle_collapses = 0;

	// most collapses remove 2 triangles; use this to establish a bound on the pass in terms of error limit
	// note that edge_collapse_goal is an estimate; triangle_collapse_goal will be used to actually limit collapses
	size_t edge_collapse_goal = triangle_collapse_goal / 2;

#if TRACE
	size_t stats[7] = {};
#endif

	for (size_t i = 0; i < collapse_count; ++i)
	{
		const Collapse& c = collapses[collapse_order[i]];

		TRACESTATS(0);

		if (c.error > error_limit)
		{
			TRACESTATS(4);
			break;
		}

		if (triangle_collapses >= triangle_collapse_goal)
		{
			TRACESTATS(5);
			break;
		}

		// we limit the error in each pass based on the error of optimal last collapse; since many collapses will be locked
		// 由于它们会与其它成功的折叠共享顶点，我们需要将可接受的误差增大一定倍数
		float error_goal = edge_collapse_goal < collapse_count ? 1.5f * collapses[collapse_order[edge_collapse_goal]].error : FLT_MAX;

		// on average, each collapse is expected to lock 6 other collapses; to avoid degenerate passes on meshes with odd
		// 拓扑，我们仅在折叠数量超过 1/6 时才相应中止。
		if (c.error > error_goal && c.error > result_error && triangle_collapses > triangle_collapse_goal / 6)
		{
			TRACESTATS(6);
			break;
		}

		unsigned int i0 = c.v0;
		unsigned int i1 = c.v1;

		unsigned int r0 = remap[i0];
		unsigned int r1 = remap[i1];

		unsigned char kind = vertex_kind[i0];

		// 我们不折叠其源顶点或目标顶点已参与折叠的边
		// 重要的是不要移动顶点两次，因为这会复杂化跟踪/重映射逻辑
		// 重要的是不要将其它顶点向已移动的顶点靠近以保持误差，因为我们在遍历过程中不会重新排序折叠
		if (collapse_locked[r0] | collapse_locked[r1])
		{
			TRACESTATS(1);
			continue;
		}

		if (hasTriangleFlips(adjacency, vertex_positions, collapse_remap, r0, r1))
		{
			// 调整折叠目标，因为此次折叠无效，不应计入误差目标
			edge_collapse_goal++;

			TRACESTATS(2);
			continue;
		}

#if TRACE >= 2
		printf("edge commit %d -> %d: kind %d->%d, error %f\n", i0, i1, vertex_kind[i0], vertex_kind[i1], sqrtf(c.error));
#endif

		assert(collapse_remap[r0] == r0);
		assert(collapse_remap[r1] == r1);

		if (kind == Kind_Complex)
		{
			// 将 complex 中的所有顶点重映射到目标顶点
			unsigned int v = i0;

			do
			{
				collapse_remap[v] = i1;
				v = wedge[v];
			} while (v != i0);
		}
		else if (kind == Kind_Seam)
		{
			// for seam collapses we need to move the seam pair together; this is a bit tricky since we need to rely on edge loops as target vertex may be locked (and thus have more than two wedges)
			unsigned int s0 = wedge[i0];
			unsigned int s1 = loop[i0] == i1 ? loopback[s0] : loop[s0];
			assert(wedge[s0] == i0); // 对于半个接缝（half-seam），s0 可能等于 i0
			assert(s1 != ~0u && remap[s1] == r1);

			// 额外的断言，用于验证 seam 对是否一致
			assert(kind != vertex_kind[i1] || s1 == wedge[i1]);
			assert(loop[i0] == i1 || loopback[i0] == i1);
			assert(loop[s0] == s1 || loopback[s0] == s1);

			// note: this should never happen due to the assertion above, but when disabled if we ever hit this case we'll get a memory safety issue; for now play it safe
			s1 = (s1 != ~0u) ? s1 : wedge[i1];

			collapse_remap[i0] = i1;
			collapse_remap[s0] = s1;
		}
		else
		{
			assert(wedge[i0] == i0);

			collapse_remap[i0] = i1;
		}

		// 注意：从技术上讲，若 r1 是 locked 顶点，我们无需锁定它，因为它无法移动且其二次型不会被使用
		// 然而，这会使得某些网格上的误差略差，因为 locked 折叠在调度上获得了不公平的优势
		collapse_locked[r0] = 1;
		collapse_locked[r1] = 1;

		// border 边折叠 1 个三角形，其它边折叠 2 个或更多
		triangle_collapses += (kind == Kind_Border) ? 1 : 2;
		edge_collapses++;

		result_error = result_error < c.error ? c.error : result_error;
	}

#if TRACE
	float error_goal_last = edge_collapse_goal < collapse_count ? 1.5f * collapses[collapse_order[edge_collapse_goal]].error : FLT_MAX;
	float error_goal_limit = error_goal_last < error_limit ? error_goal_last : error_limit;

	printf("removed %d triangles, error %e (goal %e); evaluated %d/%d collapses (done %d, skipped %d, invalid %d); %s\n",
	    int(triangle_collapses), sqrtf(result_error), sqrtf(error_goal_limit),
	    int(stats[0]), int(collapse_count), int(edge_collapses), int(stats[1]), int(stats[2]),
	    stats[4] ? "error limit" : (stats[5] ? "count limit" : (stats[6] ? "error goal" : "out of collapses")));
#endif

	return edge_collapses;
}

static void updateQuadrics(const unsigned int* collapse_remap, size_t vertex_count, Quadric* vertex_quadrics, QuadricGrad* volume_gradients, Quadric* attribute_quadrics, QuadricGrad* attribute_gradients, size_t attribute_count, const Vector3* vertex_positions, const unsigned int* remap, float& vertex_error)
{
	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (collapse_remap[i] == i)
			continue;

		unsigned int i0 = unsigned(i);
		unsigned int i1 = collapse_remap[i];

		unsigned int r0 = remap[i0];
		unsigned int r1 = remap[i1];

		// 确保我们仅更新 vertex_quadrics 一次：若任一 wedge 被移动，则 primary 顶点必须被移动
		if (i0 == r0)
		{
			quadricAdd(vertex_quadrics[r1], vertex_quadrics[r0]);

			if (volume_gradients)
				quadricAdd(volume_gradients[r1], volume_gradients[r0]);
		}

		if (attribute_count)
		{
			quadricAdd(attribute_quadrics[i1], attribute_quadrics[i0]);
			quadricAdd(&attribute_gradients[i1 * attribute_count], &attribute_gradients[i0 * attribute_count], attribute_count);

			if (i0 == r0)
			{
				// when attributes are used, distance error needs to be recomputed as collapses don't track it; it is safe to do this after the quadric adjustment
				float derr = quadricError(vertex_quadrics[r0], vertex_positions[r1]);
				vertex_error = vertex_error < derr ? derr : vertex_error;
			}
		}
	}
}

static void solveQuadrics(Vector3* vertex_positions, float* vertex_attributes, size_t vertex_count, const Quadric* vertex_quadrics, const QuadricGrad* volume_gradients, const Quadric* attribute_quadrics, const QuadricGrad* attribute_gradients, size_t attribute_count, const unsigned int* remap, const unsigned int* wedge, const EdgeAdjacency& adjacency, const unsigned char* vertex_kind, const unsigned char* vertex_update)
{
#if TRACE
	size_t stats[5] = {};
#endif

	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (!vertex_update[i])
			continue;

		// 禁止移动外部锁定的顶点
		// 在属性不连续处移动顶点可能导致将 UV 外推到 chart 边界之外
		// 移动 border 上的顶点需要更强的边二次型，以保持 border 的几何结构
		if (vertex_kind[i] == Kind_Locked || vertex_kind[i] == Kind_Seam || vertex_kind[i] == Kind_Border)
			continue;

		if (remap[i] != i)
		{
			vertex_positions[i] = vertex_positions[remap[i]];
			continue;
		}

		TRACESTATS(0);

		const Vector3& vp = vertex_positions[i];

		Quadric Q = vertex_quadrics[i];
		QuadricGrad GV = {};

		// 添加一个点二次型用于正则化，以稳定求解
		Quadric R;
		quadricFromPoint(R, vp.x, vp.y, vp.z, Q.w * 1e-4f);
		quadricAdd(Q, R);

		if (attribute_count)
		{
			// 最优点同时最小化所有 wedge 的属性二次型
			unsigned int v = unsigned(i);
			do
			{
				quadricReduceAttributes(Q, attribute_quadrics[v], &attribute_gradients[v * attribute_count], attribute_count);
				v = wedge[v];
			} while (v != i);

			// 最小化属性二次型会导致体积损失，因此我们将体积梯度作为约束纳入
			if (volume_gradients)
				GV = volume_gradients[i];
		}

		Vector3 p;
		if (!quadricSolve(p, Q, GV))
		{
			TRACESTATS(2);
			continue;
		}

		// 拒绝使顶点过度偏离其邻域的更新
		// 这可在二次型未正确定义的多数情况下检测并修复问题
		float nr = getNeighborhoodRadius(adjacency, vertex_positions, unsigned(i));
		float dp = (p.x - vp.x) * (p.x - vp.x) + (p.y - vp.y) * (p.y - vp.y) + (p.z - vp.z) * (p.z - vp.z);

		if (dp > nr * nr)
		{
			TRACESTATS(3);
			continue;
		}

		// 拒绝会使相邻三角形翻转的更新，正如边折叠时所做的
		if (hasTriangleFlips(adjacency, vertex_positions, unsigned(i), p))
		{
			TRACESTATS(4);
			continue;
		}

		TRACESTATS(1);
		vertex_positions[i] = p;
	}

#if TRACE
	printf("updated %d/%d positions; failed solve %d bounds %d flip %d\n", int(stats[1]), int(stats[0]), int(stats[2]), int(stats[3]), int(stats[4]));
#endif

	if (attribute_count == 0)
		return;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (!vertex_update[i])
			continue;

		// 禁止更新外部锁定的顶点
		if (vertex_kind[i] == Kind_Locked)
			continue;

		const Vector3& p = vertex_positions[remap[i]];
		const Quadric& A = attribute_quadrics[i];

		float iw = A.w == 0 ? 0.f : 1.f / A.w;

		for (size_t k = 0; k < attribute_count; ++k)
		{
			const QuadricGrad& G = attribute_gradients[i * attribute_count + k];

			vertex_attributes[i * attribute_count + k] = (G.gx * p.x + G.gy * p.y + G.gz * p.z + G.gw) * iw;
		}
	}
}

static size_t remapIndexBuffer(unsigned int* indices, size_t index_count, const unsigned int* collapse_remap, const unsigned int* remap)
{
	size_t write = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int v0 = collapse_remap[indices[i + 0]];
		unsigned int v1 = collapse_remap[indices[i + 1]];
		unsigned int v2 = collapse_remap[indices[i + 2]];

		// 在单次遍历中我们从不移动顶点两次
		assert(collapse_remap[v0] == v0);
		assert(collapse_remap[v1] == v1);
		assert(collapse_remap[v2] == v2);

		// 即使三角形并非拓扑退化，也应折叠零面积三角形
		// 当顶点被折叠到 seam 对上时，需要此步骤来清理 manifold->seam 折叠
		// 同样适用于 complex 折叠以及执行跨 wedge 折叠的其它一些情况
		unsigned int r0 = remap[v0];
		unsigned int r1 = remap[v1];
		unsigned int r2 = remap[v2];

		if (r0 != r1 && r0 != r2 && r1 != r2)
		{
			indices[write + 0] = v0;
			indices[write + 1] = v1;
			indices[write + 2] = v2;
			write += 3;
		}
	}

	return write;
}

static void remapEdgeLoops(unsigned int* loop, size_t vertex_count, const unsigned int* collapse_remap)
{
	for (size_t i = 0; i < vertex_count; ++i)
	{
		// 注意：对于已被重映射的顶点，这是一次空操作
		// 理想情况下我们会清除这些顶点的 loop 条目以保持一致，尽管它们不会被使用
		// 然而，重映射过程需要已重映射顶点的 loop 信息，因此这需要一次单独的遍历
		if (loop[i] != ~0u)
		{
			unsigned int l = loop[i];
			unsigned int r = collapse_remap[l];

			// 当 seam 边沿与 loop 方向相反的方向折叠时，i == r 是一个特例
			if (i == r)
				loop[i] = (loop[l] != ~0u) ? collapse_remap[loop[l]] : ~0u;
			else
				loop[i] = r;
		}
	}
}

static unsigned int follow(unsigned int* parents, unsigned int index)
{
	while (index != parents[index])
	{
		unsigned int parent = parents[index];
		parents[index] = parents[parent];
		index = parent;
	}

	return index;
}

static size_t buildComponents(unsigned int* components, size_t vertex_count, const unsigned int* indices, size_t index_count, const unsigned int* remap)
{
	for (size_t i = 0; i < vertex_count; ++i)
		components[i] = unsigned(i);

	// 通过 union-find 为每个 component 计算唯一（但非连续！）索引
	for (size_t i = 0; i < index_count; i += 3)
	{
		static const int next[4] = {1, 2, 0, 1};

		for (int e = 0; e < 3; ++e)
		{
			unsigned int i0 = indices[i + e];
			unsigned int i1 = indices[i + next[e]];

			unsigned int r0 = remap[i0];
			unsigned int r1 = remap[i1];

			r0 = follow(components, r0);
			r1 = follow(components, r1);

			// 将具有较大索引的 components 合并到具有较小索引的 components 中
			// 这保证了 component 的根始终是具有最小索引的那个
			if (r0 != r1)
				components[r0 < r1 ? r1 : r0] = r0 < r1 ? r0 : r1;
		}
	}

	// 确保每个元素在重编号 components *之前* 指向 component 根
	for (size_t i = 0; i < vertex_count; ++i)
		if (remap[i] == i)
			components[i] = follow(components, unsigned(i));

	unsigned int next_component = 0;

	// 使用连续索引对 components 重新编号
	// 一次连续遍历即可，因为 component 根总是具有最小索引
	// 注意：此遍历中使用 follow() 是不安全的，因为我们就地用连续索引替换了 component 链接
	for (size_t i = 0; i < vertex_count; ++i)
	{
		if (remap[i] == i)
		{
			unsigned int root = components[i];
			assert(root <= i); // 确保我们已为非根元素计算了 component
			components[i] = (root == i) ? next_component++ : components[root];
		}
		else
		{
			assert(remap[i] < i); // 确保我们已计算了 component
			components[i] = components[remap[i]];
		}
	}

	return next_component;
}

static void measureComponents(float* component_errors, size_t component_count, const unsigned int* components, const Vector3* vertex_positions, size_t vertex_count)
{
	memset(component_errors, 0, component_count * 4 * sizeof(float));

	// 计算每个 component 的近似球心（取其平均值）
	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int c = components[i];
		assert(components[i] < component_count);

		Vector3 v = vertex_positions[i]; // 复制可避免别名问题

		component_errors[c * 4 + 0] += v.x;
		component_errors[c * 4 + 1] += v.y;
		component_errors[c * 4 + 2] += v.z;
		component_errors[c * 4 + 3] += 1; // weight
	}

	// 完成中心计算，并将 [3] 重新初始化为半径
	for (size_t i = 0; i < component_count; ++i)
	{
		float w = component_errors[i * 4 + 3];
		float iw = w == 0.f ? 0.f : 1.f / w;

		component_errors[i * 4 + 0] *= iw;
		component_errors[i * 4 + 1] *= iw;
		component_errors[i * 4 + 2] *= iw;
		component_errors[i * 4 + 3] = 0; // radius
	}

	// 计算每个 component 的半径平方
	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int c = components[i];

		float dx = vertex_positions[i].x - component_errors[c * 4 + 0];
		float dy = vertex_positions[i].y - component_errors[c * 4 + 1];
		float dz = vertex_positions[i].z - component_errors[c * 4 + 2];
		float r = dx * dx + dy * dy + dz * dz;

		component_errors[c * 4 + 3] = component_errors[c * 4 + 3] < r ? r : component_errors[c * 4 + 3];
	}

	// 我们已将输出缓冲区用作临时空间，因此需要将结果移动到正确的索引处
	for (size_t i = 0; i < component_count; ++i)
	{
#if TRACE >= 2
		printf("component %d: center %f %f %f, error %e\n", int(i),
		    component_errors[i * 4 + 0], component_errors[i * 4 + 1], component_errors[i * 4 + 2], sqrtf(component_errors[i * 4 + 3]));
#endif
		// 注意：我们保留平方误差，以使其与 quadric error metric 匹配
		component_errors[i] = component_errors[i * 4 + 3];
	}
}

static size_t pruneComponents(unsigned int* indices, size_t index_count, const unsigned int* components, const float* component_errors, size_t component_count, float error_cutoff, float& nexterror)
{
	(void)component_count;

	size_t write = 0;
	float min_error = FLT_MAX;

	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int v0 = indices[i + 0], v1 = indices[i + 1], v2 = indices[i + 2];
		unsigned int c = components[v0];
		assert(c == components[v1] && c == components[v2]);

		if (component_errors[c] > error_cutoff)
		{
			min_error = min_error > component_errors[c] ? component_errors[c] : min_error;

			indices[write + 0] = v0;
			indices[write + 1] = v1;
			indices[write + 2] = v2;
			write += 3;
		}
	}

#if TRACE
	size_t pruned_components = 0;
	for (size_t i = 0; i < component_count; ++i)
		pruned_components += (component_errors[i] >= nexterror && component_errors[i] <= error_cutoff);

	printf("pruned %d triangles in %d components (goal %e); next %e\n", int((index_count - write) / 3), int(pruned_components), sqrtf(error_cutoff), min_error < FLT_MAX ? sqrtf(min_error) : min_error * 2);
#endif

	// 用剩余 components 中的最小误差更新下一个误差
	nexterror = min_error;
	return write;
}

struct CellHasher
{
	const unsigned int* vertex_ids;

	size_t hash(unsigned int i) const
	{
		unsigned int h = vertex_ids[i];

		// MurmurHash2 终结器
		h ^= h >> 13;
		h *= 0x5bd1e995;
		h ^= h >> 15;
		return h;
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		return vertex_ids[lhs] == vertex_ids[rhs];
	}
};

struct IdHasher
{
	size_t hash(unsigned int id) const
	{
		unsigned int h = id;

		// MurmurHash2 终结器
		h ^= h >> 13;
		h *= 0x5bd1e995;
		h ^= h >> 15;
		return h;
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		return lhs == rhs;
	}
};

struct TriangleHasher
{
	const unsigned int* indices;

	size_t hash(unsigned int i) const
	{
		const unsigned int* tri = indices + i * 3;

		// 用于可变形物体碰撞检测的优化空间哈希（Spatial Hashing）
		return (tri[0] * 73856093) ^ (tri[1] * 19349663) ^ (tri[2] * 83492791);
	}

	bool equal(unsigned int lhs, unsigned int rhs) const
	{
		const unsigned int* lt = indices + lhs * 3;
		const unsigned int* rt = indices + rhs * 3;

		return lt[0] == rt[0] && lt[1] == rt[1] && lt[2] == rt[2];
	}
};

static void computeVertexIds(unsigned int* vertex_ids, const Vector3* vertex_positions, const unsigned char* vertex_lock, size_t vertex_count, int grid_size)
{
	assert(grid_size >= 1 && grid_size <= 1024);
	float cell_scale = float(grid_size - 1);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		const Vector3& v = vertex_positions[i];

		int xi = int(v.x * cell_scale + 0.5f);
		int yi = int(v.y * cell_scale + 0.5f);
		int zi = int(v.z * cell_scale + 0.5f);

		if (vertex_lock && (vertex_lock[i] & meshopt_SimplifyVertex_Lock))
			vertex_ids[i] = (1 << 30) | unsigned(i);
		else
			vertex_ids[i] = (xi << 20) | (yi << 10) | zi;
	}
}

static size_t countTriangles(const unsigned int* vertex_ids, const unsigned int* indices, size_t index_count)
{
	size_t result = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int id0 = vertex_ids[indices[i + 0]];
		unsigned int id1 = vertex_ids[indices[i + 1]];
		unsigned int id2 = vertex_ids[indices[i + 2]];

		result += (id0 != id1) & (id0 != id2) & (id1 != id2);
	}

	return result;
}

static size_t fillVertexCells(unsigned int* table, size_t table_size, unsigned int* vertex_cells, const unsigned int* vertex_ids, size_t vertex_count)
{
	CellHasher hasher = {vertex_ids};

	memset(table, -1, table_size * sizeof(unsigned int));

	size_t result = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int* entry = hashLookup2(table, table_size, hasher, unsigned(i), ~0u);

		if (*entry == ~0u)
		{
			*entry = unsigned(i);
			vertex_cells[i] = unsigned(result++);
		}
		else
		{
			vertex_cells[i] = vertex_cells[*entry];
		}
	}

	return result;
}

static size_t countVertexCells(unsigned int* table, size_t table_size, const unsigned int* vertex_ids, size_t vertex_count)
{
	IdHasher hasher;

	memset(table, -1, table_size * sizeof(unsigned int));

	size_t result = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int id = vertex_ids[i];
		unsigned int* entry = hashLookup2(table, table_size, hasher, id, ~0u);

		result += (*entry == ~0u);
		*entry = id;
	}

	return result;
}

static void fillCellQuadrics(Quadric* cell_quadrics, const unsigned int* indices, size_t index_count, const Vector3* vertex_positions, const unsigned int* vertex_cells)
{
	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int i0 = indices[i + 0];
		unsigned int i1 = indices[i + 1];
		unsigned int i2 = indices[i + 2];

		unsigned int c0 = vertex_cells[i0];
		unsigned int c1 = vertex_cells[i1];
		unsigned int c2 = vertex_cells[i2];

		int single_cell = (c0 == c1) & (c0 == c2);

		Quadric Q;
		quadricFromTriangle(Q, vertex_positions[i0], vertex_positions[i1], vertex_positions[i2], single_cell ? 3.f : 1.f);

		if (single_cell)
		{
			quadricAdd(cell_quadrics[c0], Q);
		}
		else
		{
			quadricAdd(cell_quadrics[c0], Q);
			quadricAdd(cell_quadrics[c1], Q);
			quadricAdd(cell_quadrics[c2], Q);
		}
	}
}

static void fillCellReservoirs(Reservoir* cell_reservoirs, size_t cell_count, const Vector3* vertex_positions, const float* vertex_colors, size_t vertex_colors_stride, size_t vertex_count, const unsigned int* vertex_cells)
{
	static const float dummy_color[] = {0.f, 0.f, 0.f};

	size_t vertex_colors_stride_float = vertex_colors_stride / sizeof(float);

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int cell = vertex_cells[i];
		const Vector3& v = vertex_positions[i];
		Reservoir& r = cell_reservoirs[cell];

		const float* color = vertex_colors ? &vertex_colors[i * vertex_colors_stride_float] : dummy_color;

		r.x += v.x;
		r.y += v.y;
		r.z += v.z;
		r.r += color[0];
		r.g += color[1];
		r.b += color[2];
		r.w += 1.f;
	}

	for (size_t i = 0; i < cell_count; ++i)
	{
		Reservoir& r = cell_reservoirs[i];

		float iw = r.w == 0.f ? 0.f : 1.f / r.w;

		r.x *= iw;
		r.y *= iw;
		r.z *= iw;
		r.r *= iw;
		r.g *= iw;
		r.b *= iw;
	}
}

static void fillCellRemap(unsigned int* cell_remap, float* cell_errors, size_t cell_count, const unsigned int* vertex_cells, const Quadric* cell_quadrics, const Vector3* vertex_positions, size_t vertex_count)
{
	memset(cell_remap, -1, cell_count * sizeof(unsigned int));

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int cell = vertex_cells[i];
		float error = quadricError(cell_quadrics[cell], vertex_positions[i]);

		if (cell_remap[cell] == ~0u || cell_errors[cell] > error)
		{
			cell_remap[cell] = unsigned(i);
			cell_errors[cell] = error;
		}
	}
}

static void fillCellRemap(unsigned int* cell_remap, float* cell_errors, size_t cell_count, const unsigned int* vertex_cells, const Reservoir* cell_reservoirs, const Vector3* vertex_positions, const float* vertex_colors, size_t vertex_colors_stride, float color_weight, size_t vertex_count)
{
	static const float dummy_color[] = {0.f, 0.f, 0.f};

	size_t vertex_colors_stride_float = vertex_colors_stride / sizeof(float);

	memset(cell_remap, -1, cell_count * sizeof(unsigned int));

	for (size_t i = 0; i < vertex_count; ++i)
	{
		unsigned int cell = vertex_cells[i];
		const Vector3& v = vertex_positions[i];
		const Reservoir& r = cell_reservoirs[cell];

		const float* color = vertex_colors ? &vertex_colors[i * vertex_colors_stride_float] : dummy_color;

		float pos_error = (v.x - r.x) * (v.x - r.x) + (v.y - r.y) * (v.y - r.y) + (v.z - r.z) * (v.z - r.z);
		float col_error = (color[0] - r.r) * (color[0] - r.r) + (color[1] - r.g) * (color[1] - r.g) + (color[2] - r.b) * (color[2] - r.b);
		float error = pos_error + color_weight * col_error;

		if (cell_remap[cell] == ~0u || cell_errors[cell] > error)
		{
			cell_remap[cell] = unsigned(i);
			cell_errors[cell] = error;
		}
	}
}

static size_t filterTriangles(unsigned int* destination, unsigned int* tritable, size_t tritable_size, const unsigned int* indices, size_t index_count, const unsigned int* vertex_cells, const unsigned int* cell_remap)
{
	TriangleHasher hasher = {destination};

	memset(tritable, -1, tritable_size * sizeof(unsigned int));

	size_t result = 0;

	for (size_t i = 0; i < index_count; i += 3)
	{
		unsigned int c0 = vertex_cells[indices[i + 0]];
		unsigned int c1 = vertex_cells[indices[i + 1]];
		unsigned int c2 = vertex_cells[indices[i + 2]];

		if (c0 != c1 && c0 != c2 && c1 != c2)
		{
			unsigned int a = cell_remap[c0];
			unsigned int b = cell_remap[c1];
			unsigned int c = cell_remap[c2];

			if (b < a && b < c)
			{
				unsigned int t = a;
				a = b, b = c, c = t;
			}
			else if (c < a && c < b)
			{
				unsigned int t = c;
				c = b, b = a, a = t;
			}

			destination[result * 3 + 0] = a;
			destination[result * 3 + 1] = b;
			destination[result * 3 + 2] = c;

			unsigned int* entry = hashLookup2(tritable, tritable_size, hasher, unsigned(result), ~0u);

			if (*entry == ~0u)
				*entry = unsigned(result++);
		}
	}

	return result * 3;
}

static float interpolate(float y, float x0, float y0, float x1, float y1, float x2, float y2)
{
	// 来自《interpolation search 的复仇》论文的三点插值
	float num = (y1 - y) * (x1 - x2) * (x1 - x0) * (y2 - y0);
	float den = (y2 - y) * (x1 - x2) * (y0 - y1) + (y0 - y) * (x1 - x0) * (y1 - y2);
	return x1 + num / den;
}

} // namespace meshopt

// Note: this is only exposed for development purposes; do *not* use
enum
{
	meshopt_SimplifyInternalSolve = 1 << 29,
	meshopt_SimplifyInternalDebug = 1 << 30
};

size_t meshopt_simplifyEdge(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, const float* vertex_attributes_data, size_t vertex_attributes_stride, const float* attribute_weights, size_t attribute_count, const unsigned char* vertex_lock, size_t target_index_count, float target_error, unsigned int options, float* out_result_error)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);
	assert(target_index_count <= index_count);
	assert(target_error >= 0);
	assert((options & ~(meshopt_SimplifyLockBorder | meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute | meshopt_SimplifyPrune | meshopt_SimplifyRegularize | meshopt_SimplifyPermissive | meshopt_SimplifyInternalSolve | meshopt_SimplifyInternalDebug)) == 0);
	assert(vertex_attributes_stride >= attribute_count * sizeof(float) && vertex_attributes_stride <= 256);
	assert(vertex_attributes_stride % sizeof(float) == 0);
	assert(attribute_count <= kMaxAttributes);
	for (size_t i = 0; i < attribute_count; ++i)
		assert(attribute_weights[i] >= 0);

	meshopt_Allocator allocator;

	unsigned int* result = destination;
	if (result != indices)
		memcpy(result, indices, index_count * sizeof(unsigned int));

	// 构建索引重映射并更新 indices/vertex_count，以尽量减少后续工作
	// 注意：因此，误差将相对于子集的范围来计算
	unsigned int* sparse_remap = NULL;
	if (options & meshopt_SimplifySparse)
		sparse_remap = buildSparseRemap(result, index_count, vertex_count, &vertex_count, allocator);

	// 构建邻接信息
	EdgeAdjacency adjacency = {};
	prepareEdgeAdjacency(adjacency, index_count, vertex_count, allocator);
	updateEdgeAdjacency(adjacency, result, index_count, vertex_count, NULL);

	// 构建位置重映射，将每个顶点映射到具有相同位置的顶点
	// wedge 表为每个顶点存储具有相同位置的下一个顶点
	unsigned int* remap = allocator.allocate<unsigned int>(vertex_count);
	unsigned int* wedge = allocator.allocate<unsigned int>(vertex_count);
	buildPositionRemap(remap, wedge, vertex_positions_data, vertex_count, vertex_positions_stride, sparse_remap, allocator);

	// classify vertices; vertex kind determines collapse rules, see kCanCollapse
	unsigned char* vertex_kind = allocator.allocate<unsigned char>(vertex_count);
	unsigned int* loop = allocator.allocate<unsigned int>(vertex_count);
	unsigned int* loopback = allocator.allocate<unsigned int>(vertex_count);
	classifyVertices(vertex_kind, loop, loopback, vertex_count, adjacency, remap, wedge, vertex_lock, sparse_remap, options);

#if TRACE
	size_t unique_positions = 0;
	for (size_t i = 0; i < vertex_count; ++i)
		unique_positions += remap[i] == i;

	printf("position remap: %d vertices => %d positions\n", int(vertex_count), int(unique_positions));

	size_t kinds[Kind_Count] = {};
	for (size_t i = 0; i < vertex_count; ++i)
		kinds[vertex_kind[i]] += remap[i] == i;

	printf("kinds: manifold %d, border %d, seam %d, complex %d, locked %d\n",
	    int(kinds[Kind_Manifold]), int(kinds[Kind_Border]), int(kinds[Kind_Seam]), int(kinds[Kind_Complex]), int(kinds[Kind_Locked]));
#endif

	Vector3* vertex_positions = allocator.allocate<Vector3>(vertex_count);
	float vertex_offset[3] = {};
	float vertex_scale = rescalePositions(vertex_positions, vertex_positions_data, vertex_count, vertex_positions_stride, sparse_remap, vertex_offset);

	float* vertex_attributes = NULL;
	unsigned int attribute_remap[kMaxAttributes];

	if (attribute_count)
	{
		// 重映射属性，仅包含 weight > 0 的属性，以尽量减少 quadrics 的内存/计算开销
		size_t attributes_used = 0;
		for (size_t i = 0; i < attribute_count; ++i)
			if (attribute_weights[i] > 0)
				attribute_remap[attributes_used++] = unsigned(i);

		attribute_count = attributes_used;
		vertex_attributes = allocator.allocate<float>(vertex_count * attribute_count);
		rescaleAttributes(vertex_attributes, vertex_attributes_data, vertex_count, vertex_attributes_stride, attribute_weights, attribute_count, attribute_remap, sparse_remap);
	}

	Quadric* vertex_quadrics = allocator.allocate<Quadric>(vertex_count);
	memset(vertex_quadrics, 0, vertex_count * sizeof(Quadric));

	Quadric* attribute_quadrics = NULL;
	QuadricGrad* attribute_gradients = NULL;
	QuadricGrad* volume_gradients = NULL;

	if (attribute_count)
	{
		attribute_quadrics = allocator.allocate<Quadric>(vertex_count);
		memset(attribute_quadrics, 0, vertex_count * sizeof(Quadric));

		attribute_gradients = allocator.allocate<QuadricGrad>(vertex_count * attribute_count);
		memset(attribute_gradients, 0, vertex_count * attribute_count * sizeof(QuadricGrad));

		if (options & meshopt_SimplifyInternalSolve)
		{
			volume_gradients = allocator.allocate<QuadricGrad>(vertex_count);
			memset(volume_gradients, 0, vertex_count * sizeof(QuadricGrad));
		}
	}

	fillFaceQuadrics(vertex_quadrics, volume_gradients, result, index_count, vertex_positions, remap);
	fillVertexQuadrics(vertex_quadrics, vertex_positions, vertex_count, remap, options);
	fillEdgeQuadrics(vertex_quadrics, result, index_count, vertex_positions, remap, vertex_kind, loop, loopback);

	if (attribute_count)
		fillAttributeQuadrics(attribute_quadrics, attribute_gradients, result, index_count, vertex_positions, vertex_attributes, attribute_count);

	unsigned int* components = NULL;
	float* component_errors = NULL;
	size_t component_count = 0;
	float component_nexterror = 0;

	if (options & meshopt_SimplifyPrune)
	{
		components = allocator.allocate<unsigned int>(vertex_count);
		component_count = buildComponents(components, vertex_count, result, index_count, remap);

		component_errors = allocator.allocate<float>(component_count * 4); // 预留多余空间，供 measureComponents 内部临时使用
		measureComponents(component_errors, component_count, components, vertex_positions, vertex_count);

		component_nexterror = FLT_MAX;
		for (size_t i = 0; i < component_count; ++i)
			component_nexterror = component_nexterror > component_errors[i] ? component_errors[i] : component_nexterror;

#if TRACE
		printf("components: %d (min error %e)\n", int(component_count), sqrtf(component_nexterror));
#endif
	}

#if TRACE
	size_t pass_count = 0;
#endif

	size_t collapse_capacity = boundEdgeCollapses(adjacency, vertex_count, index_count, vertex_kind);

	Collapse* edge_collapses = allocator.allocate<Collapse>(collapse_capacity);
	unsigned int* collapse_order = allocator.allocate<unsigned int>(collapse_capacity);
	unsigned int* collapse_remap = allocator.allocate<unsigned int>(vertex_count);
	unsigned char* collapse_locked = allocator.allocate<unsigned char>(vertex_count);

	size_t result_count = index_count;
	float result_error = 0;
	float vertex_error = 0;

	// target_error input is linear; we need to adjust it to match quadricError units
	float error_scale = (options & meshopt_SimplifyErrorAbsolute) ? vertex_scale : 1.f;
	float error_limit = (target_error * target_error) / (error_scale * error_scale);

	while (result_count > target_index_count)
	{
		// 注意：在整个简化过程中，邻接结构反映正在进行的结果的 welded 拓扑
		updateEdgeAdjacency(adjacency, result, result_count, vertex_count, remap);

		size_t edge_collapse_count = pickEdgeCollapses(edge_collapses, collapse_capacity, result, result_count, remap, vertex_kind, loop, loopback);
		assert(edge_collapse_count <= collapse_capacity);

		// 由于拓扑限制，不再有任何边可以折叠
		if (edge_collapse_count == 0)
			break;

#if TRACE
		printf("pass %d:%c", int(pass_count++), TRACE >= 2 ? '\n' : ' ');
#endif

		rankEdgeCollapses(edge_collapses, edge_collapse_count, vertex_positions, vertex_attributes, vertex_quadrics, attribute_quadrics, attribute_gradients, attribute_count, remap, wedge, vertex_kind, loop, loopback);

		sortEdgeCollapses(collapse_order, edge_collapses, edge_collapse_count);

		size_t triangle_collapse_goal = (result_count - target_index_count) / 3;

		for (size_t i = 0; i < vertex_count; ++i)
			collapse_remap[i] = unsigned(i);

		memset(collapse_locked, 0, vertex_count);

		size_t collapses = performEdgeCollapses(collapse_remap, collapse_locked, edge_collapses, edge_collapse_count, collapse_order, remap, wedge, vertex_kind, loop, loopback, vertex_positions, adjacency, triangle_collapse_goal, error_limit, result_error);

		// 由于达到误差上限或三角形折叠上限，不再有任何边可以折叠
		if (collapses == 0)
			break;

		updateQuadrics(collapse_remap, vertex_count, vertex_quadrics, volume_gradients, attribute_quadrics, attribute_gradients, attribute_count, vertex_positions, remap, vertex_error);

		// 若使用属性，updateQuadrics 将更新顶点误差；若不使用，则 result_error 与 vertex_error 等价
		vertex_error = attribute_count == 0 ? result_error : vertex_error;

		// 注意：我们在边折叠后更新 loops，但此后仍可能存在陈旧的 loop 数据
		// 当具有 loop 边的三角形沿非 loop 边被折叠时，就可能发生这种情况
		// 这可行，因为指向不再连接的顶点的 loop 不会影响折叠逻辑
		remapEdgeLoops(loop, vertex_count, collapse_remap);
		remapEdgeLoops(loopback, vertex_count, collapse_remap);

		result_count = remapIndexBuffer(result, result_count, collapse_remap, remap);

		if ((options & meshopt_SimplifyPrune) && result_count > target_index_count && component_nexterror <= vertex_error)
			result_count = pruneComponents(result, result_count, components, component_errors, component_count, vertex_error, component_nexterror);
	}

	// 此时，component_nexterror 可能是陈旧的：它所引用的 component 可能已通过一系列边折叠被移除
	bool component_nextstale = true;

	// we're done with the regular simplification but we're still short of the target; try pruning more aggressively towards error_limit
	while ((options & meshopt_SimplifyPrune) && result_count > target_index_count && component_nexterror <= error_limit)
	{
#if TRACE
		printf("pass %d: cleanup; ", int(pass_count++));
#endif

		float component_cutoff = component_nexterror * 1.5f < error_limit ? component_nexterror * 1.5f : error_limit;

		// 在增加结果误差的同时，跟踪合格 components 中的最大误差
		float component_maxerror = 0;
		for (size_t i = 0; i < component_count; ++i)
			if (component_errors[i] > component_maxerror && component_errors[i] <= component_cutoff)
				component_maxerror = component_errors[i];

		size_t new_count = pruneComponents(result, result_count, components, component_errors, component_count, component_cutoff, component_nexterror);
		if (new_count == result_count && !component_nextstale)
			break;

		component_nextstale = false; // pruneComponents 保证下一个误差是最新的
		result_count = new_count;
		result_error = result_error < component_maxerror ? component_maxerror : result_error;
		vertex_error = vertex_error < component_maxerror ? component_maxerror : vertex_error;
	}

#if TRACE
	printf("result: %d triangles, error: %e (pos %.3e); total %d passes\n", int(result_count / 3), sqrtf(result_error), sqrtf(vertex_error), int(pass_count));
#endif

	// if solve is requested, update input buffers destructively from internal data
	if (options & meshopt_SimplifyInternalSolve)
	{
		unsigned char* vertex_update = collapse_locked; // 复用为临时空间
		memset(vertex_update, 0, vertex_count);

		// 将 quadric 求解限制在结果中仍被使用的顶点
		for (size_t i = 0; i < result_count; ++i)
		{
			unsigned int v = result[i];

			// 重新计算外部锁定的顶点可能导致浮点漂移
			vertex_update[v] = vertex_kind[v] != Kind_Locked;
		}

		// 边邻接数据可能已陈旧，因为我们在最后一轮边折叠后未更新它
		updateEdgeAdjacency(adjacency, result, result_count, vertex_count, remap);

		solveQuadrics(vertex_positions, vertex_attributes, vertex_count, vertex_quadrics, volume_gradients, attribute_quadrics, attribute_gradients, attribute_count, remap, wedge, adjacency, vertex_kind, vertex_update);

		finalizeVertices(const_cast<float*>(vertex_positions_data), vertex_positions_stride, const_cast<float*>(vertex_attributes_data), vertex_attributes_stride, attribute_weights, attribute_count, vertex_count, vertex_positions, vertex_attributes, sparse_remap, attribute_remap, vertex_scale, vertex_offset, vertex_update);
	}

	// if debug visualization data is requested, fill it instead of index data; for simplicity, this doesn't work with sparsity
	if ((options & meshopt_SimplifyInternalDebug) && !sparse_remap)
	{
		assert(Kind_Count <= 8 && vertex_count < (1 << 28)); // 3 位类型，1 位 loop 标记

		for (size_t i = 0; i < result_count; i += 3)
		{
			unsigned int a = result[i + 0], b = result[i + 1], c = result[i + 2];

			result[i + 0] |= (vertex_kind[a] << 28) | (unsigned(loop[a] == b || loopback[b] == a) << 31);
			result[i + 1] |= (vertex_kind[b] << 28) | (unsigned(loop[b] == c || loopback[c] == b) << 31);
			result[i + 2] |= (vertex_kind[c] << 28) | (unsigned(loop[c] == a || loopback[a] == c) << 31);
		}
	}

	// 将结果索引转换回较大网格的稠密空间
	if (sparse_remap)
		for (size_t i = 0; i < result_count; ++i)
			result[i] = sparse_remap[result[i]];

	// result_error is quadratic; we need to remap it back to linear
	if (out_result_error)
		*out_result_error = sqrtf(result_error) * error_scale;

	return result_count;
}

size_t meshopt_simplify(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, size_t target_index_count, float target_error, unsigned int options, float* out_result_error)
{
	assert((options & meshopt_SimplifyInternalSolve) == 0); // 改用 meshopt_simplifyWithUpdate

	return meshopt_simplifyEdge(destination, indices, index_count, vertex_positions_data, vertex_count, vertex_positions_stride, NULL, 0, NULL, 0, NULL, target_index_count, target_error, options, out_result_error);
}

size_t meshopt_simplifyWithAttributes(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, const float* vertex_attributes_data, size_t vertex_attributes_stride, const float* attribute_weights, size_t attribute_count, const unsigned char* vertex_lock, size_t target_index_count, float target_error, unsigned int options, float* out_result_error)
{
	assert((options & meshopt_SimplifyInternalSolve) == 0); // 改用 meshopt_simplifyWithUpdate

	return meshopt_simplifyEdge(destination, indices, index_count, vertex_positions_data, vertex_count, vertex_positions_stride, vertex_attributes_data, vertex_attributes_stride, attribute_weights, attribute_count, vertex_lock, target_index_count, target_error, options, out_result_error);
}

size_t meshopt_simplifyWithUpdate(unsigned int* indices, size_t index_count, float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, float* vertex_attributes_data, size_t vertex_attributes_stride, const float* attribute_weights, size_t attribute_count, const unsigned char* vertex_lock, size_t target_index_count, float target_error, unsigned int options, float* out_result_error)
{
	return meshopt_simplifyEdge(indices, indices, index_count, vertex_positions_data, vertex_count, vertex_positions_stride, vertex_attributes_data, vertex_attributes_stride, attribute_weights, attribute_count, vertex_lock, target_index_count, target_error, options | meshopt_SimplifyInternalSolve, out_result_error);
}

size_t meshopt_simplifySloppy(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, const unsigned char* vertex_lock, size_t target_index_count, float target_error, float* out_result_error)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);
	assert(target_index_count <= index_count);

	// 我们期望输出中约每顶点 2 个三角形
	size_t target_cell_count = target_index_count / 6;

	meshopt_Allocator allocator;

	Vector3* vertex_positions = allocator.allocate<Vector3>(vertex_count);
	rescalePositions(vertex_positions, vertex_positions_data, vertex_count, vertex_positions_stride);

	// 使用引导式二分搜索找到最佳网格大小
#if TRACE
	printf("source: %d vertices, %d triangles\n", int(vertex_count), int(index_count / 3));
	printf("target: %d cells, %d triangles\n", int(target_cell_count), int(target_index_count / 3));
#endif

	unsigned int* vertex_ids = allocator.allocate<unsigned int>(vertex_count);

	const int kInterpolationPasses = 5;

	// 不变量：min_grid 中三角形数 <= target_count
	int min_grid = int(1.f / (target_error < 1e-3f ? 1e-3f : (target_error < 1.f ? target_error : 1.f)));
	int max_grid = 1025;
	size_t min_triangles = 0;
	size_t max_triangles = index_count / 3;

	// when we're error-limited, we compute the triangle count for the min. size; this accelerates convergence and provides the correct answer when we can't use a larger grid
	if (min_grid > 1 || vertex_lock)
	{
		computeVertexIds(vertex_ids, vertex_positions, vertex_lock, vertex_count, min_grid);
		min_triangles = countTriangles(vertex_ids, indices, index_count);
	}

	// 与其从中间开始，不如猜测一下答案可能是什么！三角形数量通常随网格大小的平方增长……
	int next_grid_size = int(sqrtf(float(target_cell_count)) + 0.5f);

	for (int pass = 0; pass < 10 + kInterpolationPasses; ++pass)
	{
		if (min_triangles >= target_index_count / 3 || max_grid - min_grid <= 1)
			break;

		// 我们限制网格大小的预测范围，以确保搜索收敛
		int grid_size = next_grid_size;
		grid_size = (grid_size <= min_grid) ? min_grid + 1 : (grid_size >= max_grid ? max_grid - 1 : grid_size);

		computeVertexIds(vertex_ids, vertex_positions, vertex_lock, vertex_count, grid_size);
		size_t triangles = countTriangles(vertex_ids, indices, index_count);

#if TRACE
		printf("pass %d (%s): grid size %d, triangles %d, %s\n",
		    pass, (pass == 0) ? "guess" : (pass <= kInterpolationPasses ? "lerp" : "binary"),
		    grid_size, int(triangles),
		    (triangles <= target_index_count / 3) ? "under" : "over");
#endif

		float tip = interpolate(float(size_t(target_index_count / 3)), float(min_grid), float(min_triangles), float(grid_size), float(triangles), float(max_grid), float(max_triangles));

		if (triangles <= target_index_count / 3)
		{
			min_grid = grid_size;
			min_triangles = triangles;
		}
		else
		{
			max_grid = grid_size;
			max_triangles = triangles;
		}

		// 我们先用插值搜索开始——它通常收敛得更快
		// however, interpolation search has a worst case of O(N) so we switch to binary search after a few iterations which converges in O(logN)
		next_grid_size = (pass < kInterpolationPasses) ? int(tip + 0.5f) : (min_grid + max_grid) / 2;
	}

	if (min_triangles == 0)
	{
		if (out_result_error)
			*out_result_error = 1.f;

		return 0;
	}

	// 通过将所有具有相同量化位置的顶点映射到同一单元，构建顶点->单元关联
	size_t table_size = hashBuckets2(vertex_count);
	unsigned int* table = allocator.allocate<unsigned int>(table_size);

	unsigned int* vertex_cells = allocator.allocate<unsigned int>(vertex_count);

	computeVertexIds(vertex_ids, vertex_positions, vertex_lock, vertex_count, min_grid);
	size_t cell_count = fillVertexCells(table, table_size, vertex_cells, vertex_ids, vertex_count);

	// 为每个目标 cell 构建一个 quadric
	Quadric* cell_quadrics = allocator.allocate<Quadric>(cell_count);
	memset(cell_quadrics, 0, cell_count * sizeof(Quadric));

	fillCellQuadrics(cell_quadrics, indices, index_count, vertex_positions, vertex_cells);

	// 对每个目标单元，找到误差最小的顶点
	unsigned int* cell_remap = allocator.allocate<unsigned int>(cell_count);
	float* cell_errors = allocator.allocate<float>(cell_count);

	fillCellRemap(cell_remap, cell_errors, cell_count, vertex_cells, cell_quadrics, vertex_positions, vertex_count);

	// 计算误差
	float result_error = 0.f;

	for (size_t i = 0; i < cell_count; ++i)
		result_error = result_error < cell_errors[i] ? cell_errors[i] : result_error;

	// vertex collapses often result in duplicate triangles; we need a table to filter them out
	size_t tritable_size = hashBuckets2(min_triangles);
	unsigned int* tritable = allocator.allocate<unsigned int>(tritable_size);

	// 注意：这是对 destination 的第一次也是最后一次写入，从而允许 destination 与 indices 别名共存
	size_t write = filterTriangles(destination, tritable, tritable_size, indices, index_count, vertex_cells, cell_remap);

#if TRACE
	printf("result: grid size %d, %d cells, %d triangles (%d unfiltered), error %e\n", min_grid, int(cell_count), int(write / 3), int(min_triangles), sqrtf(result_error));
#endif

	if (out_result_error)
		*out_result_error = sqrtf(result_error);

	return write;
}

size_t meshopt_simplifyPrune(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, float target_error)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);
	assert(target_error >= 0);

	meshopt_Allocator allocator;

	unsigned int* result = destination;
	if (result != indices)
		memcpy(result, indices, index_count * sizeof(unsigned int));

	// 构建位置重映射，将每个顶点映射到具有相同位置的顶点
	unsigned int* remap = allocator.allocate<unsigned int>(vertex_count);
	buildPositionRemap(remap, NULL, vertex_positions_data, vertex_count, vertex_positions_stride, NULL, allocator);

	Vector3* vertex_positions = allocator.allocate<Vector3>(vertex_count);
	rescalePositions(vertex_positions, vertex_positions_data, vertex_count, vertex_positions_stride, NULL);

	unsigned int* components = allocator.allocate<unsigned int>(vertex_count);
	size_t component_count = buildComponents(components, vertex_count, indices, index_count, remap);

	float* component_errors = allocator.allocate<float>(component_count * 4); // 预留多余空间，供 measureComponents 内部临时使用
	measureComponents(component_errors, component_count, components, vertex_positions, vertex_count);

	float component_nexterror = 0;
	size_t result_count = pruneComponents(result, index_count, components, component_errors, component_count, target_error * target_error, component_nexterror);

	return result_count;
}

size_t meshopt_simplifyPoints(unsigned int* destination, const float* vertex_positions_data, size_t vertex_count, size_t vertex_positions_stride, const float* vertex_colors, size_t vertex_colors_stride, float color_weight, size_t target_vertex_count)
{
	using namespace meshopt;

	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);
	assert(vertex_colors_stride == 0 || (vertex_colors_stride >= 12 && vertex_colors_stride <= 256));
	assert(vertex_colors_stride % sizeof(float) == 0);
	assert(vertex_colors == NULL || vertex_colors_stride != 0);
	assert(target_vertex_count <= vertex_count);

	size_t target_cell_count = target_vertex_count;

	if (target_cell_count == 0)
		return 0;

	meshopt_Allocator allocator;

	Vector3* vertex_positions = allocator.allocate<Vector3>(vertex_count);
	rescalePositions(vertex_positions, vertex_positions_data, vertex_count, vertex_positions_stride);

	// 使用引导式二分搜索找到最佳网格大小
#if TRACE
	printf("source: %d vertices\n", int(vertex_count));
	printf("target: %d cells\n", int(target_cell_count));
#endif

	unsigned int* vertex_ids = allocator.allocate<unsigned int>(vertex_count);

	size_t table_size = hashBuckets2(vertex_count);
	unsigned int* table = allocator.allocate<unsigned int>(table_size);

	const int kInterpolationPasses = 5;

	// 不变量：min_grid 中顶点数 <= target_count
	int min_grid = 0;
	int max_grid = 1025;
	size_t min_vertices = 0;
	size_t max_vertices = vertex_count;

	// 与其从中间开始，不如猜测一下答案可能是什么！三角形数量通常随网格大小的平方增长……
	int next_grid_size = int(sqrtf(float(target_cell_count)) + 0.5f);

	for (int pass = 0; pass < 10 + kInterpolationPasses; ++pass)
	{
		assert(min_vertices < target_vertex_count);
		assert(max_grid - min_grid > 1);

		// 我们限制网格大小的预测范围，以确保搜索收敛
		int grid_size = next_grid_size;
		grid_size = (grid_size <= min_grid) ? min_grid + 1 : (grid_size >= max_grid ? max_grid - 1 : grid_size);

		computeVertexIds(vertex_ids, vertex_positions, NULL, vertex_count, grid_size);
		size_t vertices = countVertexCells(table, table_size, vertex_ids, vertex_count);

#if TRACE
		printf("pass %d (%s): grid size %d, vertices %d, %s\n",
		    pass, (pass == 0) ? "guess" : (pass <= kInterpolationPasses ? "lerp" : "binary"),
		    grid_size, int(vertices),
		    (vertices <= target_vertex_count) ? "under" : "over");
#endif

		float tip = interpolate(float(target_vertex_count), float(min_grid), float(min_vertices), float(grid_size), float(vertices), float(max_grid), float(max_vertices));

		if (vertices <= target_vertex_count)
		{
			min_grid = grid_size;
			min_vertices = vertices;
		}
		else
		{
			max_grid = grid_size;
			max_vertices = vertices;
		}

		if (vertices == target_vertex_count || max_grid - min_grid <= 1)
			break;

		// 我们先用插值搜索开始——它通常收敛得更快
		// however, interpolation search has a worst case of O(N) so we switch to binary search after a few iterations which converges in O(logN)
		next_grid_size = (pass < kInterpolationPasses) ? int(tip + 0.5f) : (min_grid + max_grid) / 2;
	}

	if (min_vertices == 0)
		return 0;

	// 通过将所有具有相同量化位置的顶点映射到同一单元，构建顶点->单元关联
	unsigned int* vertex_cells = allocator.allocate<unsigned int>(vertex_count);

	computeVertexIds(vertex_ids, vertex_positions, NULL, vertex_count, min_grid);
	size_t cell_count = fillVertexCells(table, table_size, vertex_cells, vertex_ids, vertex_count);

	// 将点累积到每个目标 cell 的 reservoir 中
	Reservoir* cell_reservoirs = allocator.allocate<Reservoir>(cell_count);
	memset(cell_reservoirs, 0, cell_count * sizeof(Reservoir));

	fillCellReservoirs(cell_reservoirs, cell_count, vertex_positions, vertex_colors, vertex_colors_stride, vertex_count, vertex_cells);

	// 对每个目标单元，找到误差最小的顶点
	unsigned int* cell_remap = allocator.allocate<unsigned int>(cell_count);
	float* cell_errors = allocator.allocate<float>(cell_count);

	// 我们缩放颜色权重，使其与位置处于同一量级，这样误差相加才有意义
	float color_weight_scaled = color_weight * (min_grid == 1 ? 1.f : 1.f / (min_grid - 1));

	fillCellRemap(cell_remap, cell_errors, cell_count, vertex_cells, cell_reservoirs, vertex_positions, vertex_colors, vertex_colors_stride, color_weight_scaled * color_weight_scaled, vertex_count);

	// 将结果复制到输出
	assert(cell_count <= target_vertex_count);
	memcpy(destination, cell_remap, sizeof(unsigned int) * cell_count);

#if TRACE
	// 计算误差
	float result_error = 0.f;

	for (size_t i = 0; i < cell_count; ++i)
		result_error = result_error < cell_errors[i] ? cell_errors[i] : result_error;

	printf("result: %d cells, %e error\n", int(cell_count), sqrtf(result_error));
#endif

	return cell_count;
}

float meshopt_simplifyScale(const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	using namespace meshopt;

	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

	float extent = rescalePositions(NULL, vertex_positions, vertex_count, vertex_positions_stride);

	return extent;
}

MESHOPTIMIZER_VOXEL_NAMESPACE_END