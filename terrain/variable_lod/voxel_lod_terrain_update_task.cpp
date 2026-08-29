#include "voxel_lod_terrain_update_task.h"
#include "../../engine/buffered_task_scheduler.h"
#include "../../engine/voxel_engine.h"
#include "../../generators/generate_block_task.h"
#include "../../meshers/mesh_block_task.h"
#include "../../storage/voxel_data.h"
#include "../../streams/load_block_data_task.h"
#include "../../streams/save_block_data_task.h"
#include "../../util/containers/container_funcs.h"
#include "../../util/dstack.h"
#include "../../util/godot/classes/engine.h"
#include "../../util/math/conv.h"
#include "../../util/profiling.h"
#include "../../util/profiling_clock.h"
#include "../../util/string/format.h"
#include "../../util/tasks/async_dependency_tracker.h"
#include "voxel_lod_terrain_update_clipbox_streaming.h"
#include "voxel_lod_terrain_update_octree_streaming.h"

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
#include "../../meshers/transvoxel/voxel_mesher_transvoxel.h"
#endif

namespace voxel {

namespace {

inline Vector3i get_block_center(Vector3i pos, int bs, int lod) {
	return (pos << lod) * bs + Vector3iUtil::create(bs / 2);
}

void init_sparse_octree_priority_dependency(
		PriorityDependency &dep,
		const Vector3i block_position,
		const uint8_t lod,
		const int data_block_size,
		const std::shared_ptr<PriorityDependency::ViewersData> &shared_viewers_data,
		const Transform3D &volume_transform,
		const float octree_lod_distance
) {
	const Vector3i voxel_pos = get_block_center(block_position, data_block_size, lod);
	const float block_radius = (data_block_size << lod) / 2;
	dep.shared = shared_viewers_data;
	dep.world_position = to_vec3f(volume_transform.xform(voxel_pos));
	const float transformed_block_radius =
			volume_transform.basis.xform(Vector3(block_radius, block_radius, block_radius)).length();

	// 超过此距离即可安全丢弃数据块，而不会有阻塞 LOD 细分的风险。
	// 该距离不取决于观察者的视距，而取决于 LOD 精度。
	// TODO 这里是否应使用 `data_block_size`？是否应改为 mesh_block_size？
	dep.drop_distance_squared = math::squared(
			2.f * transformed_block_radius *
			VoxelEngine::get_octree_lod_block_region_extent(octree_lod_distance, data_block_size)
	);
}

// 仅当我们想要缓存体素数据时才使用
void request_block_generate(
		const VolumeID volume_id,
		const unsigned int data_block_size,
		const std::shared_ptr<StreamingDependency> &stream_dependency,
		const std::shared_ptr<VoxelData> &data,
		const Vector3i block_pos,
		const int lod_index,
		const std::shared_ptr<PriorityDependency::ViewersData> &shared_viewers_data,
		const Transform3D &volume_transform,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const std::shared_ptr<AsyncDependencyTracker> tracker,
		const bool allow_drop,
		BufferedTaskScheduler &task_scheduler,
		const TaskCancellationToken cancellation_token
) {
	CRASH_COND(data_block_size > 255);
	CRASH_COND(stream_dependency == nullptr);

	// 若流和生成器均为空，则一开始就不应发出此请求
	ERR_FAIL_COND(stream_dependency->generator.is_null());

	VoxelGenerator::BlockTaskParams params;
	params.volume_id = volume_id;
	params.block_position = block_pos;
	params.format = data->get_format();
	params.lod_index = lod_index;
	params.block_size = data_block_size;
	params.stream_dependency = stream_dependency;
	params.tracker = tracker;
	params.drop_beyond_max_distance = allow_drop;
	params.data = data;
#ifdef VOXEL_ENABLE_GPU
	params.use_gpu = settings.generator_use_gpu;
#endif
	params.cancellation_token = cancellation_token;

	init_sparse_octree_priority_dependency(
			params.priority_dependency,
			block_pos,
			lod_index,
			data_block_size,
			shared_viewers_data,
			volume_transform,
			settings.lod_distance
	);

	IThreadedTask *task = stream_dependency->generator->create_block_task(params);

	task_scheduler.push_main_task(task);
}

// 仅在按数据块流式加载时使用
void request_block_load(
		const VolumeID volume_id,
		const unsigned int data_block_size,
		const std::shared_ptr<StreamingDependency> &stream_dependency,
		const std::shared_ptr<VoxelData> &data,
		const Vector3i block_pos,
		const int lod_index,
		const std::shared_ptr<PriorityDependency::ViewersData> &shared_viewers_data,
		const Transform3D &volume_transform,
		const VoxelLodTerrainUpdateData::Settings &settings,
		BufferedTaskScheduler &task_scheduler,
		const TaskCancellationToken cancellation_token,
		VoxelLodTerrainUpdateData::State &state
) {
	VOXEL_ASSERT(data_block_size < 256);
	VOXEL_ASSERT(stream_dependency != nullptr);

	VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
	{
		std::shared_ptr<VoxelBuffer> voxels;
		{
			MutexLock mlock(lod.unloaded_saving_blocks_mutex);
			auto saving_it = lod.unloaded_saving_blocks.find(block_pos);
			if (saving_it != lod.unloaded_saving_blocks.end()) {
				voxels = saving_it->second;
			}
		}
		if (voxels != nullptr) {
			lod.quick_reloading_blocks.push_back(VoxelLodTerrainUpdateData::QuickReloadingBlock{ voxels, block_pos });
			return;
		}
	}

	if (stream_dependency->stream.is_valid()) {
		PriorityDependency priority_dependency;
		init_sparse_octree_priority_dependency(
				priority_dependency,
				block_pos,
				lod_index,
				data_block_size,
				shared_viewers_data,
				volume_transform,
				settings.lod_distance
		);

		const bool request_instances = false;
		LoadBlockDataTask *task = VOXEL_NEW(LoadBlockDataTask(
				volume_id,
				block_pos,
				lod_index,
				data_block_size,
				request_instances,
				stream_dependency,
				priority_dependency,
				settings.cache_generated_blocks,
				settings.generator_use_gpu,
				data,
				cancellation_token
		));

		task_scheduler.push_io_task(task);

	} else if (settings.cache_generated_blocks) {
		// 直接生成数据块，不检查流。
		request_block_generate(
				volume_id,
				data_block_size,
				stream_dependency,
				data,
				block_pos,
				lod_index,
				shared_viewers_data,
				volume_transform,
				settings,
				nullptr,
				true,
				task_scheduler,
				cancellation_token
		);

	} else {
		VOXEL_PRINT_WARNING("Requesting a block load when it should not have been necessary");
	}
}

void send_block_data_requests(
		const VolumeID volume_id,
		const Span<const VoxelLodTerrainUpdateData::BlockToLoad> blocks_to_load,
		const std::shared_ptr<StreamingDependency> &stream_dependency,
		const std::shared_ptr<VoxelData> &data,
		const std::shared_ptr<PriorityDependency::ViewersData> &shared_viewers_data,
		const unsigned int data_block_size,
		const Transform3D &volume_transform,
		const VoxelLodTerrainUpdateData::Settings &settings,
		BufferedTaskScheduler &task_scheduler,
		VoxelLodTerrainUpdateData::State &state
) {
	for (unsigned int i = 0; i < blocks_to_load.size(); ++i) {
		const VoxelLodTerrainUpdateData::BlockToLoad btl = blocks_to_load[i];
		request_block_load(
				volume_id,
				data_block_size,
				stream_dependency,
				data,
				btl.loc.position,
				btl.loc.lod,
				shared_viewers_data,
				volume_transform,
				settings,
				task_scheduler,
				btl.cancellation_token,
				state
		);
	}
}

// 当启用了流式加载，但地形既没有流也没有生成器（移动时只能存在空的
// 数据块），或生成被配置为在网格化期间即时进行时使用。
// 因此我们必须模拟一个立即返回空数据块的 VoxelStream。
void apply_block_data_requests_as_empty(
		const Span<const VoxelLodTerrainUpdateData::BlockToLoad> blocks_to_load,
		VoxelData &data,
		VoxelLodTerrainUpdateData::State &state,
		const VoxelLodTerrainUpdateData::Settings &settings
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(data.is_streaming_enabled());

	for (const VoxelLodTerrainUpdateData::BlockToLoad &btl : blocks_to_load) {
		VoxelLodTerrainUpdateData::Lod &lod = state.lods[btl.loc.lod];
		RefCount viewers;
		{
			MutexLock mlock(lod.loading_blocks_mutex);
			auto it = lod.loading_blocks.find(btl.loc.position);
			if (it != lod.loading_blocks.end()) {
				viewers = it->second.viewers;
				lod.loading_blocks.erase(it);
			} else {
				VOXEL_PRINT_ERROR("Loading block wasn't found when consuming data requests as empty");
			}
		}
		{
			// 该数据块被视为“已加载”，因为我们知道保存文件中没有任何内容可加载，
			// 且若存在生成器，生成可以即时进行，因此通过分配一个
			// 不附带体素的数据块来表示。
			VoxelDataBlock empty_block(btl.loc.lod);
			empty_block.viewers = viewers;
			data.try_set_block(btl.loc.position, empty_block);
		}
	}

	if (settings.streaming_system == VoxelLodTerrainUpdateData::STREAMING_SYSTEM_CLIPBOX) {
		// 由于启用了流式加载，必须告知系统此数据块现在“已加载”，因为它不使用
		// 轮询来了解何时加载完成
		MutexLock mlock(state.clipbox_streaming.loaded_data_blocks_mutex);
		for (const VoxelLodTerrainUpdateData::BlockToLoad &btl : blocks_to_load) {
			state.clipbox_streaming.loaded_data_blocks.push_back(btl.loc);
		}
	}
}

void request_voxel_block_save(
		const VolumeID volume_id,
		const std::shared_ptr<VoxelBuffer> &voxels,
		const Vector3i block_pos,
		const int lod_index,
		const std::shared_ptr<StreamingDependency> &stream_dependency,
		BufferedTaskScheduler &task_scheduler,
		const std::shared_ptr<AsyncDependencyTracker> tracker,
		const bool with_flush
) {
	CRASH_COND(stream_dependency == nullptr);
	ERR_FAIL_COND(stream_dependency->stream.is_null());

	SaveBlockDataTask *task =
			VOXEL_NEW(SaveBlockDataTask(volume_id, block_pos, lod_index, voxels, stream_dependency, tracker, with_flush));

	// 无优先级数据，保存不需要排序。

	task_scheduler.push_io_task(task);
}

void send_mesh_requests(
		const VolumeID volume_id,
		VoxelLodTerrainUpdateData::State &state,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const std::shared_ptr<VoxelData> &data_ptr,
		const std::shared_ptr<MeshingDependency> meshing_dependency,
		const std::shared_ptr<PriorityDependency::ViewersData> &shared_viewers_data,
		const Transform3D &volume_transform,
		BufferedTaskScheduler &task_scheduler
) {
	VOXEL_PROFILE_SCOPE();

	VOXEL_ASSERT(data_ptr != nullptr);
	const VoxelData &data = *data_ptr;

	const int data_block_size = data.get_block_size();
	const int mesh_block_size = 1 << settings.mesh_block_size_po2;
	const int render_to_data_factor = mesh_block_size / data_block_size;
	const unsigned int lod_count = data.get_lod_count();

	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
		VOXEL_PROFILE_SCOPE();
		VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

		for (unsigned int bi = 0; bi < lod.mesh_blocks_pending_update.size(); ++bi) {
			VOXEL_PROFILE_SCOPE();
			const VoxelLodTerrainUpdateData::MeshToUpdate &mesh_to_update = lod.mesh_blocks_pending_update[bi];

			auto mesh_block_it = lod.mesh_map_state.map.find(mesh_to_update.position);
			// 在请求网格更新之前，数据块必须已被分配
			VOXEL_ASSERT_CONTINUE(mesh_block_it != lod.mesh_map_state.map.end());
			VoxelLodTerrainUpdateData::MeshBlockState &mesh_block = mesh_block_it->second;
			// 这里获得的所有数据块必须处于已调度状态
			VOXEL_ASSERT_CONTINUE(mesh_block.state == VoxelLodTerrainUpdateData::MESH_UPDATE_NOT_SENT);

			// 获取数据块及其相邻数据块
			// VoxelEngine::BlockMeshInput mesh_request;
			// mesh_request.render_block_position = mesh_block_pos;
			// mesh_request.lod = lod_index;

			// 我们会经常分配它。若这成为问题，将其池化应该很容易。
			MeshBlockTask *task = VOXEL_NEW(MeshBlockTask);
			task->volume_id = volume_id;
			task->mesh_block_position = mesh_to_update.position;
			task->lod_index = lod_index;
			task->lod_hint = true;
			task->meshing_dependency = meshing_dependency;
			task->data = data_ptr;
			task->require_visual = mesh_to_update.require_visual;
			task->collision_hint = settings.collision_enabled;
#ifdef VOXEL_ENABLE_SMOOTH_MESHING
			task->detail_texture_settings = settings.detail_texture_settings;
			task->detail_texture_generator_override = settings.detail_texture_generator_override;
			task->detail_texture_generator_override_begin_lod_index =
					settings.detail_texture_generator_override_begin_lod_index;
			task->detail_texture_use_gpu = settings.detail_textures_use_gpu;
#endif
			task->block_generation_use_gpu = settings.generator_use_gpu;
			task->cancellation_token = mesh_to_update.cancellation_token;

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
			// 若已有更新正在处理，则不要更新细节纹理
			if (settings.detail_texture_settings.enabled &&
				lod_index >= settings.detail_texture_settings.begin_lod_index &&
				mesh_block.detail_texture_state != VoxelLodTerrainUpdateData::DETAIL_TEXTURE_PENDING) {
				mesh_block.detail_texture_state = VoxelLodTerrainUpdateData::DETAIL_TEXTURE_PENDING;
				task->require_detail_texture = true;
			}
#endif

			const Box3i data_box =
					Box3i(render_to_data_factor * mesh_to_update.position, Vector3iUtil::create(render_to_data_factor))
							.padded(1);

			// 迭代顺序对线程访问很重要。
			// 由于使用的约定，该数组还隐式编码了数据块位置，
			// 因此无需在请求中再包含位置
			data.get_blocks_with_voxel_data(data_box, lod_index, to_span(task->blocks));
			task->blocks_count = Vector3iUtil::get_volume_u64(data_box.size);

			// TODO 发送给此函数的坐标存在不一致。
			// 有时发送数据块坐标，有时发送网格数据块坐标。它们并不总是
			// 相同，这可能会导致优先级排序出现问题？
			init_sparse_octree_priority_dependency(
					task->priority_dependency,
					task->mesh_block_position,
					task->lod_index,
					mesh_block_size,
					shared_viewers_data,
					volume_transform,
					settings.lod_distance
			);

			task_scheduler.push_main_task(task);

			mesh_block.state = VoxelLodTerrainUpdateData::MESH_UPDATE_SENT;
			mesh_block.update_list_index = -1;
		}

		lod.mesh_blocks_pending_update.clear();
	}
}

// 为编辑准备而生成所有尚未存在的数据块。
// 此函数为每个数据块调度一个并行任务。
// 可轮询返回的跟踪器以检测其是否完成。
// 仅在完全加载模式下使用，因为在流式加载模式下数据块必须已存在。
std::shared_ptr<AsyncDependencyTracker> preload_boxes_async(
		VoxelLodTerrainUpdateData::State &state,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const std::shared_ptr<VoxelData> data_ptr,
		const Span<const Box3i> voxel_boxes,
		const Span<IThreadedTask *> next_tasks,
		const VolumeID volume_id,
		const std::shared_ptr<StreamingDependency> &stream_dependency,
		const std::shared_ptr<PriorityDependency::ViewersData> &shared_viewers_data,
		const Transform3D &volume_transform,
		BufferedTaskScheduler &task_scheduler
) {
	VOXEL_PROFILE_SCOPE();

	VOXEL_ASSERT(data_ptr != nullptr);
	VoxelData &data = *data_ptr;

	VOXEL_ASSERT_RETURN_V_MSG(
			data.is_streaming_enabled() == false, nullptr, "This function can only be used in full load mode"
	);

	struct TaskArguments {
		Vector3i block_pos;
		unsigned int lod_index;
	};

	StdVector<TaskArguments> todo;

	const unsigned int data_block_size = data.get_block_size();
	const unsigned int lod_count = data.get_lod_count();

	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
		for (unsigned int box_index = 0; box_index < voxel_boxes.size(); ++box_index) {
			VOXEL_PROFILE_SCOPE_NAMED("Box");

			VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
			const Box3i voxel_box = voxel_boxes[box_index];
			const Box3i block_box = voxel_box.downscaled(data_block_size << lod_index);

			// VOXEL_PRINT_VERBOSE(String("Preloading box {0} at lod {1}")
			// 						.format(varray(block_box.to_string(), lod_index)));

			static thread_local StdVector<Vector3i> tls_missing;
			tls_missing.clear();
			data.get_missing_blocks(block_box, lod_index, tls_missing);

			if (tls_missing.size() > 0) {
				// MutexLock mlock(lod.loading_blocks_mutex);
				for (const Vector3i &missing_bpos : tls_missing) {
					if (!lod.has_loading_block(missing_bpos)) {
						todo.push_back(TaskArguments{ missing_bpos, lod_index });
						// 在完全加载模式下，我们不应需要填充 loading_blocks
						// lod.loading_blocks.insert(missing_bpos);
					}
				}
			}
		}
	}

