// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

MESHOPTIMIZER_VOXEL_NAMESPACE_BEGIN

// 此作品基于：
// Tom Forsyth。《线性速度顶点缓存优化》，2006
// Pedro Sander、Diego Nehab 和 Joshua Barczak。用于顶点局部性与减少过度绘制的高速三角形重排。2007
namespace meshopt
{

const size_t kCacheSizeMax = 16;
const size_t kValenceMax = 8;

struct VertexScoreTable
{
	float cache[1 + kCacheSizeMax];
	float live[1 + kValenceMax];
};

// 经过调优，以最小化具有与 NVidia 和 AMD 相似缓存特性的 GPU 的 ACMR
static const VertexScoreTable kVertexScoreTable = {
    {0.f, 0.779f, 0.791f, 0.789f, 0.981f, 0.843f, 0.726f, 0.847f, 0.882f, 0.867f, 0.799f, 0.642f, 0.613f, 0.600f, 0.568f, 0.372f, 0.234f},
    {0.f, 0.995f, 0.713f, 0.450f, 0.404f, 0.059f, 0.005f, 0.147f, 0.006f},
};

// 经过调优，以最小化编码后的索引缓冲区大小
static const VertexScoreTable kVertexScoreTableStrip = {
    {0.f, 1.000f, 1.000f, 1.000f, 0.453f, 0.561f, 0.490f, 0.459f, 0.179f, 0.526f, 0.000f, 0.227f, 0.184f, 0.490f, 0.112f, 0.050f, 0.131f},
    {0.f, 0.956f, 0.786f, 0.577f, 0.558f, 0.618f, 0.549f, 0.499f, 0.489f},
};

struct TriangleAdjacency
{
	unsigned int* counts;
	unsigned int* offsets;
	unsigned int* data;
};

static void buildTriangleAdjacency(TriangleAdjacency& adjacency, const unsigned int* indices, size_t index_count, size_t vertex_count, meshopt_Allocator& allocator)
{
	size_t face_count = index_count / 3;

	// 分配数组
	adjacency.counts = allocator.allocate<unsigned int>(vertex_count);
	adjacency.offsets = allocator.allocate<unsigned int>(vertex_count);
	adjacency.data = allocator.allocate<unsigned int>(index_count);

	// 填充三角形计数
	memset(adjacency.counts, 0, vertex_count * sizeof(unsigned int));

	for (size_t i = 0; i < index_count; ++i)
	{
		assert(indices[i] < vertex_count);

		adjacency.counts[indices[i]]++;
	}

	// 填充偏移表
	unsigned int offset = 0;

	for (size_t i = 0; i < vertex_count; ++i)
	{
		adjacency.offsets[i] = offset;
		offset += adjacency.counts[i];
	}

	assert(offset == index_count);

	// 填充三角形数据
	for (size_t i = 0; i < face_count; ++i)
	{
		unsigned int a = indices[i * 3 + 0], b = indices[i * 3 + 1], c = indices[i * 3 + 2];

		adjacency.data[adjacency.offsets[a]++] = unsigned(i);
		adjacency.data[adjacency.offsets[b]++] = unsigned(i);
		adjacency.data[adjacency.offsets[c]++] = unsigned(i);
	}

	// 修复被上一轮处理扰乱的偏移
	for (size_t i = 0; i < vertex_count; ++i)
	{
		assert(adjacency.offsets[i] >= adjacency.counts[i]);

		adjacency.offsets[i] -= adjacency.counts[i];
	}
}

static unsigned int getNextVertexDeadEnd(const unsigned int* dead_end, unsigned int& dead_end_top, unsigned int& input_cursor, const unsigned int* live_triangles, size_t vertex_count)
{
	// 检查死胡同栈
	while (dead_end_top)
	{
		unsigned int vertex = dead_end[--dead_end_top];

		if (live_triangles[vertex] > 0)
			return vertex;
	}

	// 输入顺序
	while (input_cursor < vertex_count)
	{
		if (live_triangles[input_cursor] > 0)
			return input_cursor;

		++input_cursor;
	}

	return ~0u;
}

static unsigned int getNextVertexNeighbor(const unsigned int* next_candidates_begin, const unsigned int* next_candidates_end, const unsigned int* live_triangles, const unsigned int* cache_timestamps, unsigned int timestamp, unsigned int cache_size)
{
	unsigned int best_candidate = ~0u;
	int best_priority = -1;

	for (const unsigned int* next_candidate = next_candidates_begin; next_candidate != next_candidates_end; ++next_candidate)
	{
		unsigned int vertex = *next_candidate;

		// 否则我们无需处理它
		if (live_triangles[vertex] > 0)
		{
			int priority = 0;

			// 扇出后它是否仍在缓存中？
			if (2 * live_triangles[vertex] + timestamp - cache_timestamps[vertex] <= cache_size)
			{
				priority = timestamp - cache_timestamps[vertex]; // 在缓存中的位置
			}

			if (priority > best_priority)
			{
				best_candidate = vertex;
				best_priority = priority;
			}
		}
	}

	return best_candidate;
}

static float vertexScore(const VertexScoreTable* table, int cache_position, unsigned int live_triangles)
{
	assert(cache_position >= -1 && cache_position < int(kCacheSizeMax));

	unsigned int live_triangles_clamped = live_triangles < kValenceMax ? live_triangles : kValenceMax;

	return table->cache[1 + cache_position] + table->live[live_triangles_clamped];
}

static unsigned int getNextTriangleDeadEnd(unsigned int& input_cursor, const unsigned char* emitted_flags, size_t face_count)
{
	// 输入顺序
	while (input_cursor < face_count)
	{
		if (!emitted_flags[input_cursor])
			return input_cursor;

		++input_cursor;
	}

	return ~0u;
}

} // namespace meshopt

