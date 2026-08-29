#include "voxel_engine.h"
#include "../constants/voxel_constants.h"
#include "../generators/generate_block_task.h"
#include "../meshers/mesh_block_task.h"
#include "../streams/load_all_blocks_data_task.h"
#include "../streams/load_block_data_task.h"
#include "../streams/save_block_data_task.h"
#include "../util/godot/classes/display_server.h"
#include "../util/godot/classes/os.h"
#include "../util/godot/classes/project_settings.h"
#include "../util/godot/classes/rd_sampler_state.h"
#include "../util/godot/classes/rendering_server.h"
#include "../util/io/log.h"
#include "../util/macros.h"
#include "../util/math/conv.h"
#include "../util/profiling.h"
#include "../util/string/format.h"

namespace voxel {

VoxelEngine *g_voxel_engine = nullptr;

VoxelEngine &VoxelEngine::get_singleton() {
	VOXEL_ASSERT_MSG(g_voxel_engine != nullptr, "Accessing singleton while it's null");
	return *g_voxel_engine;
}

void VoxelEngine::create_singleton(Config config) {
	VOXEL_ASSERT_MSG(g_voxel_engine == nullptr, "Creating singleton twice");
	g_voxel_engine = VOXEL_NEW(VoxelEngine(config));
}

void VoxelEngine::destroy_singleton() {
	VOXEL_ASSERT_MSG(g_voxel_engine != nullptr, "Destroying singleton twice");
	VOXEL_DELETE(g_voxel_engine);
	g_voxel_engine = nullptr;
}

VoxelEngine::VoxelEngine(Config config) {
	const int hw_threads_hint = Thread::get_hardware_concurrency();
	VOXEL_PRINT_VERBOSE(format("Voxel: HW threads hint: {}", hw_threads_hint));

	VOXEL_ASSERT(config.thread_count_margin_below_max >= 0);
	VOXEL_ASSERT(config.thread_count_minimum >= 1);
	VOXEL_ASSERT(config.thread_count_ratio_over_max >= 0.f);

	// 计算通用线程池的线程数。
	// 注意 I/O 线程算作一个已占用线程，并且始终存在。

	const int maximum_thread_count =
			math::max(hw_threads_hint - config.thread_count_margin_below_max, config.thread_count_minimum);
	const int thread_count_by_ratio = int(Math::round(float(config.thread_count_ratio_over_max) * hw_threads_hint));
	const int thread_count = math::clamp(thread_count_by_ratio, config.thread_count_minimum, maximum_thread_count);
	VOXEL_PRINT_VERBOSE(format("Voxel: automatic thread count set to {}", thread_count));

	if (thread_count > hw_threads_hint) {
		VOXEL_PRINT_WARNING("Configured thread count exceeds hardware thread count. Performance may not be optimal");
	}

	_general_thread_pool.set_name("Voxel general");
	_general_thread_pool.set_thread_count(thread_count);
	_general_thread_pool.set_priority_update_period(200);

	// 初始化世界
	_world.shared_priority_dependency = make_shared_instance<PriorityDependency::ViewersData>();
	// 提供初始容量以减少失效的可能性
	_world.shared_priority_dependency->viewers.resize(64);

	VOXEL_PRINT_VERBOSE(format("Size of LoadBlockDataTask: {}", sizeof(LoadBlockDataTask)));
	VOXEL_PRINT_VERBOSE(format("Size of SaveBlockDataTask: {}", sizeof(SaveBlockDataTask)));
	VOXEL_PRINT_VERBOSE(format("Size of MeshBlockTask: {}", sizeof(MeshBlockTask)));

	set_main_thread_time_budget_usec(config.main_thread_budget_usec);
}

VoxelEngine::~VoxelEngine() {
	// GDScriptLanguage 单例可能在我们之前被销毁，因此任务引用的任何脚本都无法被释放。
	// 为绕开此问题，任务会在场景树自动加载被销毁时清除。
	// 所以正常情况下这里不应还有任务需要清除，
	// 但为了正确性还是执行清理，本就应该如此……
	wait_and_clear_all_tasks(true);

#ifdef VOXEL_ENABLE_GPU
	_gpu_task_runner.stop();
#endif
}

static bool auto_detect_threaded_graphics_resource_building_support() {
	const ProjectSettings *project = ProjectSettings::get_singleton();
	VOXEL_ASSERT_RETURN_V(project != nullptr, false);

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 7
	// 无头（headless）DisplayServer 会安装哑光栅化器
	// （DisplayServerHeadless::create_func -> RasterizerDummy::make_current()），其存储
	// 不适合多线程并发访问。此时项目的 rendering_method/rendering_driver 设置
	// 仍会报告其配置值，因此无法用它们来检测。
	// 这是对 Godot 一个问题（渲染器在无头模式下非线程安全）的规避方案：
	// https://github.com/godotengine/godot/issues/121949
	// 一旦 Godot 自身修复了该问题，就可以移除这段代码。
	const DisplayServer *display_server = DisplayServer::get_singleton();
	if (display_server == nullptr || display_server->get_name() == "headless") {
		return false;
	}
#endif

	const voxel::godot::RenderThreadModel rendering_thread_model = voxel::godot::get_render_thread_model(*project);
	const voxel::godot::RenderMethod rendering_method = voxel::godot::get_current_rendering_method();
	const voxel::godot::RenderDriverName driver = voxel ::godot::get_current_rendering_driver();

	if (!voxel::godot::is_render_thread_model_safe(rendering_thread_model)) {
		return false;
	}

	switch (rendering_method) {
		case voxel::godot::RENDER_METHOD_GL_COMPATIBILITY:
			return false;
		case voxel::godot::RENDER_METHOD_UNKNOWN:
			return false;
		default:
			break;
	}

	switch (driver) {
		case voxel::godot::RENDER_DRIVER_OPENGL3:
		case voxel::godot::RENDER_DRIVER_OPENGL3_ANGLE:
		case voxel::godot::RENDER_DRIVER_OPENGL3_ES:
		case voxel::godot::RENDER_DRIVER_UNKNOWN:
			return false;
		default:
			return true;
	}
}

void VoxelEngine::try_initialize_gpu_features() {
	_threaded_graphics_resource_building_enabled = auto_detect_threaded_graphics_resource_building_support();
	VOXEL_PRINT_VERBOSE(format(
			"Auto-detected threaded graphics resource building: {}", _threaded_graphics_resource_building_enabled
	));

#ifdef VOXEL_ENABLE_GPU
	if (_gpu_task_runner.is_running() == false) {
		_gpu_task_runner.start();
	}
#endif
}

void VoxelEngine::wait_and_clear_all_tasks(bool warn) {
	_general_thread_pool.wait_for_all_tasks();

	_general_thread_pool.dequeue_completed_tasks([warn](voxel::IThreadedTask *task) {
		if (warn) {
			VOXEL_PRINT_WARNING(
					"General tasks remain on module cleanup, "
					"this could become a problem if they reference scripts"
			);
		}
		VOXEL_DELETE(task);
	});
}

VolumeID VoxelEngine::add_volume(VolumeCallbacks callbacks) {
	VOXEL_ASSERT(callbacks.check_callbacks());
	Volume volume;
	volume.callbacks = callbacks;
	return _world.volumes.add(volume);
}

VoxelEngine::VolumeCallbacks VoxelEngine::get_volume_callbacks(VolumeID volume_id) const {
	const Volume &volume = _world.volumes.get(volume_id);
	return volume.callbacks;
}

void VoxelEngine::remove_volume(VolumeID volume_id) {
	_world.volumes.remove(volume_id);
	// TODO 如何取消网格化任务？

	if (_world.volumes.count() == 0) {
		// 为绕开该问题
		// 当最后一个体被销毁时（例如游戏退出时）
		wait_and_clear_all_tasks(false);
	}
}

bool VoxelEngine::is_volume_valid(VolumeID volume_id) const {
	return _world.volumes.exists(volume_id);
}

ViewerID VoxelEngine::add_viewer() {
	return _world.viewers.add(Viewer());
}

void VoxelEngine::remove_viewer(ViewerID viewer_id) {
	_world.viewers.remove(viewer_id);
}

bool VoxelEngine::get_viewer_count() const {
	return _world.viewers.count();
}

void VoxelEngine::set_viewer_position(ViewerID viewer_id, Vector3 position) {
	Viewer &viewer = _world.viewers.get(viewer_id);
	viewer.world_position = position;
}

void VoxelEngine::set_viewer_distances(ViewerID viewer_id, Viewer::Distances distances) {
	Viewer &viewer = _world.viewers.get(viewer_id);
	viewer.view_distances = distances;
}

VoxelEngine::Viewer::Distances VoxelEngine::get_viewer_distances(ViewerID viewer_id) const {
	const Viewer &viewer = _world.viewers.get(viewer_id);
	return viewer.view_distances;
}

void VoxelEngine::set_viewer_requires_visuals(ViewerID viewer_id, bool enabled) {
	Viewer &viewer = _world.viewers.get(viewer_id);
	viewer.require_visuals = enabled;
}

bool VoxelEngine::is_viewer_requiring_visuals(ViewerID viewer_id) const {
	const Viewer &viewer = _world.viewers.get(viewer_id);
	return viewer.require_visuals;
}

void VoxelEngine::set_viewer_requires_collisions(ViewerID viewer_id, bool enabled) {
	Viewer &viewer = _world.viewers.get(viewer_id);
	viewer.require_collisions = enabled;
}

bool VoxelEngine::is_viewer_requiring_collisions(ViewerID viewer_id) const {
	const Viewer &viewer = _world.viewers.get(viewer_id);
	return viewer.require_collisions;
}

void VoxelEngine::set_viewer_requires_data_block_notifications(ViewerID viewer_id, bool enabled) {
	Viewer &viewer = _world.viewers.get(viewer_id);
	viewer.requires_data_block_notifications = enabled;
}

bool VoxelEngine::is_viewer_requiring_data_block_notifications(ViewerID viewer_id) const {
	const Viewer &viewer = _world.viewers.get(viewer_id);
	return viewer.requires_data_block_notifications;
}

void VoxelEngine::set_viewer_network_peer_id(ViewerID viewer_id, int peer_id) {
	Viewer &viewer = _world.viewers.get(viewer_id);
	viewer.network_peer_id = peer_id;
}

int VoxelEngine::get_viewer_network_peer_id(ViewerID viewer_id) const {
	const Viewer &viewer = _world.viewers.get(viewer_id);
	return viewer.network_peer_id;
}

bool VoxelEngine::viewer_exists(ViewerID viewer_id) const {
	return _world.viewers.exists(viewer_id);
}

void VoxelEngine::push_main_thread_time_spread_task(
		voxel::ITimeSpreadTask *task,
		TimeSpreadTaskRunner::Priority priority
) {
	_time_spread_task_runner.push(task, priority);
}

void VoxelEngine::push_main_thread_progressive_task(voxel::IProgressiveTask *task) {
	_progressive_task_runner.push(task);
}

int VoxelEngine::get_main_thread_time_budget_usec() const {
	return _main_thread_time_budget_usec;
}

void VoxelEngine::set_main_thread_time_budget_usec(unsigned int usec) {
	_main_thread_time_budget_usec = usec;
}

bool VoxelEngine::is_threaded_graphics_resource_building_enabled() const {
	return _threaded_graphics_resource_building_enabled;
}

// void VoxelEngine::set_threaded_graphics_resource_building_enabled(bool enabled) {
// 	_threaded_graphics_resource_building_enabled = enabled;
// }

void VoxelEngine::push_async_task(voxel::IThreadedTask *task) {
	_general_thread_pool.enqueue(task, false);
}

void VoxelEngine::push_async_tasks(Span<voxel::IThreadedTask *> tasks) {
	_general_thread_pool.enqueue(tasks, false);
}

void VoxelEngine::push_async_io_task(voxel::IThreadedTask *task) {
	// I/O 任务串行运行，因为它们通常因锁定共享资源而无法很好地并行运行。
	_general_thread_pool.enqueue(task, true);
}

void VoxelEngine::push_async_io_tasks(Span<voxel::IThreadedTask *> tasks) {
	_general_thread_pool.enqueue(tasks, true);
}

#ifdef VOXEL_ENABLE_GPU
void VoxelEngine::push_gpu_task(IGPUTask *task) {
	_gpu_task_runner.push(task);
}

uint32_t VoxelEngine::get_pending_gpu_tasks_count() const {
	return _gpu_task_runner.get_pending_task_count();
}
#endif

void VoxelEngine::process() {
	VOXEL_PROFILE_SCOPE();
	VOXEL_PROFILE_PLOT("Static memory usage", int64_t(OS::get_singleton()->get_static_memory_usage()));
	VOXEL_PROFILE_PLOT("TimeSpread tasks", int64_t(_time_spread_task_runner.get_pending_count()));
	VOXEL_PROFILE_PLOT("Progressive tasks", int64_t(_progressive_task_runner.get_pending_count()));
	VOXEL_PROFILE_PLOT("Threaded tasks", int64_t(_general_thread_pool.get_debug_remaining_tasks()));
	VOXEL_PROFILE_PLOT("Objects", int64_t(ObjectDB::get_object_count()));
	VOXEL_PROFILE_PLOT(
			"VOXEL Std Allocator",
			int64_t(StdDefaultAllocatorCounters::g_allocated - StdDefaultAllocatorCounters::g_deallocated)
	);

	// 接收生成和网格化结果
	_general_thread_pool.dequeue_completed_tasks([](voxel::IThreadedTask *task) {
		task->apply_result();
		VOXEL_DELETE(task);
	});

	// 在出队线程任务后再运行此步骤，因为它们可能向此执行器添加任务，
	// 这些任务又可能立即完成（这样可避免 1 帧的延迟）。
	_time_spread_task_runner.process(_main_thread_time_budget_usec);

	_progressive_task_runner.process();

	// 更新观察者依赖
	sync_viewers_task_priority_data();

#ifdef VOXEL_ENABLE_GPU
	VOXEL_PROFILE_PLOT("Pending GPU tasks", int64_t(_gpu_task_runner.get_pending_task_count()));
#endif
}

void VoxelEngine::sync_viewers_task_priority_data() {
	const unsigned int viewer_count = _world.viewers.count();

	if (viewer_count > _world.shared_priority_dependency->viewers.size()) {
		// 失效并构建新实例。后续任务将引用它。

		// 一个边缘情况是存在大量存活时间很长的任务堆积（例如用户使用了极慢的生成器）。
		// 这些任务的优先级将不再动态更新，因此某些数据块可能以异常的速度加载。
		// 为绕开此问题，我们可以通过预先分配足够多的元素来尽量减少这种失效发生的次数。
		// 超出容量后该问题会重现，但应该很少见。如果某款游戏需要大量观察者，
		// 我们也许能找到一种无需遍历全部观察者的不同策略？

		// TODO 能否通过使用原子大小或内存屏障来避免失效？
		_world.shared_priority_dependency = make_shared_instance<PriorityDependency::ViewersData>();
		_world.shared_priority_dependency->viewers.resize(viewer_count);
	}

	PriorityDependency::ViewersData &dep = *_world.shared_priority_dependency;

	size_t i = 0;
	unsigned int max_distance = 0;
	_world.viewers.for_each_value([&i, &max_distance, &dep](Viewer &viewer) {
		dep.viewers[i] = to_vec3f(viewer.world_position);
		max_distance = math::max(max_distance, viewer.view_distances.max());
		++i;
	});

	dep.viewers_count = viewer_count;

	// 取消距离被增大出于两个原因：
	// - 某些体使用立方体区域，其角落处的距离更大
	// - 需要迟滞以避免来回切换
	dep.highest_view_distance = max_distance * 2;
}

namespace {

unsigned int debug_get_active_thread_count(const voxel::ThreadedTaskRunner &pool) {
	unsigned int active_count = 0;
	for (unsigned int i = 0; i < pool.get_thread_count(); ++i) {
		voxel::ThreadedTaskRunner::State s = pool.get_thread_debug_state(i);
		if (s == voxel::ThreadedTaskRunner::STATE_RUNNING) {
			++active_count;
		}
	}
	return active_count;
}

VoxelEngine::Stats::ThreadPoolStats debug_get_pool_stats(const voxel::ThreadedTaskRunner &pool) {
	VoxelEngine::Stats::ThreadPoolStats d;
	d.tasks = pool.get_debug_remaining_tasks();
	d.active_threads = debug_get_active_thread_count(pool);
	d.thread_count = pool.get_thread_count();

	fill(d.active_task_names, (const char *)nullptr);
	for (unsigned int i = 0; i < d.thread_count; ++i) {
		d.active_task_names[i] = pool.get_thread_debug_task_name(i);
	}

	return d;
}

} // namespace

VoxelEngine::Stats VoxelEngine::get_stats() const {
	Stats s;
	s.general = debug_get_pool_stats(_general_thread_pool);
	s.generation_tasks = _debug_generate_block_task_count;
	s.meshing_tasks = MeshBlockTask::debug_get_running_count();
	s.streaming_tasks = LoadBlockDataTask::debug_get_running_count() + SaveBlockDataTask::debug_get_running_count();
	s.main_thread_tasks = _time_spread_task_runner.get_pending_count() + _progressive_task_runner.get_pending_count();
#ifdef VOXEL_ENABLE_GPU
	s.gpu_tasks = _gpu_task_runner.get_pending_task_count();
#endif
	return s;
}

int VoxelEngine::get_thread_count() const {
	return _general_thread_pool.get_thread_count();
}

void VoxelEngine::set_thread_count(uint32_t count) {
	_general_thread_pool.set_thread_count(count);
}

} // namespace voxel