	VOXEL_PRINT_VERBOSE(format("Preloading boxes with {} tasks", todo.size()));

	std::shared_ptr<AsyncDependencyTracker> tracker = nullptr;

	// TODO `next_tasks` 并行执行。但由于它们可能是编辑操作，我们是否可以按顺序执行？

	if (todo.size() > 0) {
		VOXEL_PROFILE_SCOPE_NAMED("Posting requests");

		// 仅当我们确实在创建任务时才创建跟踪器。若仍创建它，
		// 则没有任务会接管其所有权，因此若此函数返回后未保存它，
		// 它就会销毁 `next_tasks`。

		// 这可能先运行生成任务，再运行编辑操作
		tracker = make_shared_instance<AsyncDependencyTracker>(
				todo.size(),
				next_tasks,
				[](Span<IThreadedTask *> p_next_tasks) { //
					VoxelEngine::get_singleton().push_async_tasks(p_next_tasks);
				}
		);

		for (unsigned int i = 0; i < todo.size(); ++i) {
			const TaskArguments args = todo[i];
			request_block_generate(
					volume_id,
					data_block_size,
					stream_dependency,
					data_ptr,
					args.block_pos,
					args.lod_index,
					shared_viewers_data,
					volume_transform,
					settings,
					tracker,
					false,
					task_scheduler,
					TaskCancellationToken()
			);
		}

	} else if (next_tasks.size() > 0) {
		// 无需预加载，现在即可调度 `next_tasks`
		VoxelEngine::get_singleton().push_async_tasks(next_tasks);
	}

