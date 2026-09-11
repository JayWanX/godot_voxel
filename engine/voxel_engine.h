#ifndef VOXEL_ENGINE_H
#define VOXEL_ENGINE_H

#include "../meshers/voxel_mesher.h"
#include "../util/containers/slot_map.h"
#include "../util/containers/std_vector.h"
#include "../util/godot/classes/rendering_device.h"
#include "../util/io/file_locker.h"
#include "../util/memory/memory.h"
#include "../util/string/std_string.h"
#include "../util/tasks/progressive_task_runner.h"
#include "../util/tasks/threaded_task_runner.h"
#include "../util/tasks/time_spread_task_runner.h"
#include "ids.h"
#include "priority_dependency.h"

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
#include "detail_rendering/detail_rendering.h"
#endif

#ifdef VOXEL_ENABLE_INSTANCER
#include "../streams/instance_data.h"
#endif

#ifdef VOXEL_ENABLE_GPU
#include "gpu/compute_shader.h"
#include "gpu/gpu_storage_buffer_pool.h"
#include "gpu/gpu_task_runner.h"
#endif

class RenderingDevice;

namespace voxel {

// 用于通用事务的单例，尤其是任务系统和共享的观察者列表。
// 在 Godot 术语中这曾被称为“服务器”（server），但我不太认同这个说法，而且它容易
// 与网络相关功能混淆。
class VoxelEngine {
public:
	struct BlockMeshOutput {
		enum Type {
			TYPE_MESHED, // 包含网格
			TYPE_DROPPED // 表示网格化已被取消
		};

		Type type;
		VoxelMesher::Output surfaces;
		// 仅当 `has_mesh_resource` 为 true 时使用（通常是在允许在线程中构建网格的情况下）。否则，
		// 网格数据将位于 `surfaces` 中，且必须在主线程上构建。
		Ref<Mesh> mesh;
		Ref<Mesh> shadow_occluder_mesh;
		// 将 Mesh 表面索引重映射为 Mesher 材质索引。仅当 `has_mesh_resource` 为 true 时使用。
		// TODO 优化：适合做小向量优化。绝大多数网格只有少量表面，
		// 无需分配即可容纳在此。
		StdVector<uint16_t> mesh_material_indices;
		// 以网格数据块坐标表示
		Vector3i position;
		// TODO 重命名为 lod_index
		uint8_t lod;
		// 指示网格资源是否已作为任务的一部分构建完成。若未构建，则需在需要时于主线程上构建。
		bool has_mesh_resource;
		// 指示网格化任务是否被要求在可行时构建渲染网格。
		bool visual_was_required;
#ifdef VOXEL_ENABLE_SMOOTH_MESHING
		// 可以为空。附加到网格化输出上以便于跟踪，因为它从网格任务开始异步烘焙，
		// 完成时间可能早于或晚于网格。
		std::shared_ptr<DetailTextureOutput> detail_textures;
#endif
	};

	struct BlockDataOutput {
		enum Type { //
			TYPE_LOADED,
			TYPE_GENERATED,
			TYPE_SAVED
		};

		Type type;
		// 如果 TYPE_LOADED 时 voxels 为空，表示在流中（如果有）未找到任何数据块，且未调度生成器任务。
		// 这种情况出现在我们不想缓存生成数据的数据块时。
		std::shared_ptr<VoxelBuffer> voxels;
#ifdef VOXEL_ENABLE_INSTANCER
		UniquePtr<InstanceBlockData> instances;
#endif
		Vector3i position;
		uint8_t lod_index;
		bool dropped;
		bool max_lod_hint;
		// 设置了此标志的数据块不应被忽略。
		// 用于关闭数据流式加载、一次性加载所有数据块的情况。
		// TODO 未使用？
		bool initial_load;
		bool had_instances;
		bool had_voxels;
	};

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
	struct BlockDetailTextureOutput {
		std::shared_ptr<DetailTextureOutput> detail_textures;
		Vector3i position;
		uint32_t lod_index;
	};
#endif

	struct VolumeCallbacks {
		void (*mesh_output_callback)(void *, BlockMeshOutput &) = nullptr;
		void (*data_output_callback)(void *, BlockDataOutput &) = nullptr;
#ifdef VOXEL_ENABLE_SMOOTH_MESHING
		void (*detail_texture_output_callback)(void *, BlockDetailTextureOutput &) = nullptr;
#endif
		void *data = nullptr;

		inline bool check_callbacks() const {
			VOXEL_ASSERT_RETURN_V(mesh_output_callback != nullptr, false);
			VOXEL_ASSERT_RETURN_V(data_output_callback != nullptr, false);
			// VOXEL_ASSERT_RETURN_V(normalmap_output_callback != nullptr, false);
			VOXEL_ASSERT_RETURN_V(data != nullptr, false);
			return true;
		}
	};

