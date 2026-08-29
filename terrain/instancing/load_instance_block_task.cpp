#include "load_instance_block_task.h"
#include "../../engine/buffered_task_scheduler.h"
#include "../../streams/instance_data.h"
#include "../../util/containers/container_funcs.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/array_mesh.h"
#include "../../util/math/box3i.h"
#include "../../util/math/conv.h"
#include "../../util/profiling.h"
#include "generate_instances_block_task.h"
#include "instancer_quick_reloading_cache.h"

namespace voxel {

LoadInstanceChunkTask::LoadInstanceChunkTask(
		std::shared_ptr<InstancerTaskOutputQueue> output_queue,
		Ref<VoxelStream> stream,
		Ref<VoxelGenerator> voxel_generator,
		std::shared_ptr<InstancerQuickReloadingCache> quick_reload_cache,
		Ref<VoxelInstanceLibrary> library,
		Array mesh_arrays,
		const int32_t vertex_range_end,
		const int32_t index_range_end,
		const Vector3i grid_position,
		const uint8_t lod_index,
		const uint8_t instance_block_size,
		const uint8_t data_block_size,
		const UpMode up_mode
) :
		_output_queue(output_queue),
		_stream(stream),
		_voxel_generator(voxel_generator),
		_quick_reload_cache(quick_reload_cache),
		_library(library),
		_mesh_arrays(mesh_arrays),
		_vertex_range_end(vertex_range_end),
		_index_range_end(index_range_end),
		_render_grid_position(grid_position),
		_lod_index(lod_index),
		_instance_block_size(instance_block_size),
		_data_block_size(data_block_size),
		_up_mode(up_mode) //
{
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT(_output_queue != nullptr);
	VOXEL_ASSERT(_instance_block_size > 0);
	VOXEL_ASSERT(_data_block_size > 0);
	if (_instance_block_size < _data_block_size) {
		VOXEL_PRINT_ERROR("_instance_block_size < _data_block_size is not supported");
	}
#endif
}

void LoadInstanceChunkTask::run(ThreadedTaskContext &ctx) {
	VOXEL_PROFILE_SCOPE();

	struct Layer {
		int id = -1;
		uint8_t edited_mask = 0;
		StdVector<Transform3f> transforms;
	};

	StdVector<Layer> layers;

	// 尝试加载已保存的数据块
	if (_stream.is_valid()) {
		VOXEL_PROFILE_SCOPE();

		const unsigned int data_factor = _instance_block_size / _data_block_size;
		const Box3i data_box(_render_grid_position * data_factor, Vector3iUtil::create(data_factor));

		VOXEL_ASSERT(data_factor <= 2);
		FixedArray<VoxelStream::InstancesQueryData, 8> queries;

		// 创建查询
		unsigned int query_count = 0;
		data_box.for_each_cell([&query_count, &queries, this](Vector3i data_pos) {
			VoxelStream::InstancesQueryData &query = queries[query_count];
			query.lod_index = _lod_index;
			query.position_in_blocks = data_pos;
			query.result = VoxelStream::RESULT_BLOCK_NOT_FOUND;
			++query_count;
		});

		{
			unsigned int stream_queries_count = 0;
			FixedArray<VoxelStream::InstancesQueryData, 8> stream_queries;
			FixedArray<uint8_t, 8> stream_queries_octant_indices;

			for (unsigned int i = 0; i < stream_queries_octant_indices.size(); ++i) {
				stream_queries_octant_indices[i] = i;
			}

			if (_quick_reload_cache != nullptr) {
				// 首先查看快速重载缓存，并过滤出流向流的查询……我不喜欢这样，
				// 尤其是不同数据块大小会使情况变得复杂
				MutexLock mlock(_quick_reload_cache->mutex);
				for (unsigned int query_index = 0; query_index < query_count; ++query_index) {
					VoxelStream::InstancesQueryData &query = queries[query_index];
					auto it = _quick_reload_cache->map.find(query.position_in_blocks);

					if (it != _quick_reload_cache->map.end()) {
						VOXEL_PROFILE_SCOPE_NAMED("Instance quick reload");
						UniquePtr<InstanceBlockData> data;
						if (it->second != nullptr) {
							data = make_unique_instance<InstanceBlockData>();
							it->second->copy_to(*data);
						}
						query.result = VoxelStream::RESULT_BLOCK_FOUND;
						query.data = std::move(data);

					} else {
						stream_queries[stream_queries_count] = std::move(query);
						stream_queries_octant_indices[stream_queries_count] = query_index;
						++stream_queries_count;
					}
				}

			} else {
				stream_queries = std::move(queries);
				stream_queries_count = query_count;
			}

			_stream->load_instance_blocks(to_span(stream_queries, stream_queries_count));

			for (unsigned int i = 0; i < stream_queries_count; ++i) {
				const unsigned int octant_index = stream_queries_octant_indices[i];
				queries[octant_index] = std::move(stream_queries[i]);
			}
		}

		const Vector3i data_min_block_pos = _render_grid_position * data_factor;
		const int data_block_size_at_lod = static_cast<int>(_data_block_size) << _lod_index;

		// 遍历每个卦限（如果数据块与渲染数据块大小相同则只有 1 个，否则为 8 个）
		// 根据在流中找到的内容填充图层
		for (unsigned int octant_index = 0; octant_index < query_count; ++octant_index) {
			const VoxelStream::InstancesQueryData &query = queries[octant_index];

			if (query.result == VoxelStream::RESULT_BLOCK_FOUND) {
				if (query.data == nullptr) {
					// 这里一定没有任何实例，它们已被编辑移除

					if (query_count > 1) {
						for (Layer &layer : layers) {
							layer.edited_mask |= (1 << octant_index);
						}
					} else {
						for (Layer &layer : layers) {
							layer.edited_mask = 0xff;
						}
					}

					continue;
				}

				for (const InstanceBlockData::LayerData &loaded_layer_data : query.data->layers) {
					const int layer_id = loaded_layer_data.id;
					size_t layer_index;

					// 这里不使用哈希表，该数组通常很小
					if (!find(to_span_const(layers), layer_index, [layer_id](const Layer &layer) {
							return layer.id == layer_id;
						})) {
						layer_index = layers.size();
						Layer layer;
						layer.id = layer_id;
						layers.push_back(layer);
					}

					Layer &layer = layers[layer_index];

					if (query_count > 1) {
						// 将卦限标记为已修改，以便生成器可以跳过它
						layer.edited_mask |= (1 << octant_index);
					} else {
						layer.edited_mask = 0xff;
					}

					const unsigned int dst_index0 = layer.transforms.size();

					layer.transforms.reserve(layer.transforms.size() + loaded_layer_data.instances.size());
					for (const InstanceBlockData::InstanceData &id : loaded_layer_data.instances) {
						layer.transforms.push_back(id.transform);
					}

					if (data_factor == 2) {
						// 数据块相对于比渲染数据块更小的网格存储实例。
						// 因此我们需要调整它们的相对位置。
						const Vector3f rel =
								to_vec3f((query.position_in_blocks - data_min_block_pos) * data_block_size_at_lod);
						VOXEL_ASSERT(dst_index0 <= layer.transforms.size());
						for (auto it = layer.transforms.begin() + dst_index0; it != layer.transforms.end(); ++it) {
							it->origin += rel;
						}
					}
				}
			}
		}
	}

	// 生成其余部分
	if (_mesh_arrays.size() != 0 && _library.is_valid()) {
		VOXEL_PROFILE_SCOPE();

		// TODO 缓存内存
		StdVector<VoxelInstanceLibrary::PackedItem> items;
		_library->get_packed_items_at_lod(items, _lod_index);

		if (items.size() > 0) {
			BufferedTaskScheduler &task_scheduler = BufferedTaskScheduler::get_for_current_thread();

			for (const VoxelInstanceLibrary::PackedItem &item : items) {
				if (item.generator.is_valid()) {
					size_t layer_index;
					// 这里不使用哈希表，该数组通常很小
					const int layer_id = item.id;
					if (!find(to_span_const(layers), layer_index, [layer_id](const Layer &layer) {
							return layer.id == layer_id;
						})) {
						layer_index = layers.size();
						layers.push_back(Layer());
					}

					Layer &layer = layers[layer_index];
					if (layer.edited_mask == 0xff) {
						// 无需生成
						continue;
					}

					PackedVector3Array vertices = _mesh_arrays[ArrayMesh::ARRAY_VERTEX];

					if (vertices.size() != 0) {
						// 触发一个单独的任务，因为它可以并行运行，而当前任务可能不行（直到
						// 我们弄清楚如何让 VoxelStream I/O 足够并行）
						GenerateInstancesBlockTask *task = VOXEL_NEW(GenerateInstancesBlockTask);
						task->mesh_block_grid_position = _render_grid_position;
						task->layer_id = item.id;
						task->mesh_block_size = static_cast<int>(_instance_block_size) << _lod_index;
						task->lod_index = _lod_index;
						task->edited_mask = layer.edited_mask;
						task->up_mode = _up_mode;
						task->surface_arrays = _mesh_arrays;
						task->vertex_range_end = _vertex_range_end;
						task->index_range_end = _index_range_end;
						task->generator = item.generator;
						task->voxel_generator = _voxel_generator;
						task->transforms = std::move(layer.transforms);
						task->output_queue = _output_queue;

						task_scheduler.push_main_task(task);
					}

					// 我们将剩余加载委托给了另一个任务，因此将其从列表中移除。
					layers[layer_index] = std::move(layers.back());
					layers.pop_back();
				}
			}

			task_scheduler.flush();
		}
	}

	// 发布结果
	for (Layer &layer : layers) {
		InstanceLoadingTaskOutput o;
		o.layer_id = layer.id;
		// 通常会是满的
		o.edited_mask = layer.edited_mask;
		o.render_block_position = _render_grid_position;
		o.transforms = std::move(layer.transforms);
		{
			MutexLock(_output_queue->mutex);
			_output_queue->results.push_back(std::move(o));
		}
	}
}

} // namespace voxel