	return tracker;
}

void process_async_edits(
		VoxelLodTerrainUpdateData::State &state,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const std::shared_ptr<VoxelData> &data,
		const VolumeID volume_id,
		const std::shared_ptr<StreamingDependency> &stream_dependency,
		const std::shared_ptr<PriorityDependency::ViewersData> &shared_viewers_data,
		const Transform3D &volume_transform,
		BufferedTaskScheduler &task_scheduler
) {
	VOXEL_PROFILE_SCOPE();

	if (state.running_async_edits.size() == 0) {
		// 当先前编辑完成时，调度所有后续编辑

		StdVector<Box3i> boxes_to_preload;
		StdVector<IThreadedTask *> tasks_to_schedule;
		std::shared_ptr<AsyncDependencyTracker> last_tracker = nullptr;

		for (unsigned int edit_index = 0; edit_index < state.pending_async_edits.size(); ++edit_index) {
			VoxelLodTerrainUpdateData::AsyncEdit &edit = state.pending_async_edits[edit_index];
			CRASH_COND(edit.task_tracker->has_next_tasks());

			// 不确定是否值得做，我认为任务在调度之前不会被中止。
			if (edit.task_tracker->is_aborted()) {
				VOXEL_PRINT_VERBOSE("Aborted async edit");
				VOXEL_DELETE(edit.task);
				continue;
			}

			boxes_to_preload.push_back(edit.box);
			tasks_to_schedule.push_back(edit.task);
			state.running_async_edits.push_back( //
					VoxelLodTerrainUpdateData::RunningAsyncEdit{ edit.task_tracker, edit.box }
			);
		}

		if (boxes_to_preload.size() > 0) {
			preload_boxes_async(
					state,
					settings,
					data,
					to_span_const(boxes_to_preload),
					to_span(tasks_to_schedule),
					volume_id,
					stream_dependency,
					shared_viewers_data,
					volume_transform,
					task_scheduler
			);
		}

		state.pending_async_edits.clear();
	}
}