void meshopt_optimizeVertexCacheTable(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, const meshopt::VertexScoreTable* table)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);

	meshopt_Allocator allocator;

	// 对空网格的防护
	if (index_count == 0 || vertex_count == 0)
		return;

	// 支持就地（in-place）优化
	if (destination == indices)
	{
		unsigned int* indices_copy = allocator.allocate<unsigned int>(index_count);
		memcpy(indices_copy, indices, index_count * sizeof(unsigned int));
		indices = indices_copy;
	}

	unsigned int cache_size = 16;
	assert(cache_size <= kCacheSizeMax);

	size_t face_count = index_count / 3;

	// 构建邻接信息
	TriangleAdjacency adjacency = {};
	buildTriangleAdjacency(adjacency, indices, index_count, vertex_count, allocator);

	// live triangle counts; note, we alias adjacency.counts as we remove triangles after emitting them so the counts always match
	unsigned int* live_triangles = adjacency.counts;

	// 已发射标志
	unsigned char* emitted_flags = allocator.allocate<unsigned char>(face_count);
	memset(emitted_flags, 0, face_count);

	// 计算初始顶点得分
	float* vertex_scores = allocator.allocate<float>(vertex_count);

	for (size_t i = 0; i < vertex_count; ++i)
		vertex_scores[i] = vertexScore(table, -1, live_triangles[i]);

	// 计算三角形得分
	float* triangle_scores = allocator.allocate<float>(face_count);

	for (size_t i = 0; i < face_count; ++i)
	{
		unsigned int a = indices[i * 3 + 0];
		unsigned int b = indices[i * 3 + 1];
		unsigned int c = indices[i * 3 + 2];

		triangle_scores[i] = vertex_scores[a] + vertex_scores[b] + vertex_scores[c];
	}

	unsigned int cache_holder[2 * (kCacheSizeMax + 4)];
	unsigned int* cache = cache_holder;
	unsigned int* cache_new = cache_holder + kCacheSizeMax + 4;
	size_t cache_count = 0;

	unsigned int current_triangle = 0;
	unsigned int input_cursor = 1;

	unsigned int output_triangle = 0;

	while (current_triangle != ~0u)
	{
		assert(output_triangle < face_count);

		unsigned int a = indices[current_triangle * 3 + 0];
		unsigned int b = indices[current_triangle * 3 + 1];
		unsigned int c = indices[current_triangle * 3 + 2];

		// 输出索引
		destination[output_triangle * 3 + 0] = a;
		destination[output_triangle * 3 + 1] = b;
		destination[output_triangle * 3 + 2] = c;
		output_triangle++;

		// 更新已发射标志
		emitted_flags[current_triangle] = true;
		triangle_scores[current_triangle] = 0;

		// 新三角形
		size_t cache_write = 0;
		cache_new[cache_write++] = a;
		cache_new[cache_write++] = b;
		cache_new[cache_write++] = c;

		// 旧三角形
		for (size_t i = 0; i < cache_count; ++i)
		{
			unsigned int index = cache[i];

			cache_new[cache_write] = index;
			cache_write += (index != a) & (index != b) & (index != c);
		}

		unsigned int* cache_temp = cache;
		cache = cache_new, cache_new = cache_temp;
		cache_count = cache_write > cache_size ? cache_size : cache_write;

		// 从邻接数据中移除已发射的三角形
		// 这能确保我们在后续迭代中花费更少时间遍历这些列表
		// 活跃三角形计数会作为这些调整的副产品被更新
		for (size_t k = 0; k < 3; ++k)
		{
			unsigned int index = indices[current_triangle * 3 + k];

			unsigned int* neighbors = &adjacency.data[0] + adjacency.offsets[index];
			size_t neighbors_size = adjacency.counts[index];

			for (size_t i = 0; i < neighbors_size; ++i)
			{
				unsigned int tri = neighbors[i];

				if (tri == current_triangle)
				{
					neighbors[i] = neighbors[neighbors_size - 1];
					adjacency.counts[index]--;
					break;
				}
			}
		}

		unsigned int best_triangle = ~0u;
		float best_score = 0;

		// 更新缓存位置、顶点得分和三角形得分，并寻找下一个最佳三角形
		for (size_t i = 0; i < cache_write; ++i)
		{
			unsigned int index = cache[i];

			// 若我们永远不会使用该顶点，则无需更新得分
			if (adjacency.counts[index] == 0)
				continue;

			int cache_position = i >= cache_size ? -1 : int(i);

			// 更新顶点得分
			float score = vertexScore(table, cache_position, live_triangles[index]);
			float score_diff = score - vertex_scores[index];

			vertex_scores[index] = score;

			// 更新顶点所关联三角形的得分
			const unsigned int* neighbors_begin = &adjacency.data[0] + adjacency.offsets[index];
			const unsigned int* neighbors_end = neighbors_begin + adjacency.counts[index];

			for (const unsigned int* it = neighbors_begin; it != neighbors_end; ++it)
			{
				unsigned int tri = *it;
				assert(!emitted_flags[tri]);

				float tri_score = triangle_scores[tri] + score_diff;
				assert(tri_score > 0);

				best_triangle = best_score < tri_score ? tri : best_triangle;
				best_score = best_score < tri_score ? tri_score : best_score;

				triangle_scores[tri] = tri_score;
			}
		}

		// 若遇到死胡同，则按顺序遍历输入三角形
		current_triangle = best_triangle;

		if (current_triangle == ~0u)
		{
			current_triangle = getNextTriangleDeadEnd(input_cursor, &emitted_flags[0], face_count);
		}
	}

	assert(input_cursor == face_count);
	assert(output_triangle == face_count);
}

