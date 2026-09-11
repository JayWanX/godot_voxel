#include "generate_column_multipass_task.h"
#include "../../engine/buffered_task_scheduler.h"
#include "../../engine/voxel_engine.h"
#include "../../storage/voxel_data.h"
#include "../../util/containers/std_vector.h"
#include "../../util/dstack.h"
#include "../../util/math/vector2i.h"
#include <core/os/time.h>
#include "../../util/string/format.h"

namespace voxel {

namespace {
#ifdef VOXEL_PROFILER_ENABLED
std::atomic_int g_task_count[VoxelGeneratorMultipassCB::MAX_SUBPASSES] = { 0 };
const char *g_profiling_task_names[VoxelGeneratorMultipassCB::MAX_SUBPASSES] = {
	"GenerateColumnMultipassTasks_subpass0", //
	"GenerateColumnMultipassTasks_subpass1", //
	"GenerateColumnMultipassTasks_subpass2", //
	"GenerateColumnMultipassTasks_subpass3", //
	"GenerateColumnMultipassTasks_subpass4", //
	"GenerateColumnMultipassTasks_subpass5", //
	"GenerateColumnMultipassTasks_subpass6", //
};
#endif

} // namespace

using namespace VoxelGeneratorMultipassCBStructs;

GenerateColumnMultipassTask::GenerateColumnMultipassTask(
		Vector2i p_column_position,
		VoxelFormat p_format,
		uint8_t p_block_size,
		uint8_t p_subpass_index,
		std::shared_ptr<Internal> p_generator_internal,
		Ref<VoxelGeneratorMultipassCB> p_generator,
		TaskPriority p_priority,
		IThreadedTask *p_caller,
		std::shared_ptr<std::atomic_int> p_caller_dependency_count
) {
	_column_position = p_column_position;
	_format = p_format;
	_priority = p_priority;
	_block_size = p_block_size;
	_subpass_index = p_subpass_index;

	VOXEL_ASSERT(p_generator.is_valid());
	_generator = p_generator;

	VOXEL_ASSERT(p_generator_internal != nullptr);
	_generator_internal = p_generator_internal;

	VOXEL_ASSERT(p_caller != nullptr);
	_caller_task = p_caller;
	_caller_task_dependency_counter = p_caller_dependency_count;

#ifdef VOXEL_PROFILER_ENABLED
	int64_t v = ++g_task_count[_subpass_index];
	VOXEL_PROFILE_PLOT(g_profiling_task_names[_subpass_index], v);
#endif

	// println(format("K {} {} {} {} {}", int(_subpass_index), _column_position.x, 0, _column_position.y,
	// 		Time::get_singleton()->get_ticks_usec()));
}

GenerateColumnMultipassTask::~GenerateColumnMultipassTask() {
	VOXEL_ASSERT(_caller_task == nullptr);

#ifdef VOXEL_PROFILER_ENABLED
	int64_t v = --g_task_count[_subpass_index];
	VOXEL_PROFILE_PLOT(g_profiling_task_names[_subpass_index], v);
#endif

	// println(format("J {} {} {} {} {}", int(_subpass_index), _column_position.x, 0, _column_position.y,
	// 		Time::get_singleton()->get_ticks_usec()));
}

void GenerateColumnMultipassTask::run(ThreadedTaskContext &ctx) {
	VOXEL_DSTACK();
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(_generator.is_valid());

	Map &map = _generator_internal->map;
	BufferedTaskScheduler &task_scheduler = BufferedTaskScheduler::get_for_current_thread();

	const int final_subpass_index =
			VoxelGeneratorMultipassCB::get_subpass_count_from_pass_count(_generator_internal->passes.size()) - 1;

	if (_cancelled) {
		// 至少一个子任务被取消，因此我们也必须清理并返回。

		// 如果有的话，从列中注销
		{
			if (!map.spatial_lock.try_lock_write(BoxBounds2i::from_position(_column_position))) {
				// 稍后重试（有趣的情况，但这就是模式）
				ctx.status = ThreadedTaskContext::STATUS_POSTPONED;
				return;
			}
			SpatialLock2D::UnlockWriteOnScopeExit swlock(
					map.spatial_lock, BoxBounds2i::from_position(_column_position)
			);

			MutexLock mlock(map.mutex);
			auto column_it = map.columns.find(_column_position);
			if (column_it != map.columns.end()) {
				// 从列中注销任务
				Column &column = column_it->second;
				column.pending_subpass_tasks_mask &= ~(1 << _subpass_index);

				if (_subpass_index == final_subpass_index) {
					// 调度挂起的数据块请求，让它们处理取消
					schedule_final_block_tasks(column, task_scheduler);
				}
			}
		}

		return_to_caller(false);
		task_scheduler.flush();
		return;
	}

	const int pass_index = VoxelGeneratorMultipassCB::get_pass_index_from_subpass(_subpass_index);
	const Pass &pass = _generator_internal->passes[pass_index];

	if (_subpass_index == 0) {
		// 第一个子 pass 不能依赖另一个子 pass
		VOXEL_ASSERT(pass.dependency_extents == 0);
	} else {
		VOXEL_ASSERT(pass.dependency_extents > 0);
	}

	const Box2i neighbors_box = Box2i::from_min_max(
			_column_position - Vector2iUtil::create(pass.dependency_extents),
			_column_position + Vector2iUtil::create(pass.dependency_extents + 1)
	);

	const unsigned int central_block_index =
			Vector2iUtil::get_yx_index(Vector2iUtil::create(pass.dependency_extents), neighbors_box.size);

	StdVector<Column *> columns;
	// TODO 缓存内存
	columns.reserve(Vector2iUtil::get_area(neighbors_box.size));

	// 锁定我们要处理的区域
	{
		VOXEL_PROFILE_SCOPE_NAMED("Region");

		// 阻塞直到可用会造成瓶颈。不一定总是大瓶颈，但足以在
		// 分析器中非常显眼。
		// SpatialLock3D::Write swlock(map->spatial_lock, neighbors_box);
		if (!map.spatial_lock.try_lock_write(neighbors_box)) {
			// 稍后重试
			ctx.status = ThreadedTaskContext::STATUS_POSTPONED;
			return;
		}
		// 有时我希望 C++ 也有 `defer`
		SpatialLock2D::UnlockWriteOnScopeExit swlock(map.spatial_lock, neighbors_box);

		// 从地图中获取列
		{
			VOXEL_PROFILE_SCOPE_NAMED("Fetch columns");

			// TODO 我们不从这里创建新列，能否使用共享锁？
			MutexLock mlock(map.mutex);

			// 坐标顺序很重要（注意，这里的 Vector2i 中的 Y 对应 3D 中的 Z）。
			neighbors_box.for_each_cell_yx([&columns, &map](Vector2i cpos) {
				auto it = map.columns.find(cpos);
				Column *column = nullptr;
				if (it != map.columns.end()) {
					column = &it->second;
				}
				columns.push_back(column);
			});
		}

		const int subpass_index = _subpass_index;
		const int prev_subpass_index = subpass_index - 1;

		Column *main_column = columns[central_block_index];

		bool spawned_subtasks = false;
		bool postpone = false;

		// 检查加载级别
		{
			VOXEL_PROFILE_SCOPE_NAMED("Check levels");

			Vector2i cpos;

			const Vector2i cpos_min = neighbors_box.position;
			const Vector2i cpos_max = neighbors_box.position + neighbors_box.size;

			std::shared_ptr<std::atomic_int> dependency_counter = nullptr;

			for (Column *column : columns) {
				if (column == nullptr) {
					// 不再加载，我们必须取消任务

					if (main_column != nullptr) {
						main_column->pending_subpass_tasks_mask &= ~(1 << _subpass_index);

						if (_subpass_index == final_subpass_index) {
							// 调度挂起的数据块请求，让它们处理取消
							schedule_final_block_tasks(*main_column, task_scheduler);
						}
					}

					return_to_caller(false);
					task_scheduler.flush();
					return;
				}
			}

			// VOXEL_ASSERT(!has_duplicate(to_span_const(columns)));

			unsigned int i = 0;
			for (cpos.y = cpos_min.y; cpos.y < cpos_max.y; ++cpos.y) {
				for (cpos.x = cpos_min.x; cpos.x < cpos_max.x; ++cpos.x) {
					Column *column = columns[i];
					VOXEL_ASSERT(column != nullptr);

					// 我们希望邻居中的所有数据块至少处于上一个子 pass，然后才能
					// 运行当前子 pass
					if (prev_subpass_index >= 0 && column->subpass_index < prev_subpass_index) {
						// 依赖尚未就绪。

						if (column->loading) {
							// 有一个任务正在处理依赖，所以我们等待。
							// TODO 理想情况下，我们应该订阅该任务的完成。
							// println(format("O {} {} {} {} {}", int(_subpass_index), _column_position.x, 0,
							// 		_column_position.y, Time::get_singleton()->get_ticks_usec()));
							postpone = true;

						} else if ((column->pending_subpass_tasks_mask & (1 << prev_subpass_index)) != 0) {
							// 有一个任务正在处理依赖，所以我们等待。
							// TODO 理想情况下，我们应该订阅该任务的完成。
							// 我们可以做到
							// println(format("O {} {} {} {} {}", int(_subpass_index), _column_position.x, 0,
							// 		_column_position.y, Time::get_singleton()->get_ticks_usec()));
							postpone = true;

						} else {
							// 没有任务在处理依赖，生成一个。

							if (dependency_counter == nullptr) {
								dependency_counter = make_shared_instance<std::atomic_int>();
							}
							++(*dependency_counter);

							GenerateColumnMultipassTask *subtask = VOXEL_NEW(GenerateColumnMultipassTask(
									cpos,
									_format,
									_block_size,
									prev_subpass_index,
									_generator_internal,
									_generator,
									_priority,
									this,
									dependency_counter
							));
							subtask->_caller_mp_task = this;
							task_scheduler.push_main_task(subtask);

							column->pending_subpass_tasks_mask |= (1 << prev_subpass_index);

							spawned_subtasks = true;
						}
						// TODO 如果某列在返回一次后被释放，则重新启动其生成过程。
						// 这旨在覆盖列的数据块被多次请求的情况。理想情况下
						// 这不应发生，但现实世界一团糟：
						// - 游戏可能崩溃了
						// - 保存可能失败了
						// - 文件可能被删除了
						// - 游戏可能只是想要重置某个区域
					}

					++i;
				}
			}
		}

		if (spawned_subtasks) {
			ctx.status = ThreadedTaskContext::STATUS_TAKEN_OUT;

		} else if (postpone) {
			ctx.status = ThreadedTaskContext::STATUS_POSTPONED;
			return;

		} else {
			VOXEL_PROFILE_SCOPE_NAMED("Run pass");
			// 我们可以运行 pass

			VOXEL_ASSERT(main_column != nullptr);

			if (main_column->subpass_index == prev_subpass_index) {
				const int column_height_blocks = _generator_internal->column_height_blocks;

				if (_subpass_index == 0) {
					// 第一个 pass 创建数据块
					// main_column->blocks.resize(column_height_blocks);
					for (Block &block : main_column->blocks) {
						block.voxels.create(Vector3iUtil::create(_block_size), &_format);
					}
				}

				// 调试检查
				// const int subpass_completion_iterations = math::cubed(pass.dependency_extents * 2 + 1);
				// VOXEL_ASSERT(main_block->subpass_iterations[_subpass_index] < subpass_completion_iterations);

				// 我们将 pass 分成两个，并不意味着每次都该再次运行生成器。相反，
				// 我们可能只在引用给定 pass 的第一个子 pass 上运行生成器。
				// TODO 这意味着我们生成的某些任务实际上并不做繁重的工作。能否进一步简化？
				const int prev_pass_index = VoxelGeneratorMultipassCB::get_pass_index_from_subpass(prev_subpass_index);

				if (pass_index == 0 || prev_pass_index != pass_index) {
					const int column_base_y_blocks = _generator_internal->column_base_y_blocks;

					// TODO 缓存内存
					StdVector<Block *> blocks;
					blocks.reserve(columns.size() * column_height_blocks);
					// 按 ZXY 索引组成数据块网格（索引+1 沿 Y 上升）。
					// 这里 ZXY 索引很方便，因为列用 YX（即 ZX，因为 2D 中的 Y 是 3D 中的 Z）索引。
					for (Column *column : columns) {
						for (Block &block : column->blocks) {
							blocks.push_back(&block);
						}
					}

					PassInput input;
					input.grid = to_span(blocks);
					input.grid_size = Vector3i(neighbors_box.size.x, column_height_blocks, neighbors_box.size.y);
					input.grid_origin =
							Vector3i(neighbors_box.position.x, column_base_y_blocks, neighbors_box.position.y);
					input.main_block_position = Vector3i(_column_position.x, column_base_y_blocks, _column_position.y);
					input.pass_index = pass_index;
					input.block_size = _block_size;

					// 这应该是使用 `_generator` 的唯一位置。
					_generator->generate_pass(input);
				}

				// 更新级别
				main_column->subpass_index = _subpass_index;

				// for (Column *column : columns) {
				// 	column->subpass_iterations[_subpass_index]++;
				// }

				// Pass, X, Y, Z, Time
				// println(format("P {} {} {} {} {}", int(_subpass_index), _column_position.x, 0, _column_position.y,
				// 		Time::get_singleton()->get_ticks_usec()));
			}

			main_column->pending_subpass_tasks_mask &= ~(1 << _subpass_index);

			if (main_column->subpass_index == final_subpass_index) {
				// 所有等待此列完成（且自身未生成列子任务）的任务现在都可以恢复
				schedule_final_block_tasks(*main_column, task_scheduler);
			}

			return_to_caller(true);
		}

	} // 区域锁

	// println(format("End of {} t {}", uint64_t(this), Thread::get_caller_id()));
	task_scheduler.flush();
}

void GenerateColumnMultipassTask::schedule_final_block_tasks(Column &column, BufferedTaskScheduler &task_scheduler) {
	for (Block &block : column.blocks) {
		if (block.final_pending_task != nullptr) {
			VOXEL_ASSERT(block.final_pending_task != _caller_task);
			task_scheduler.push_main_task(block.final_pending_task);
			block.final_pending_task = nullptr;
		}
	}
}

void GenerateColumnMultipassTask::return_to_caller(bool success) {
	VOXEL_ASSERT(_caller_task != nullptr);
	VOXEL_ASSERT(_caller_task_dependency_counter != nullptr);
	const int counter = --(*_caller_task_dependency_counter);
	VOXEL_ASSERT(counter >= 0);
	if (!success) {
		if (_caller_mp_task != nullptr) {
			_caller_mp_task->_cancelled = true;
		}
		// println(format("C {} {} {} {} {}", int(_subpass_index), _column_position.x, 0, _column_position.y,
		// 		Time::get_singleton()->get_ticks_usec()));
	}
	if (counter == 0) {
		VoxelEngine::get_singleton().push_async_task(_caller_task);
	}
	_caller_task = nullptr;
}

} // namespace voxel