void process_changed_generated_areas(
		VoxelLodTerrainUpdateData::State &state,
		const VoxelLodTerrainUpdateData::Settings &settings,
		const unsigned int lod_count
) {
	const unsigned int mesh_block_size = 1 << settings.mesh_block_size_po2;

	MutexLock lock(state.changed_generated_areas_mutex);
	if (state.changed_generated_areas.size() == 0) {
		return;
	}

	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
		VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

		for (auto box_it = state.changed_generated_areas.begin(); box_it != state.changed_generated_areas.end();
			 ++box_it) {
			const Box3i &voxel_box = *box_it;
			const Box3i bbox = voxel_box.padded(1).downscaled(mesh_block_size << lod_index);

			// TODO 若存在缓存的已生成数据块，则需要重新缓存或移除

			RWLockRead rlock(lod.mesh_map_state.map_lock);

			bbox.for_each_cell_zxy([&lod](const Vector3i bpos) {
				auto block_it = lod.mesh_map_state.map.find(bpos);
				if (block_it != lod.mesh_map_state.map.end()) {
					VoxelLodTerrainUpdateTask::schedule_mesh_update(
							block_it->second,
							bpos,
							lod.mesh_blocks_pending_update,
							block_it->second.mesh_viewers.get() > 0
					);
				}
			});
		}
	}

	state.changed_generated_areas.clear();
}

} // namespace