void meshopt_optimizeVertexCache(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count)
{
	meshopt_optimizeVertexCacheTable(destination, indices, index_count, vertex_count, &meshopt::kVertexScoreTable);
}

void meshopt_optimizeVertexCacheStrip(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count)
{
	meshopt_optimizeVertexCacheTable(destination, indices, index_count, vertex_count, &meshopt::kVertexScoreTableStrip);
}

void meshopt_optimizeVertexCacheFifo(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(cache_size >= 3);

	meshopt_Allocator allocator;

	// 对空网格的防护
	if (index_count == 0 || vertex_count == 0)
		return;

	// 支持就地（in-place）优化
	if (destination == indices)
	{
		unsigned int* indices_copy = allocator.allocate<unsigned int>(index_count);
		memcpy(indices_copy, indices, index_count * sizeof(unsigned int));
		indices = indices_copy;
	}

	size_t face_count = index_count / 3;

	// 构建邻接信息
	TriangleAdjacency adjacency = {};
	buildTriangleAdjacency(adjacency, indices, index_count, vertex_count, allocator);

	// 活跃三角形计数
	unsigned int* live_triangles = allocator.allocate<unsigned int>(vertex_count);
	memcpy(live_triangles, adjacency.counts, vertex_count * sizeof(unsigned int));

	// 缓存时间戳
	unsigned int* cache_timestamps = allocator.allocate<unsigned int>(vertex_count);
	memset(cache_timestamps, 0, vertex_count * sizeof(unsigned int));

	// 死胡同栈
	unsigned int* dead_end = allocator.allocate<unsigned int>(index_count);
	unsigned int dead_end_top = 0;

	// 已发射标志
	unsigned char* emitted_flags = allocator.allocate<unsigned char>(face_count);
	memset(emitted_flags, 0, face_count);

	unsigned int current_vertex = 0;

	unsigned int timestamp = cache_size + 1;
	unsigned int input_cursor = 1; // 遇死胡同时从中重新开始的顶点

	unsigned int output_triangle = 0;

	while (current_vertex != ~0u)
	{
		const unsigned int* next_candidates_begin = &dead_end[0] + dead_end_top;

		// 发出所有顶点邻接
		const unsigned int* neighbors_begin = &adjacency.data[0] + adjacency.offsets[current_vertex];
		const unsigned int* neighbors_end = neighbors_begin + adjacency.counts[current_vertex];

		for (const unsigned int* it = neighbors_begin; it != neighbors_end; ++it)
		{
			unsigned int triangle = *it;

			if (!emitted_flags[triangle])
			{
				unsigned int a = indices[triangle * 3 + 0], b = indices[triangle * 3 + 1], c = indices[triangle * 3 + 2];

				// 输出索引
				destination[output_triangle * 3 + 0] = a;
				destination[output_triangle * 3 + 1] = b;
				destination[output_triangle * 3 + 2] = c;
				output_triangle++;

				// 更新死胡同栈
				dead_end[dead_end_top + 0] = a;
				dead_end[dead_end_top + 1] = b;
				dead_end[dead_end_top + 2] = c;
				dead_end_top += 3;

				// 更新活跃三角形计数
				live_triangles[a]--;
				live_triangles[b]--;
				live_triangles[c]--;

				// 更新缓存信息
				// if vertex is not in cache, put it in cache
				if (timestamp - cache_timestamps[a] > cache_size)
					cache_timestamps[a] = timestamp++;

				if (timestamp - cache_timestamps[b] > cache_size)
					cache_timestamps[b] = timestamp++;

				if (timestamp - cache_timestamps[c] > cache_size)
					cache_timestamps[c] = timestamp++;

				// 更新已发射标志
				emitted_flags[triangle] = true;
			}
		}

		// 下一个候选就是刚刚推入死胡同栈的那些
		const unsigned int* next_candidates_end = &dead_end[0] + dead_end_top;

		// 获取下一个顶点
		current_vertex = getNextVertexNeighbor(next_candidates_begin, next_candidates_end, &live_triangles[0], &cache_timestamps[0], timestamp, cache_size);

		if (current_vertex == ~0u)
		{
			current_vertex = getNextVertexDeadEnd(&dead_end[0], dead_end_top, input_cursor, &live_triangles[0], vertex_count);
		}
	}

	assert(output_triangle == face_count);
}

MESHOPTIMIZER_VOXEL_NAMESPACE_END