#include "load_block_data_task.h"
#include "../engine/voxel_engine.h"
#include "../generators/generate_block_task.h"
#include "../storage/voxel_buffer_internal.h"
#include "../util/dstack.h"
#include "../util/io/log.h"
#include "../util/profiling.h"

namespace zylann::voxel {

namespace {
std::atomic_int g_debug_load_block_tasks_count = { 0 };
}

LoadBlockDataTask::LoadBlockDataTask(VolumeID p_volume_id, Vector3i p_block_pos, uint8_t p_lod, uint8_t p_block_size,
		bool p_request_instances, std::shared_ptr<StreamingDependency> p_stream_dependency,
		PriorityDependency p_priority_dependency, bool generate_cache_data, bool generator_use_gpu,
		const std::shared_ptr<VoxelData> &vdata) :
		_priority_dependency(p_priority_dependency),
		_position(p_block_pos),
		_volume_id(p_volume_id),
		_lod_index(p_lod),
		_block_size(p_block_size),
		_request_instances(p_request_instances),
		_generate_cache_data(generate_cache_data),
		_generator_use_gpu(generator_use_gpu),
		//_request_voxels(true),
		_stream_dependency(p_stream_dependency),
		_voxel_data(vdata) {
	//
	++g_debug_load_block_tasks_count;
}

LoadBlockDataTask::~LoadBlockDataTask() {
	--g_debug_load_block_tasks_count;
}

int LoadBlockDataTask::debug_get_running_count() {
	return g_debug_load_block_tasks_count;
}

void LoadBlockDataTask::run(zylann::ThreadedTaskContext &ctx) {
	ZN_DSTACK();
	ZN_PROFILE_SCOPE();

	CRASH_COND(_stream_dependency == nullptr);
	Ref<VoxelStream> stream = _stream_dependency->stream;
	CRASH_COND(stream.is_null());

	const Vector3i origin_in_voxels = (_position << _lod_index) * _block_size;

	ERR_FAIL_COND(_voxels != nullptr);
	_voxels = make_shared_instance<VoxelBufferInternal>();
	_voxels->create(_block_size, _block_size, _block_size);

	// TODO We should consider batching this again, but it needs to be done carefully.
	// Each task is one block, and priority depends on distance to closest viewer.
	// If we batch blocks, we have to do it by distance too.

	// TODO Assign max_lod_hint when available

	VoxelStream::VoxelQueryData voxel_query_data{ *_voxels, origin_in_voxels, _lod_index, VoxelStream::RESULT_ERROR };
	stream->load_voxel_block(voxel_query_data);

	if (voxel_query_data.result == VoxelStream::RESULT_ERROR) {
		ERR_PRINT("Error loading voxel block");

	} else if (voxel_query_data.result == VoxelStream::RESULT_BLOCK_NOT_FOUND) {
		if (_generate_cache_data) {
			Ref<VoxelGenerator> generator = _stream_dependency->generator;

			if (generator.is_valid()) {
				VoxelGenerator::BlockTaskParams params;
				params.voxels = _voxels;
				params.volume_id = _volume_id;
				params.block_position = _position;
				params.lod_index = _lod_index;
				params.block_size = _block_size;
				params.stream_dependency = _stream_dependency;
				params.priority_dependency = _priority_dependency;
				params.use_gpu = _generator_use_gpu;
				params.data = _voxel_data;

				IThreadedTask *task = generator->create_block_task(params);

				VoxelEngine::get_singleton().push_async_task(task);
				_requested_generator_task = true;

			} else {
				// If there is no generator... what do we do? What defines the format of that empty block?
				// If the user leaves the defaults it's fine, but otherwise blocks of inconsistent format can
				// end up in the volume and that can cause errors.
				// TODO Define format on volume?
			}
		} else {
			_voxels.reset();
		}
	}

	if (_request_instances && stream->supports_instance_blocks()) {
		ERR_FAIL_COND(_instances != nullptr);

		VoxelStream::InstancesQueryData instances_query;
		instances_query.lod = _lod_index;
		instances_query.position = _position;
		stream->load_instance_blocks(Span<VoxelStream::InstancesQueryData>(&instances_query, 1));

		if (instances_query.result == VoxelStream::RESULT_ERROR) {
			ERR_PRINT("Error loading instance block");

		} else if (voxel_query_data.result == VoxelStream::RESULT_BLOCK_FOUND) {
			_instances = std::move(instances_query.data);
		}
		// If not found, instances will return null,
		// which means it can be generated by the instancer after the meshing process
	}

	_has_run = true;
}

TaskPriority LoadBlockDataTask::get_priority() {
	float closest_viewer_distance_sq;
	const TaskPriority p =
			_priority_dependency.evaluate(_lod_index, constants::TASK_PRIORITY_LOAD_BAND2, &closest_viewer_distance_sq);
	_too_far = closest_viewer_distance_sq > _priority_dependency.drop_distance_squared;
	return p;
}

bool LoadBlockDataTask::is_cancelled() {
	return !_stream_dependency->valid || _too_far;
}

void LoadBlockDataTask::apply_result() {
	if (VoxelEngine::get_singleton().is_volume_valid(_volume_id)) {
		// TODO Comparing pointer may not be guaranteed
		// The request response must match the dependency it would have been requested with.
		// If it doesn't match, we are no longer interested in the result.
		if (_stream_dependency->valid && !_requested_generator_task) {
			VoxelEngine::BlockDataOutput o;
			o.voxels = _voxels;
			o.instances = std::move(_instances);
			o.position = _position;
			o.lod_index = _lod_index;
			o.dropped = !_has_run;
			o.max_lod_hint = _max_lod_hint;
			o.initial_load = false;
			o.type = VoxelEngine::BlockDataOutput::TYPE_LOADED;

			VoxelEngine::VolumeCallbacks callbacks = VoxelEngine::get_singleton().get_volume_callbacks(_volume_id);
			CRASH_COND(callbacks.data_output_callback == nullptr);
			callbacks.data_output_callback(callbacks.data, o);
		}

	} else {
		// This can happen if the user removes the volume while requests are still about to return
		ZN_PRINT_VERBOSE("Stream data request response came back but volume wasn't found");
	}
}

} // namespace zylann::voxel