void VoxelLodTerrainUpdateTask::send_block_save_requests(
		const VolumeID volume_id,
		const Span<VoxelData::BlockToSave> blocks_to_save,
		const std::shared_ptr<StreamingDependency> &stream_dependency,
		BufferedTaskScheduler &task_scheduler,
		const std::shared_ptr<AsyncDependencyTracker> tracker,
		const bool with_flush
) {
	for (unsigned int i = 0; i < blocks_to_save.size(); ++i) {
		const VoxelData::BlockToSave &b = blocks_to_save[i];
		VOXEL_PRINT_VERBOSE(format("Requesting save of block {} lod {}", b.position, b.lod_index));
		request_voxel_block_save(
				volume_id, b.voxels, b.position, b.lod_index, stream_dependency, task_scheduler, tracker, with_flush
		);
	}
}

void VoxelLodTerrainUpdateTask::flush_pending_lod_edits(
		VoxelLodTerrainUpdateData::State &state,
		VoxelData &data,
		const int mesh_block_size
) {
	VOXEL_DSTACK();
	VOXEL_PROFILE_SCOPE();

	static thread_local StdVector<Vector3i> tls_modified_lod0_blocks;
	static thread_local StdVector<Box3i> tls_modified_voxel_areas_lod0;
	// static thread_local StdVector<VoxelData::BlockLocation> tls_updated_block_locations;

	tls_modified_lod0_blocks.clear();
	tls_modified_voxel_areas_lod0.clear();

	// 消耗输入
	{
		MutexLock lock(state.edit_notifications.mutex);

		// 不确定能否直接使用 `=`？std::vector 会如何处理容量？
		append_array(tls_modified_lod0_blocks, state.edit_notifications.edited_blocks_lod0);
		append_array(tls_modified_voxel_areas_lod0, state.edit_notifications.edited_voxel_areas_lod0);

		state.edit_notifications.edited_blocks_lod0.clear();
		state.edit_notifications.edited_voxel_areas_lod0.clear();
	}

	// 更新所有数据 LOD
	// tls_updated_block_locations.clear();
	data.update_lods(to_span(tls_modified_lod0_blocks), nullptr);

	// 更新受影响的网格。
	// TODO 优化：是否更早地在 LOD0 触发网格更新？由于先完成所有 mipmap 生成工作
	// 会带来一些延迟，而且我们知道编辑无论如何都会在 mipmap 生成之前发生
	const unsigned int lod_count = data.get_lod_count();
	for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
		VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
		const int mesh_block_size_at_lod = mesh_block_size << lod_index;

		for (const Box3i voxel_box : tls_modified_voxel_areas_lod0) {
			// 靠近数据块边界的编辑需要填充，因为这类编辑尽管只影响
			// 一个数据块，却可能影响多个网格
			const Box3i padded_voxel_box = voxel_box.padded(1);
			const Box3i mesh_block_box = padded_voxel_box.downscaled(mesh_block_size_at_lod);

			mesh_block_box.for_each_cell([&lod](Vector3i mesh_block_pos) {
				auto mesh_block_it = lod.mesh_map_state.map.find(mesh_block_pos);
				if (mesh_block_it != lod.mesh_map_state.map.end()) {
					// 若此处存在网格数据块状态，则它将需要更新。
					// 若不存在，则它可能在我们靠近时稍后被创建
					schedule_mesh_update( //
							mesh_block_it->second, //
							mesh_block_pos, //
							lod.mesh_blocks_pending_update, //
							mesh_block_it->second.mesh_viewers.get() > 0 //
					);
				}
			});
		}
	}

	// -- 旧逻辑仅基于已更新的数据块，但不考虑填充（需要在周围强制编辑
	// 来模拟，这并不理想）。
	// 在每个受影响的 LOD 调度网格更新
	// for (const VoxelData::BlockLocation loc : tls_updated_block_locations) {
	// 	const Vector3i mesh_block_pos = math::floordiv(loc.position, data_to_mesh_factor);
	// 	VoxelLodTerrainUpdateData::Lod &dst_lod = state.lods[loc.lod_index];
	//
	// 	auto mesh_block_it = dst_lod.mesh_map_state.map.find(mesh_block_pos);
	// 	if (mesh_block_it != dst_lod.mesh_map_state.map.end()) {
	// 		// 若此处存在网格数据块状态，则它将需要更新。
	// 		// 若不存在，则它可能在我们靠近时稍后被创建
	// 		schedule_mesh_update(mesh_block_it->second, mesh_block_pos, dst_lod.blocks_pending_update);
	// 	}
	// }
}