	struct Viewer {
		struct Distances {
			unsigned int horizontal = 128;
			unsigned int vertical = 128;

			inline unsigned int max() const {
				return math::max(horizontal, vertical);
			}
		};
		// enum Flags {
		// 	FLAG_DATA = 1,
		// 	FLAG_VISUAL = 2,
		// 	FLAG_COLLISION = 4,
		// 	FLAGS_COUNT = 3
		// };
		Vector3 world_position;
		Distances view_distances;
		bool require_collisions = true;
		bool require_visuals = true;
		bool requires_data_block_notifications = false;
		int network_peer_id = -1;
	};

	static constexpr unsigned int DEFAULT_MAIN_THREAD_BUDGET_USEC = 8000;

	struct Config {
		int thread_count_minimum = 1;
		// 我们希望线程数比 CPU 可用线程数少多少（作为上限）
		int thread_count_margin_below_max = 1;
		// 尝试使用的可用 CPU 线程比例
		float thread_count_ratio_over_max = 0.5;
		unsigned int main_thread_budget_usec = DEFAULT_MAIN_THREAD_BUDGET_USEC;
	};

	static VoxelEngine &get_singleton();
	static void create_singleton(Config config);
	static void destroy_singleton();

	// 这是一个独立的初始化步骤。
	// 必须在 RenderingServer 单例可用时调用（类注册期间并不可用）。
	// 参见 https://github.com/godotengine/godot-cpp/issues/1180
	void try_initialize_gpu_features();

	VolumeID add_volume(VolumeCallbacks callbacks);
	VolumeCallbacks get_volume_callbacks(VolumeID volume_id) const;

	void remove_volume(VolumeID volume_id);
	bool is_volume_valid(VolumeID volume_id) const;

	std::shared_ptr<PriorityDependency::ViewersData> get_shared_viewers_data_from_default_world() const {
		return _world.shared_priority_dependency;
	}

	ViewerID add_viewer();
	void remove_viewer(ViewerID viewer_id);
	void set_viewer_position(ViewerID viewer_id, Vector3 position);
	void set_viewer_distances(ViewerID viewer_id, Viewer::Distances distances);
	Viewer::Distances get_viewer_distances(ViewerID viewer_id) const;
	void set_viewer_requires_visuals(ViewerID viewer_id, bool enabled);
	bool is_viewer_requiring_visuals(ViewerID viewer_id) const;
	void set_viewer_requires_collisions(ViewerID viewer_id, bool enabled);
	bool is_viewer_requiring_collisions(ViewerID viewer_id) const;
	void set_viewer_requires_data_block_notifications(ViewerID viewer_id, bool enabled);
	bool is_viewer_requiring_data_block_notifications(ViewerID viewer_id) const;
	void set_viewer_network_peer_id(ViewerID viewer_id, int peer_id);
	int get_viewer_network_peer_id(ViewerID viewer_id) const;
	bool viewer_exists(ViewerID viewer_id) const;
	void sync_viewers_task_priority_data();
	bool get_viewer_count() const;

	template <typename F>
	inline void for_each_viewer(F f) const {
		_world.viewers.for_each_key_value(f);
	}

	void push_main_thread_time_spread_task(
			ITimeSpreadTask *task,
			TimeSpreadTaskRunner::Priority priority = TimeSpreadTaskRunner::PRIORITY_NORMAL
	);
	int get_main_thread_time_budget_usec() const;
	void set_main_thread_time_budget_usec(unsigned int usec);

	// 从多个线程访问应当快速且安全。
	bool is_threaded_graphics_resource_building_enabled() const;
	// void set_threaded_graphics_resource_building_enabled(bool enabled);

	void push_main_thread_progressive_task(IProgressiveTask *task);

	// 线程安全。
	void push_async_task(IThreadedTask *task);
	// 线程安全。
	void push_async_tasks(Span<IThreadedTask *> tasks);
	// 线程安全。
	void push_async_io_task(IThreadedTask *task);
	// 线程安全。
	void push_async_io_tasks(Span<IThreadedTask *> tasks);

#ifdef VOXEL_ENABLE_GPU
	void push_gpu_task(IGPUTask *task);

	template <typename F>
	void push_gpu_task_f(F f) {
		struct Task : public IGPUTask {
			F f;
			Task(F pf) : f(pf) {}
			void prepare(GPUTaskContext &ctx) override {
				f(ctx);
			}
			void collect(GPUTaskContext &ctx) override {}
		};
		push_gpu_task(VOXEL_NEW(Task(f)));
	}

	uint32_t get_pending_gpu_tasks_count() const;
#endif

	void process();
	void wait_and_clear_all_tasks(bool warn);

	inline FileLocker &get_file_locker() {
		return _file_locker;
	}

