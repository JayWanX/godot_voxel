// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <math.h>
#include <string.h>

MESHOPTIMIZER_VOXEL_NAMESPACE_BEGIN

// 此作品基于：
// Pedro Sander、Diego Nehab 和 Joshua Barczak。用于顶点局部性与减少过度绘制的高速三角形重排。2007
namespace meshopt
{

static void calculateSortData(float* sort_data, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, const unsigned int* clusters, size_t cluster_count)
{
	size_t vertex_stride_float = vertex_positions_stride / sizeof(float);

	float mesh_centroid[3] = {};

	for (size_t i = 0; i < vertex_count; ++i)
	{
		const float* p = vertex_positions + vertex_stride_float * i;

		mesh_centroid[0] += p[0];
		mesh_centroid[1] += p[1];
		mesh_centroid[2] += p[2];
	}

	mesh_centroid[0] /= float(vertex_count);
	mesh_centroid[1] /= float(vertex_count);
	mesh_centroid[2] /= float(vertex_count);

	for (size_t cluster = 0; cluster < cluster_count; ++cluster)
	{
		size_t cluster_begin = clusters[cluster] * 3;
		size_t cluster_end = (cluster + 1 < cluster_count) ? clusters[cluster + 1] * 3 : index_count;
		assert(cluster_begin < cluster_end);

		float cluster_area = 0;
		float cluster_centroid[3] = {};
		float cluster_normal[3] = {};

		for (size_t i = cluster_begin; i < cluster_end; i += 3)
		{
			const float* p0 = vertex_positions + vertex_stride_float * indices[i + 0];
			const float* p1 = vertex_positions + vertex_stride_float * indices[i + 1];
			const float* p2 = vertex_positions + vertex_stride_float * indices[i + 2];

			float p10[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
			float p20[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};

			float normalx = p10[1] * p20[2] - p10[2] * p20[1];
			float normaly = p10[2] * p20[0] - p10[0] * p20[2];
			float normalz = p10[0] * p20[1] - p10[1] * p20[0];

			float area = sqrtf(normalx * normalx + normaly * normaly + normalz * normalz);

			cluster_centroid[0] += (p0[0] + p1[0] + p2[0]) * (area / 3);
			cluster_centroid[1] += (p0[1] + p1[1] + p2[1]) * (area / 3);
			cluster_centroid[2] += (p0[2] + p1[2] + p2[2]) * (area / 3);
			cluster_normal[0] += normalx;
			cluster_normal[1] += normaly;
			cluster_normal[2] += normalz;
			cluster_area += area;
		}

		float inv_cluster_area = cluster_area == 0 ? 0 : 1 / cluster_area;

		cluster_centroid[0] *= inv_cluster_area;
		cluster_centroid[1] *= inv_cluster_area;
		cluster_centroid[2] *= inv_cluster_area;

		float cluster_normal_length = sqrtf(cluster_normal[0] * cluster_normal[0] + cluster_normal[1] * cluster_normal[1] + cluster_normal[2] * cluster_normal[2]);
		float inv_cluster_normal_length = cluster_normal_length == 0 ? 0 : 1 / cluster_normal_length;

		cluster_normal[0] *= inv_cluster_normal_length;
		cluster_normal[1] *= inv_cluster_normal_length;
		cluster_normal[2] *= inv_cluster_normal_length;

		float centroid_vector[3] = {cluster_centroid[0] - mesh_centroid[0], cluster_centroid[1] - mesh_centroid[1], cluster_centroid[2] - mesh_centroid[2]};

		sort_data[cluster] = centroid_vector[0] * cluster_normal[0] + centroid_vector[1] * cluster_normal[1] + centroid_vector[2] * cluster_normal[2];
	}
}

static void calculateSortOrderRadix(unsigned int* sort_order, const float* sort_data, unsigned short* sort_keys, size_t cluster_count)
{
	// 计算排序数据的边界并使用定点 snorm 重新归一化
	float sort_data_max = 1e-3f;

	for (size_t i = 0; i < cluster_count; ++i)
	{
		float dpa = fabsf(sort_data[i]);

		sort_data_max = (sort_data_max < dpa) ? dpa : sort_data_max;
	}

	const int sort_bits = 11;

	for (size_t i = 0; i < cluster_count; ++i)
	{
		// 注意：我们翻转分布，因为高点积应排在前面
		float sort_key = 0.5f - 0.5f * (sort_data[i] / sort_data_max);

		sort_keys[i] = meshopt_quantizeUnorm(sort_key, sort_bits) & ((1 << sort_bits) - 1);
	}

	// 为计数排序填充直方图
	unsigned int histogram[1 << sort_bits];
	memset(histogram, 0, sizeof(histogram));

	for (size_t i = 0; i < cluster_count; ++i)
	{
		histogram[sort_keys[i]]++;
	}

	// 基于直方图数据计算偏移
	size_t histogram_sum = 0;

	for (size_t i = 0; i < 1 << sort_bits; ++i)
	{
		size_t count = histogram[i];
		histogram[i] = unsigned(histogram_sum);
		histogram_sum += count;
	}

	assert(histogram_sum == cluster_count);

	// 基于偏移计算排序顺序
	for (size_t i = 0; i < cluster_count; ++i)
	{
		sort_order[histogram[sort_keys[i]]++] = unsigned(i);
	}
}

static unsigned int updateCache(unsigned int a, unsigned int b, unsigned int c, unsigned int cache_size, unsigned int* cache_timestamps, unsigned int& timestamp)
{
	unsigned int cache_misses = 0;

	// if vertex is not in cache, put it in cache
	if (timestamp - cache_timestamps[a] > cache_size)
	{
		cache_timestamps[a] = timestamp++;
		cache_misses++;
	}

	if (timestamp - cache_timestamps[b] > cache_size)
	{
		cache_timestamps[b] = timestamp++;
		cache_misses++;
	}

	if (timestamp - cache_timestamps[c] > cache_size)
	{
		cache_timestamps[c] = timestamp++;
		cache_misses++;
	}

	return cache_misses;
}

static size_t generateHardBoundaries(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size, unsigned int* cache_timestamps)
{
	memset(cache_timestamps, 0, vertex_count * sizeof(unsigned int));

	unsigned int timestamp = cache_size + 1;

	size_t face_count = index_count / 3;

	size_t result = 0;

	for (size_t i = 0; i < face_count; ++i)
	{
		unsigned int m = updateCache(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2], cache_size, &cache_timestamps[0], timestamp);

		// 当三个顶点都不在缓存中时，通常可相对安全地假定这是网格中的新补丁
		// that is disjoint from previous vertices; sometimes it might come back to reference existing vertices but that frequently
		// 提示顶点缓存优化算法存在低效
		// 通常第一个三角形有 3 次未命中，除非它是退化的 - 因此我们确保第一个簇总是从 0 开始
		if (i == 0 || m == 3)
		{
			destination[result++] = unsigned(i);
		}
	}

	assert(result <= index_count / 3);

	return result;
}

static size_t generateSoftBoundaries(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, const unsigned int* clusters, size_t cluster_count, unsigned int cache_size, float threshold, unsigned int* cache_timestamps)
{
	memset(cache_timestamps, 0, vertex_count * sizeof(unsigned int));

	unsigned int timestamp = 0;

	size_t result = 0;

	for (size_t it = 0; it < cluster_count; ++it)
	{
		size_t start = clusters[it];
		size_t end = (it + 1 < cluster_count) ? clusters[it + 1] : index_count / 3;
		assert(start < end);

		// 重置缓存
		timestamp += cache_size + 1;

		// 度量簇的 ACMR
		unsigned int cluster_misses = 0;

		for (size_t i = start; i < end; ++i)
		{
			unsigned int m = updateCache(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2], cache_size, &cache_timestamps[0], timestamp);

			cluster_misses += m;
		}

		float cluster_threshold = threshold * (float(cluster_misses) / float(end - start));

		// 第一个簇总是从硬簇边界开始
		destination[result++] = unsigned(start);

		// 重置缓存
		timestamp += cache_size + 1;

		unsigned int running_misses = 0;
		unsigned int running_faces = 0;

		for (size_t i = start; i < end; ++i)
		{
			unsigned int m = updateCache(indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2], cache_size, &cache_timestamps[0], timestamp);

			running_misses += m;
			running_faces += 1;

			if (float(running_misses) / float(running_faces) <= cluster_threshold)
			{
				// 当前三角形已达到目标 ACMR，因此需要在下一个三角形上开始新簇
				// 注意：这可能意味着为最后一个三角形向目标添加 'end`，这表明最后一个
				// cluster is empty; however, the 'pop_back' after the loop will clean it up
				destination[result++] = unsigned(i + 1);

				// 重置缓存
				timestamp += cache_size + 1;

				running_misses = 0;
				running_faces = 0;
			}
		}