uint8_t VoxelLodTerrainUpdateTask::get_transition_mask(
		const VoxelLodTerrainUpdateData::State &state,
		const Vector3i block_pos,
		const unsigned int lod_index,
		const unsigned int lod_count
) {
	uint8_t transition_mask = 0;

	if (lod_index + 1 >= lod_count) {
		// 我们在更高分辨率的数据块上执行过渡。
		// 因此，最低分辨率的数据块永远不会有过渡。
		return transition_mask;
	}

	const VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

	// 基于八叉树规则以及它必须先于其它步骤运行的约束，检查相同 LOD 的相邻数据块：
	// 若某个相邻数据块缺失或不可见，则意味着以下情况之一：
	// - lod+1 处的相邻数据块可见或未加载（必须有过渡）
	// - lod-1 处的相邻数据块可见（无过渡）

	uint8_t visible_neighbors_of_same_lod = 0;
	for (unsigned int dir = 0; dir < Cube::SIDE_COUNT; ++dir) {
		const Vector3i npos = block_pos + Cube::g_side_normals[dir];

		auto nblock_it = lod.mesh_map_state.map.find(npos);

		if (nblock_it != lod.mesh_map_state.map.end() && nblock_it->second.visual_active) {
			visible_neighbors_of_same_lod |= (1 << dir);
		}
	}

	if (visible_neighbors_of_same_lod == 0b111111) {
		// 无需过渡
		return transition_mask;
	}

	{
		const Vector3i lower_pos = block_pos >> 1;
		const Vector3i upper_pos = block_pos << 1;

		const VoxelLodTerrainUpdateData::Lod &lower_lod = state.lods[lod_index + 1];

		// 至少有一个相邻数据块不可见。
		// 检查不同 LOD 的相邻数据块（某一侧只能有一种）
		for (unsigned int dir = 0; dir < Cube::SIDE_COUNT; ++dir) {
			const unsigned int dir_mask = (1 << dir);

			if ((visible_neighbors_of_same_lod & dir_mask) != 0) {
				continue;
			}

			const Vector3i side_normal = Cube::g_side_normals[dir];
			const Vector3i lower_neighbor_pos = (block_pos + side_normal) >> 1;

			if (lower_neighbor_pos != lower_pos) {
				auto lower_neighbor_block_it = lower_lod.mesh_map_state.map.find(lower_neighbor_pos);

				if (lower_neighbor_block_it != lower_lod.mesh_map_state.map.end() &&
					lower_neighbor_block_it->second.visual_active) {
					// 该数据块有一个可见的较低 LOD 相邻数据块
					transition_mask |= dir_mask;
					continue;
				}
			}

			if (lod_index > 0) {
				// 检查较高 LOD 的相邻数据块。
				// 每侧始终有 4 个，检查任意一个即可

				Vector3i upper_neighbor_pos = upper_pos;
				for (unsigned int i = 0; i < Vector3iUtil::AXIS_COUNT; ++i) {
					if (side_normal[i] == -1) {
						--upper_neighbor_pos[i];
					} else if (side_normal[i] == 1) {
						upper_neighbor_pos[i] += 2;
					}
				}

				const VoxelLodTerrainUpdateData::Lod &upper_lod = state.lods[lod_index - 1];
				auto upper_neighbor_block_it = upper_lod.mesh_map_state.map.find(upper_neighbor_pos);

				if (upper_neighbor_block_it == upper_lod.mesh_map_state.map.end() ||
					upper_neighbor_block_it->second.visual_active == false) {
					// 该数据块尚无可见的相邻数据块。世界边界？假定为较低 LOD。
					transition_mask |= dir_mask;
				}
			}
		}
	}

	return transition_mask;
}

