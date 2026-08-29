// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

MESHOPTIMIZER_VOXEL_NAMESPACE_BEGIN

// 此作品基于：
// Francine Evans、Steven Skiena 和 Amitabh Varshney。《优化用于快速渲染的三角形带》，1996
namespace meshopt
{

static unsigned int findStripFirst(const unsigned int buffer[][3], unsigned int buffer_size, const unsigned char* valence)
{
	unsigned int index = 0;
	unsigned int iv = ~0u;

	for (size_t i = 0; i < buffer_size; ++i)
	{
		unsigned char va = valence[buffer[i][0]], vb = valence[buffer[i][1]], vc = valence[buffer[i][2]];
		unsigned int v = (va < vb && va < vc) ? va : (vb < vc ? vb : vc);

		if (v < iv)
		{
			index = unsigned(i);
			iv = v;
		}
	}

	return index;
}

static int findStripNext(const unsigned int buffer[][3], unsigned int buffer_size, unsigned int e0, unsigned int e1)
{
	for (size_t i = 0; i < buffer_size; ++i)
	{
		unsigned int a = buffer[i][0], b = buffer[i][1], c = buffer[i][2];

		if (e0 == a && e1 == b)
			return (int(i) << 2) | 2;
		else if (e0 == b && e1 == c)
			return (int(i) << 2) | 0;
		else if (e0 == c && e1 == a)
			return (int(i) << 2) | 1;
	}

	return -1;
}

} // namespace meshopt

size_t meshopt_stripify(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int restart_index)
{
	assert(destination != indices);
	assert(index_count % 3 == 0);

	using namespace meshopt;

	meshopt_Allocator allocator;

	const size_t buffer_capacity = 8;

	unsigned int buffer[buffer_capacity][3] = {};
	unsigned int buffer_size = 0;

	size_t index_offset = 0;

	unsigned int strip[2] = {};
	unsigned int parity = 0;

	size_t strip_size = 0;

	// compute vertex valence; this is used to prioritize starting triangle for strips
	// note: we use 8-bit counters for performance; for outlier vertices the valence is incorrect but that just affects the heuristic
	unsigned char* valence = allocator.allocate<unsigned char>(vertex_count);
	memset(valence, 0, vertex_count);

	for (size_t i = 0; i < index_count; ++i)
	{
		unsigned int index = indices[i];
		assert(index < vertex_count);

		valence[index]++;
	}

	int next = -1;

	while (buffer_size > 0 || index_offset < index_count)
	{
		assert(next < 0 || (size_t(next >> 2) < buffer_size && (next & 3) < 3));

		// 填充三角形缓冲区
		while (buffer_size < buffer_capacity && index_offset < index_count)
		{
			buffer[buffer_size][0] = indices[index_offset + 0];
			buffer[buffer_size][1] = indices[index_offset + 1];
			buffer[buffer_size][2] = indices[index_offset + 2];

			buffer_size++;
			index_offset += 3;
		}

		assert(buffer_size > 0);

		if (next >= 0)
		{
			unsigned int i = next >> 2;
			unsigned int a = buffer[i][0], b = buffer[i][1], c = buffer[i][2];
			unsigned int v = buffer[i][next & 3];

			// 从 buffer 中有序移除
			memmove(buffer[i], buffer[i + 1], (buffer_size - i - 1) * sizeof(buffer[0]));
			buffer_size--;

			// 更新顶点度数（valence），用于条带起始启发式
			valence[a]--;
			valence[b]--;
			valence[c]--;

			// find next triangle (note that edge order flips on every iteration)
			// 在某些情况下，我们需要执行一次交换以选取不同的出向三角形边
			// for [a b c], the default strip edge is [b c], but we might want to use [a c]
			int cont = findStripNext(buffer, buffer_size, parity ? strip[1] : v, parity ? v : strip[1]);
			int swap = cont < 0 ? findStripNext(buffer, buffer_size, parity ? v : strip[0], parity ? strip[0] : v) : -1;

			if (cont < 0 && swap >= 0)
			{
				// [a b c] => [a b a c]
				destination[strip_size++] = strip[0];
				destination[strip_size++] = v;

				// 下一条 strip 具有相同的绕序
				// ? a b => b a v
				strip[1] = v;

				next = swap;
			}
			else
			{
				// 发出 strip 中的下一个顶点
				destination[strip_size++] = v;

				// 下一条 strip 的绕序已翻转
				strip[0] = strip[1];
				strip[1] = v;
				parity ^= 1;

				next = cont;
			}
		}
		else
		{
			// if we didn't find anything, we need to find the next new triangle
			// 我们使用启发式算法以最大化 strip 长度
			unsigned int i = findStripFirst(buffer, buffer_size, valence);
			unsigned int a = buffer[i][0], b = buffer[i][1], c = buffer[i][2];

			// 从 buffer 中有序移除
			memmove(buffer[i], buffer[i + 1], (buffer_size - i - 1) * sizeof(buffer[0]));
			buffer_size--;

			// 更新顶点度数（valence），用于条带起始启发式
			valence[a]--;
			valence[b]--;
			valence[c]--;

			// 我们需要预先旋转三角形，以便在下一轮迭代中能在现有缓冲区中找到匹配
			int ea = findStripNext(buffer, buffer_size, c, b);
			int eb = findStripNext(buffer, buffer_size, a, c);
			int ec = findStripNext(buffer, buffer_size, b, a);

			// in some cases we can have several matching edges; since we can pick any edge, we pick the one with the smallest
			// 缓冲区中的三角形索引。这可减少条带化对 ACMR 的影响，另外——出于尚不明确的原因
			// ——略能提高条带化效率
			int mine = INT_MAX;
			mine = (ea >= 0 && mine > ea) ? ea : mine;
			mine = (eb >= 0 && mine > eb) ? eb : mine;
			mine = (ec >= 0 && mine > ec) ? ec : mine;

			if (ea == mine)
			{
				// 保留 abc
				next = ea;
			}
			else if (eb == mine)
			{
				// abc -> bca
				unsigned int t = a;
				a = b, b = c, c = t;

				next = eb;
			}
			else if (ec == mine)
			{
				// abc -> cab
				unsigned int t = c;
				c = b, b = a, a = t;

				next = ec;
			}

			if (restart_index)
			{
				if (strip_size)
					destination[strip_size++] = restart_index;

				destination[strip_size++] = a;
				destination[strip_size++] = b;
				destination[strip_size++] = c;

				// 新 strip 始终以相同的边绕序开始
				strip[0] = b;
				strip[1] = c;
				parity = 1;
			}
			else
			{
				if (strip_size)
				{
					// 使用退化三角形连接上一条 strip
					destination[strip_size++] = strip[1];
					destination[strip_size++] = a;
				}

				// 注意：我们可能需要根据奇偶性翻转已发出的三角形
				// 我们最终总是以出向边 "cb" 结束
				unsigned int e0 = parity ? c : b;
				unsigned int e1 = parity ? b : c;

				destination[strip_size++] = a;
				destination[strip_size++] = e0;
				destination[strip_size++] = e1;

				strip[0] = e0;
				strip[1] = e1;
				parity ^= 1;
			}
		}
	}

	return strip_size;
}