		// 每次达到目标 ACMR 时我们都刷新簇
		// 这意味按定义最后一个簇不是很好 - 经常出现只剩几个三角形的
		// 情况，从而产生很差的 ACMR 并显著拉低整体结果
		// 因此我们移除最后一个簇边界，将最后一个完整簇与最后一个不完整簇合并
		// 有时最后一个簇实际上足够好 - 这种情况下，上面的代码会在簇边界数组
		// 中添加 'end'（无论如何都需要移除）- 此代码会自动完成该操作
		if (destination[result - 1] != start)
		{
			result--;
		}
	}

	assert(result >= cluster_count);
	assert(result <= index_count / 3);

	return result;
}

} // namespace meshopt

void meshopt_optimizeOverdraw(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, float threshold)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(vertex_positions_stride >= 12 && vertex_positions_stride <= 256);
	assert(vertex_positions_stride % sizeof(float) == 0);

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

	unsigned int* cache_timestamps = allocator.allocate<unsigned int>(vertex_count);

	// 从完整三角形的缓存未命中生成硬边界
	unsigned int* hard_clusters = allocator.allocate<unsigned int>(index_count / 3);
	size_t hard_cluster_count = generateHardBoundaries(hard_clusters, indices, index_count, vertex_count, cache_size, cache_timestamps);

	// 生成软边界
	unsigned int* soft_clusters = allocator.allocate<unsigned int>(index_count / 3 + 1);
	size_t soft_cluster_count = generateSoftBoundaries(soft_clusters, indices, index_count, vertex_count, hard_clusters, hard_cluster_count, cache_size, threshold, cache_timestamps);

	const unsigned int* clusters = soft_clusters;
	size_t cluster_count = soft_cluster_count;

	// 填充排序数据
	float* sort_data = allocator.allocate<float>(cluster_count);
	calculateSortData(sort_data, indices, index_count, vertex_positions, vertex_count, vertex_positions_stride, clusters, cluster_count);

	// 使用排序数据对簇排序
	unsigned short* sort_keys = allocator.allocate<unsigned short>(cluster_count);
	unsigned int* sort_order = allocator.allocate<unsigned int>(cluster_count);
	calculateSortOrderRadix(sort_order, sort_data, sort_keys, cluster_count);

	// 填充输出缓冲区
	size_t offset = 0;

	for (size_t it = 0; it < cluster_count; ++it)
	{
		unsigned int cluster = sort_order[it];
		assert(cluster < cluster_count);

		size_t cluster_begin = clusters[cluster] * 3;
		size_t cluster_end = (cluster + 1 < cluster_count) ? clusters[cluster + 1] * 3 : index_count;
		assert(cluster_begin < cluster_end);

		memcpy(destination + offset, indices + cluster_begin, (cluster_end - cluster_begin) * sizeof(unsigned int));
		offset += cluster_end - cluster_begin;
	}

	assert(offset == index_count);
}

MESHOPTIMIZER_VOXEL_NAMESPACE_END