void update_transition_masks(
		VoxelLodTerrainUpdateData::State &state,
		uint32_t lods_to_update_transitions,
		const unsigned int lod_count,
		// 目前为继续支持旧的八叉树流式加载系统所需，该系统不支持多个观察者
		const bool use_refcounts
) {
	// TODO 优化：这可行但不够智能。
	// 它不会花费太长时间（当八叉树以 LOD 距离 60 更新时约 100 微秒）。
	// 我们过去只根据八叉树更新中添加/移除的数据块来更新位置，
	// 这比现在更快。但它遗漏了一些位置，导致出现恼人的裂缝。
	// 因此，当 LOD N 中任何数据块状态改变时，我们会更新 LOD N-1、N 和 N+1 中的所有过渡。
	// 目前尚不清楚旧方法为何不起作用，也许是因为它没有正确更新 N-1 和 N+1。
	// 若你找到更好的方法，它必须符合下面的验证检查。
	if (lods_to_update_transitions != 0) {
		VOXEL_PROFILE_SCOPE_NAMED("Transition masks");
		// 我们传入一个被填充为 (0b111 << index) 的掩码，因为我们想包含 lod+1、lod+0 和 lod-1。但
		// 由于 -1 的情况需要更多代码，我们改为将掩码偏移 1。然后在最后，
		// 只需在此处撤销一次该偏移。
		lods_to_update_transitions >>= 1;

		for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
			if ((lods_to_update_transitions & (1 << lod_index)) == 0) {
				continue;
			}

			VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];

			// TODO 可能没有必要，因为我们在更新任务中运行此代码。任务运行期间不允许其它线程修改
			// 此映射。
			RWLockRead rlock(lod.mesh_map_state.map_lock);

			for (auto it = lod.mesh_map_state.map.begin(); it != lod.mesh_map_state.map.end(); ++it) {
				VoxelLodTerrainUpdateData::MeshBlockState &mesh_block = it->second;

				if (mesh_block.visual_active && (!use_refcounts || mesh_block.mesh_viewers.get() > 0)) {
					const uint8_t recomputed_mask =
							VoxelLodTerrainUpdateTask::get_transition_mask(state, it->first, lod_index, lod_count);

					if (recomputed_mask != it->second.transition_mask) {
						mesh_block.transition_mask = recomputed_mask;
						lod.mesh_blocks_to_update_transitions.push_back( //
								VoxelLodTerrainUpdateData::TransitionUpdate{ it->first, recomputed_mask }
						);
					}
				}
			}
		}
	}
#if 0
	// DEBUG: 过渡掩码更新的验证检查。
	{
		VOXEL_PROFILE_SCOPE_NAMED("Transition checks");
		for (unsigned int lod_index = 0; lod_index < lod_count; ++lod_index) {
			const VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
			RWLockRead rlock(lod.mesh_map_state.map_lock);
			for (auto it = lod.mesh_map_state.map.begin(); it != lod.mesh_map_state.map.end(); ++it) {
				if (it->second.active) {
					const uint8_t recomputed_mask =
							VoxelLodTerrainUpdateTask::get_transition_mask(state, it->first, lod_index, lod_count);
					CRASH_COND(recomputed_mask != it->second.transition_mask);
				}
			}
		}
	}
#endif
}