size_t meshopt_stripifyBound(size_t index_count)
{
	assert(index_count % 3 == 0);

	// 无重启的最坏情况是每个三角形 2 个退化索引和 3 个索引
	// 有重启的最坏情况是每个三角形 1 个重启索引和 3 个索引
	return (index_count / 3) * 5;
}

size_t meshopt_unstripify(unsigned int* destination, const unsigned int* indices, size_t index_count, unsigned int restart_index)
{
	assert(destination != indices);

	size_t offset = 0;
	size_t start = 0;

	for (size_t i = 0; i < index_count; ++i)
	{
		if (restart_index && indices[i] == restart_index)
		{
			start = i + 1;
		}
		else if (i - start >= 2)
		{
			unsigned int a = indices[i - 2], b = indices[i - 1], c = indices[i];

			// 为奇数三角形翻转绕序
			if ((i - start) & 1)
			{
				unsigned int t = a;
				a = b, b = t;
			}

			// 尽管我们使用重启索引，strip 交换仍会产生退化三角形，因此跳过它们
			if (a != b && a != c && b != c)
			{
				destination[offset + 0] = a;
				destination[offset + 1] = b;
				destination[offset + 2] = c;
				offset += 3;
			}
		}
	}

	return offset;
}

size_t meshopt_unstripifyBound(size_t index_count)
{
	assert(index_count == 0 || index_count >= 3);

	return (index_count == 0) ? 0 : (index_count - 2) * 3;
}

MESHOPTIMIZER_VOXEL_NAMESPACE_END