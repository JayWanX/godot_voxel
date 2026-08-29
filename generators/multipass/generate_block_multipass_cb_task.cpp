#include "generate_block_multipass_cb_task.h"
#include "../../engine/voxel_engine.h"
#include "../../generators/multipass/generate_column_multipass_task.h"
#include "../../generators/multipass/voxel_generator_multipass_cb.h"
#include "../../storage/voxel_buffer.h"
#include "../../storage/voxel_data.h"
#include "../../streams/save_block_data_task.h"
#include "../../util/dstack.h"
#include "../../util/godot/classes/time.h"
#include "../../util/io/log.h"
#include "../../util/math/conv.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"
#include "../../util/tasks/async_dependency_tracker.h"

namespace voxel {

GenerateBlockMultipassCBTask::GenerateBlockMultipassCBTask(const VoxelGenerator::BlockTaskParams &params) :
		_block_position(params.block_position),
		_format(params.format),
		_volume_id(params.volume_id),
		_lod_index(params.lod_index),
		_block_size(params.block_size),
		_drop_beyond_max_distance(params.drop_beyond_max_distance),
		_priority_dependency(params.priority_dependency),
		_stream_dependency(params.stream_dependency),
		_tracker(params.tracker),
		_cancellation_token(params.cancellation_token) {
	//
	VoxelEngine::get_singleton().debug_increment_generate_block_task_counter();
}

GenerateBlockMultipassCBTask::~GenerateBlockMultipassCBTask() {
	VoxelEngine::get_singleton().debug_decrement_generate_block_task_counter();
	// println(format("H {} {} {} {}", position.x, position.y, position.z, Time::get_singleton()->get_ticks_usec()));
}

void GenerateBlockMultipassCBTask::run(voxel::ThreadedTaskContext &ctx) {
	VOXEL_DSTACK();
	VOXEL_PROFILE_SCOPE();

	CRASH_COND(_stream_dependency == nullptr);
	Ref<VoxelGenerator> generator = _stream_dependency->generator;
	ERR_FAIL_COND(generator.is_null());

	// TODO 想办法让生成器提供自己的任务，而不是在这里硬塞
	Ref<VoxelGeneratorMultipassCB> multipass_generator = generator;
	VOXEL_ASSERT_RETURN(multipass_generator.is_valid());

	VOXEL_ASSERT_RETURN(multipass_generator->get_pass_count() > 0);
	std::shared_ptr<VoxelGeneratorMultipassCBStructs::Internal> multipass_generator_internal =
			multipass_generator->get_internal();
	VoxelGeneratorMultipassCBStructs::Map &map = multipass_generator_internal->map;

	const int final_subpass_index =
			VoxelGeneratorMultipassCB::get_subpass_count_from_pass_count(multipass_generator->get_pass_count()) - 1;

	const int column_min_y = multipass_generator->get_column_base_y_blocks();
	const int column_height = multipass_generator->get_column_height_blocks();

	if (_block_position.y < column_min_y || _block_position.y >= column_min_y + column_height) {
		// 回退到单 pass

	} else {
		const Vector2i column_position(_block_position.x, _block_position.z);
		// TODO 可推迟的候选？数量很多，可能会引起争用
		SpatialLock2D::Read srlock(map.spatial_lock, BoxBounds2i::from_position(column_position));
		VoxelGeneratorMultipassCBStructs::Column *column = nullptr;
		{
			MutexLock mlock(map.mutex);
			auto column_it = map.columns.find(column_position);
			if (column_it == map.columns.end()) {
				// 丢弃，出于某种原因它不可用
				return;
			}

			column = &column_it->second;
		}

		if (column == nullptr) {
			// 丢弃。
			// 要么是我们在子任务被取消后返回这里，要么是目标列在任务开始运行前被卸载了。
			return;
		}

		const int block_index = _block_position.y - column_min_y;

		if (column->subpass_index != final_subpass_index) {
			// 数据块尚未完成
			if (_stage == 1) {
				// 我们在调度子任务后回到这里，意味着它们不得不取消。
				// 丢弃
				// TODO 我们真的需要想办法简化这种类似协程的任务流程
				return;
			}

			VoxelGeneratorMultipassCBStructs::Block &block = column->blocks[block_index];

			if (block.final_pending_task != nullptr) {
				// 该数据块已有一个生成任务。

				// 它绝不能是当前任务。只有未调度、未运行的任务才能存储
				// 在这里。如果是当前任务，那就出问题了。
				VOXEL_ASSERT(block.final_pending_task != this);

				// 如果你向前传送，再回去，然后再向前传送，就可能发生这种情况。
				// 因为 VoxelTerrain 会忘记"加载中的数据块"落出其活动区域，而
				// 多 pass 生成器会在更大的半径内忘记它们，因此会导致同一数据块
				// 在前一个请求仍被缓存、尚未运行时被再次请求。
				// 我们不能删除这个任务。

				// 丢弃当前任务，我们应让现有任务来完成工作
				return;
				// TODO 不确定如果已有任务，我们是否应该让地形将其视为丢弃！
				// 即使已有任务在等待，它也可能导致请求循环
			}

			if ((column->pending_subpass_tasks_mask & (1 << final_subpass_index)) == 0) {
				// 没有任务在处理它，而我们是第一个顶层任务。
				// 生成一个子任务，把这一列带到最终状态。
				GenerateColumnMultipassTask *subtask = VOXEL_NEW(GenerateColumnMultipassTask(
						column_position,
						_format,
						_block_size,
						final_subpass_index,
						multipass_generator_internal,
						multipass_generator,
						ctx.task_priority,
						// 子任务取得当前任务的所有权，它会在完成（或取消）时
						// 将其调度回来
						this,
						make_shared_instance<std::atomic_int>(1)
				));

				column->pending_subpass_tasks_mask |= (1 << final_subpass_index);

				VoxelEngine::get_singleton().push_async_task(subtask);

			} else {
				// 一个列任务已经在进行中。
				// 注册为等待该列的最终结果。数据块取得所有权。
				block.final_pending_task = this;
			}

			// 所有权现在要么交给子任务，要么交给目标数据块。
			// 因此，我们必须从任务运行器中取出。当列完成或卸载时，
			// 我们会再次被调度。
			ctx.status = ThreadedTaskContext::STATUS_TAKEN_OUT;
			_stage = 1;
			// println(format("Takeout {}", uint64_t(this)));

		} else {
			// 数据块已就绪

			VoxelGeneratorMultipassCBStructs::Block &block = column->blocks[block_index];

			// TODO 从该数据块中取出体素数据以节省内存，它不能再被生成触碰
			// （如果这样做，我们需要将列的空间锁改为 WRITE）
			voxels = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
			voxels->create(block.voxels.get_size());
			voxels->copy_format(block.voxels);
			voxels->copy_channels_from(block.voxels);
			voxels->copy_voxel_metadata(block.voxels);

			run_stream_saving_and_finish();
		}
		return;
	}

	// 单 pass 生成

	if (voxels == nullptr) {
		voxels = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
		voxels->create(Vector3iUtil::create(_block_size), &_format);
	}

	run_cpu_generation();
	run_stream_saving_and_finish();
}

void GenerateBlockMultipassCBTask::run_cpu_generation() {
	const Vector3i origin_in_voxels = (_block_position << _lod_index) * _block_size;

	Ref<VoxelGenerator> generator = _stream_dependency->generator;

	VoxelGenerator::VoxelQueryData query_data{ *voxels, origin_in_voxels, _lod_index };
	generator->generate_block(query_data);
}

void GenerateBlockMultipassCBTask::run_stream_saving_and_finish() {
	if (_stream_dependency->valid) {
		Ref<VoxelStream> stream = _stream_dependency->stream;

		// TODO 在某些情况下我们并不想一直运行这个，对吧？
		// 比如在全加载模式下，未编辑的数据块会即时生成……
		if (stream.is_valid() && stream->get_save_generator_output()) {
			VOXEL_PRINT_VERBOSE(
					format("Requesting save of generator output for block {} lod {}", _block_position, int(_lod_index))
			);

			// TODO 优化：`voxels` 实际上不需要共享
			std::shared_ptr<VoxelBuffer> voxels_copy = make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
			voxels->copy_to(*voxels_copy, true);

			// 没有实例，生成器在这个阶段还不设计为产生它们。
			// 没有优先级数据，保存不需要排序。

			SaveBlockDataTask *save_task = VOXEL_NEW(SaveBlockDataTask(
					_volume_id, _block_position, _lod_index, voxels_copy, _stream_dependency, nullptr, false
			));

			VoxelEngine::get_singleton().push_async_io_task(save_task);
		}
	}

	_has_run = true;
}

TaskPriority GenerateBlockMultipassCBTask::get_priority() {
	float closest_viewer_distance_sq;
	const TaskPriority p = _priority_dependency.evaluate(
			_lod_index, constants::TASK_PRIORITY_GENERATE_BAND2, &closest_viewer_distance_sq
	);
	_too_far = _drop_beyond_max_distance && closest_viewer_distance_sq > _priority_dependency.drop_distance_squared;
	return p;
}

bool GenerateBlockMultipassCBTask::is_cancelled() {
	if (_stream_dependency->valid == false) {
		return true;
	}
	if (_cancellation_token.is_valid()) {
		return _cancellation_token.is_cancelled();
	}
	return _too_far; // || stream_dependency->stream->get_fallback_generator().is_null();
}

void GenerateBlockMultipassCBTask::apply_result() {
	bool aborted = true;

	if (VoxelEngine::get_singleton().is_volume_valid(_volume_id)) {
		// TODO 比较指针可能不保证有效
		// 请求响应必须与请求时所带的依赖匹配。
		// 如果不匹配，我们就不再对结果感兴趣。
		if (_stream_dependency->valid) {
			Ref<VoxelStream> stream = _stream_dependency->stream;

			VoxelEngine::BlockDataOutput o;
			o.voxels = voxels;
			o.position = _block_position;
			o.lod_index = _lod_index;
			o.dropped = !_has_run;
			if (stream.is_valid() && stream->get_save_generator_output()) {
				// 我们不能把该数据块视为"已生成"，因为一旦保存，就没有状态可以说明这一点，
				// 所以必须把它视为已编辑的数据块
				o.type = VoxelEngine::BlockDataOutput::TYPE_LOADED;
			} else {
				o.type = VoxelEngine::BlockDataOutput::TYPE_GENERATED;
			}
			o.max_lod_hint = false;
			o.initial_load = false;

			VoxelEngine::VolumeCallbacks callbacks = VoxelEngine::get_singleton().get_volume_callbacks(_volume_id);
			ERR_FAIL_COND(callbacks.data_output_callback == nullptr);
			callbacks.data_output_callback(callbacks.data, o);

			aborted = !_has_run;
		}

	} else {
		// 如果用户在请求即将返回时移除了体积，就可能发生这种情况
		VOXEL_PRINT_VERBOSE("Gemerated data request response came back but volume wasn't found");
	}

	// TODO 如果我们能访问将数据块写入其中的数据结构，就可以更早地在 run() 中完成。
	// 这可以稍微减少延迟。地形需要用生成的数据块做的其余事情可以稍后运行。
	if (_tracker != nullptr) {
		if (aborted) {
			_tracker->abort();
		} else {
			_tracker->post_complete();
		}
	}
}

} // namespace voxel