void add_unloaded_saving_blocks(VoxelLodTerrainUpdateData::Lod &lod, Span<const VoxelData::BlockToSave> src) {
	if (src.size() == 0) {
		return;
	}
	VOXEL_PROFILE_SCOPE();
	MutexLock mlock(lod.unloaded_saving_blocks_mutex);
	for (const VoxelData::BlockToSave &bts : src) {
		lod.unloaded_saving_blocks[bts.position] = bts.voxels;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VoxelLodTerrainUpdateTask::run(ThreadedTaskContext &ctx) {
	VOXEL_PROFILE_SCOPE();

	struct SetCompleteOnScopeExit {
		std::atomic_bool &_complete;
		SetCompleteOnScopeExit(std::atomic_bool &b) : _complete(b) {}
		~SetCompleteOnScopeExit() {
			_complete = true;
		}
	};

#ifdef DEV_ENABLED
	CRASH_COND(_update_data == nullptr);
	CRASH_COND(_data == nullptr);
	CRASH_COND(_streaming_dependency == nullptr);
	CRASH_COND(_meshing_dependency == nullptr);
	CRASH_COND(_shared_viewers_data == nullptr);
#endif

	VoxelLodTerrainUpdateData &update_data = *_update_data;
	VoxelLodTerrainUpdateData::State &state = update_data.state;
	const VoxelLodTerrainUpdateData::Settings &settings = update_data.settings;
	VoxelData &data = *_data;
	Ref<VoxelGenerator> generator = _streaming_dependency->generator;
	Ref<VoxelStream> stream = _streaming_dependency->stream;
	ProfilingClock profiling_clock;
	ProfilingClock profiling_clock_total;

	// TODO 这不是一个好名字，"streaming" 有多种含义。改名为 "can_load"？
	const bool stream_enabled =
			((stream.is_valid() && stream->is_runnable()) || (generator.is_valid() && generator->is_runnable()));

	const unsigned int lod_count = data.get_lod_count();

#ifdef DEV_ENABLED
	// 确保主线程已处理上一次线程更新任务的输出
	for (unsigned int lod_index = 0; lod_index < state.lods.size(); ++lod_index) {
		const VoxelLodTerrainUpdateData::Lod &lod = state.lods[lod_index];
		CRASH_COND(lod.mesh_blocks_to_unload.size() != 0);
		CRASH_COND(lod.mesh_blocks_to_update_transitions.size() != 0);
		CRASH_COND(lod.mesh_blocks_to_activate_visuals.size() != 0);
		CRASH_COND(lod.mesh_blocks_to_deactivate_visuals.size() != 0);
		CRASH_COND(lod.mesh_blocks_to_activate_collision.size() != 0);
		CRASH_COND(lod.mesh_blocks_to_deactivate_collision.size() != 0);
	}
#endif

	SetCompleteOnScopeExit scoped_complete(update_data.task_is_complete);

	CRASH_COND_MSG(update_data.task_is_complete, "Expected only one update task to run on a given volume");
	MutexLock mutex_lock(update_data.completion_mutex);

	// 更新因编辑产生的挂起 LOD 数据修改。
	// 这些修改从编辑中延迟处理，以便我们可以批量执行。
	// 它必须首先发生，因为数据块随后可能被卸载。
	// 这也是编辑后网格更新的原因。
	flush_pending_lod_edits(state, data, 1 << settings.mesh_block_size_po2);

	// 其它网格更新
	process_changed_generated_areas(state, settings, lod_count);

	static thread_local StdVector<VoxelData::BlockToSave> tls_data_blocks_to_save;
	static thread_local StdVector<VoxelLodTerrainUpdateData::BlockToLoad> tls_data_blocks_to_load;

	StdVector<VoxelLodTerrainUpdateData::BlockToLoad> &data_blocks_to_load = tls_data_blocks_to_load;
	data_blocks_to_load.clear();

	StdVector<VoxelData::BlockToSave> *data_blocks_to_save = stream.is_valid() ? &tls_data_blocks_to_save : nullptr;

	profiling_clock.restart();
	if (settings.streaming_system == VoxelLodTerrainUpdateData::STREAMING_SYSTEM_LEGACY_OCTREE) {
		process_octree_streaming(
				state, data, _viewer_pos, data_blocks_to_save, data_blocks_to_load, settings, stream_enabled
		);
	} else {
		process_clipbox_streaming(
				state,
				data,
				to_span(update_data.viewers),
				_volume_transform,
				data_blocks_to_save,
				data_blocks_to_load,
				settings,
				stream_enabled,
				_meshing_dependency->mesher.is_valid()
		);
	}
	state.stats.time_detect_required_blocks = profiling_clock.restart();

	BufferedTaskScheduler &task_scheduler = BufferedTaskScheduler::get_for_current_thread();

	process_async_edits(
			state,
			settings,
			_data,
			_volume_id,
			_streaming_dependency,
			_shared_viewers_data,
			_volume_transform,
			task_scheduler
	);

	profiling_clock.restart();
	{
		VOXEL_PROFILE_SCOPE_NAMED("IO requests");
		// 用户可能尚未设置流，或流已被关闭
		if (stream_enabled) {
			const unsigned int data_block_size = data.get_block_size();

			// 即使没有该检查，这部分仍会“工作”，因为 `data_blocks_to_load` 会是空的，
			// 但我添加它是为了明确
			if (data.is_streaming_enabled()) {
				if (stream.is_null() && !settings.cache_generated_blocks) {
					// TODO 优化：不太理想，因为有一点延迟。网格请求需要再经过一个更新周期
					// 才能发出。我们也可以直接设置这些空数据块，而不是将它们放入
					// 该列表，但目前这样写代码更简单。
					apply_block_data_requests_as_empty(to_span(data_blocks_to_load), data, state, settings);

				} else {
					send_block_data_requests(
							_volume_id,
							to_span(data_blocks_to_load),
							_streaming_dependency,
							_data,
							_shared_viewers_data,
							data_block_size,
							_volume_transform,
							settings,
							task_scheduler,
							state
					);
				}
			}

			if (data_blocks_to_save != nullptr) {
				send_block_save_requests(
						_volume_id, to_span(*data_blocks_to_save), _streaming_dependency, task_scheduler, nullptr, false
				);
			}
		}
		data_blocks_to_load.clear();
		if (data_blocks_to_save != nullptr) {
			data_blocks_to_save->clear();
		}
	}
	state.stats.time_io_requests = profiling_clock.restart();

	// TODO 当未分配网格化器时，网格请求仍会累积但不会发送。更好的支持方式是
	// 允许仅体素/无网格的观察者，类似于 VoxelTerrain
	if (_meshing_dependency->mesher.is_valid()) {
		send_mesh_requests(
				_volume_id,
				state,
				settings,
				_data,
				_meshing_dependency,
				_shared_viewers_data,
				_volume_transform,
				task_scheduler
		);
	}

	task_scheduler.flush();

	state.stats.time_mesh_requests = profiling_clock.restart();

	state.stats.time_total = profiling_clock.restart();
}

} // namespace voxel