	static inline int get_octree_lod_block_region_extent(float lod_distance, float block_size) {
		// 这是观察者周围可加载数据块的边界半径。
		// `lod_distance` 是数据块应细分为更小块的距离阈值。
		// 每个 LOD 都是分形的，因此该值对每一级都相同，乘以 2^lod。
		return static_cast<int>(Math::ceil(lod_distance / block_size)) * 2 + 2;
	}

	struct Stats {
		struct ThreadPoolStats {
			unsigned int thread_count;
			unsigned int active_threads;
			unsigned int tasks;
			FixedArray<const char *, ThreadedTaskRunner::MAX_THREADS> active_task_names;
		};

		ThreadPoolStats general;
		int generation_tasks;
		int streaming_tasks;
		int meshing_tasks;
		int main_thread_tasks;
#ifdef VOXEL_ENABLE_GPU
		int gpu_tasks;
#endif
	};

	Stats get_stats() const;

	int get_thread_count() const;
	void set_thread_count(uint32_t count);

	// RenderingDevice &get_rendering_device() const {
	// 	VOXEL_ASSERT(_rendering_device != nullptr);
	// 	return *_rendering_device;
	// }

	// TODO 本应设为私有，但那样 `memdelete<T>` 将无法调用它……
	~VoxelEngine();

	inline void debug_increment_generate_block_task_counter() {
		// 需要按条件执行此操作，以避免在非性能分析构建中出现“未使用变量”警告
#ifdef VOXEL_PROFILER_ENABLED
		int64_t v =
#endif
				++_debug_generate_block_task_count;
#ifdef VOXEL_PROFILER_ENABLED
		VOXEL_PROFILE_PLOT("GenerateBlock* tasks", v);
#endif
	}

	inline void debug_decrement_generate_block_task_counter() {
#ifdef VOXEL_PROFILER_ENABLED
		int64_t v =
#endif
				--_debug_generate_block_task_count;
#ifdef VOXEL_PROFILER_ENABLED
		VOXEL_PROFILE_PLOT("GenerateBlock* tasks", v);
#endif
	}

private:
	VoxelEngine(Config config);

	// 由于我们要向运行在多个线程中的任务发送数据，因此采用了以下几种策略：
	//
	// - 为每个任务复制数据。适用于调度后不会改变的简单信息。
	//
	// - 每线程实例。当某些堆分配的类实例在多线程中使用不安全，
	//   且在调度后不会改变时采用。
	//
	// - 共享指针。当数据在调度后可能改变时采用。
	//   通常在无锁情况下使用，但仅当允许脏读时才如此。
	//   如果这类数据集发生结构性变化（例如大小变化，或其他不可脏读的字段），
	//   则创建新实例，并让旧引用“自然消亡”。

	struct Volume {
		VolumeCallbacks callbacks;
	};

	struct World {
		SlotMap<Volume, uint16_t, uint16_t> volumes;
		SlotMap<Viewer, uint16_t, uint16_t> viewers;

		// 如果数量发生变化，必须用新实例覆盖。
		std::shared_ptr<PriorityDependency::ViewersData> shared_priority_dependency;
	};

	// TODO 未来支持多世界
	World _world;

	ThreadedTaskRunner _general_thread_pool;
	// 用于只能在主线程上运行并分散到多帧执行的任务
	TimeSpreadTaskRunner _time_spread_task_runner;
	unsigned int _main_thread_time_budget_usec = DEFAULT_MAIN_THREAD_BUDGET_USEC;
	ProgressiveTaskRunner _progressive_task_runner;

	FileLocker _file_locker;

	// 缓存是否允许在线程内构建 Mesh 和 Texture 资源。
	// 取决于 Godot 在此方面的效率以及所使用的渲染器。
	// 例如，OpenGL 渲染器对此支持不佳，而 Vulkan 渲染器应该没问题。
	bool _threaded_graphics_resource_building_enabled = false;

#ifdef VOXEL_ENABLE_GPU
	GPUTaskRunner _gpu_task_runner;
#endif

	// 生成任务的类型可能有多种，因此用公共计数器统计它们。
	std::atomic_int _debug_generate_block_task_count = { 0 };
};

struct VoxelFileLockerRead {
	VoxelFileLockerRead(const StdString &path) : _path(path) {
		VoxelEngine::get_singleton().get_file_locker().lock_read(path);
	}

	~VoxelFileLockerRead() {
		VoxelEngine::get_singleton().get_file_locker().unlock(_path);
	}

	StdString _path;
};

struct VoxelFileLockerWrite {
	VoxelFileLockerWrite(const StdString &path) : _path(path) {
		VoxelEngine::get_singleton().get_file_locker().lock_write(path);
	}

	~VoxelFileLockerWrite() {
		VoxelEngine::get_singleton().get_file_locker().unlock(_path);
	}

	StdString _path;
};

} // namespace voxel

#endif // VOXEL_ENGINE_H
