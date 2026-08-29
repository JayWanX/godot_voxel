#ifndef VOXEL_LOD_TERRAIN_UPDATE_DATA_H
#define VOXEL_LOD_TERRAIN_UPDATE_DATA_H

#include "../../constants/voxel_constants.h"
#include "../../generators/voxel_generator.h"
#include "../../streams/voxel_stream.h"
#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_map.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include "../../util/safe_ref_count.h"
#include "../../util/tasks/cancellation_token.h"
#include "../voxel_mesh_map.h"
#include "lod_octree.h"

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
#include "../../engine/detail_rendering/detail_rendering.h"
#endif


namespace voxel {

class AsyncDependencyTracker;

// VoxelLodTerrain 更新循环中多线程部分所需的设置和状态。
// 更多信息参见 `VoxelLodTerrainUpdateTask`。
struct VoxelLodTerrainUpdateData {
	struct TransitionUpdate {
		Vector3i block_position;
		uint8_t transition_mask;
	};

	struct BlockLocation {
		Vector3i position;
		uint8_t lod;

		inline bool operator==(BlockLocation other) const {
			return position == other.position && other.lod == lod;
		}
	};

	struct BlockToLoad {
		BlockLocation loc;
		TaskCancellationToken cancellation_token;
	};

	// struct BlockToSave {
	// 	std::shared_ptr<VoxelBuffer> voxels;
	// 	Vector3i position;
	// 	uint8_t lod;
	// };

	enum StreamingSystem : uint8_t { //
		STREAMING_SYSTEM_LEGACY_OCTREE = 0,
		STREAMING_SYSTEM_CLIPBOX
	};

	// 这些值在更新任务期间不会改变。
	struct Settings {
		// 体素可以存在的区域。
		// 注意，这些边界可能无法精确表示。该体积基于数据块，因此结果将
		// 近似到最近的数据块。
		// Box3i bounds_in_voxels;
		// unsigned int lod_count = 0;

		// 观察者与 LOD0 末端之间的距离。可能不会被精确遵守，会被向上取整
		float lod_distance = 0.f;
		// LOD0 末端与 LOD1 末端之间的距离，并延续到其它 LOD
		float secondary_lod_distance = 0.f;
		unsigned int view_distance_voxels = 512;
		StreamingSystem streaming_system = STREAMING_SYSTEM_LEGACY_OCTREE;
		// bool full_load_mode = false;
		// 若为 true，则尝试生成数据块并在发出网格请求前将它们存储到数据映射中。
		// 若为 false，则网格化将改为在运行时生成未编辑的体素。
		// 若流式加载被禁用，此选项不生效。
		bool cache_generated_blocks = false;
		bool collision_enabled = true;
		bool detail_textures_use_gpu = false;
		bool generator_use_gpu = false;
		uint8_t detail_texture_generator_override_begin_lod_index = 0;
		unsigned int mesh_block_size_po2 = 4;
#ifdef VOXEL_ENABLE_SMOOTH_MESHING
		DetailRenderingSettings detail_texture_settings;
#endif
		Ref<VoxelGenerator> detail_texture_generator_override;
	};

	enum MeshState {
		MESH_NEVER_UPDATED = 0, // TODO 与 MESH_NEED_UPDATE 冗余？
		MESH_UP_TO_DATE,
		MESH_NEED_UPDATE, // 网格已过期，但尚未安排更新
		MESH_UPDATE_NOT_SENT, // 网格已过期并已安排更新，但请求尚未
							  // 发出
		MESH_UPDATE_SENT // 网格已过期，且已发出更新请求，等待响应
	};

	enum DetailTextureState { //
		DETAIL_TEXTURE_IDLE = 0,
		DETAIL_TEXTURE_NEED_UPDATE,
		DETAIL_TEXTURE_PENDING
	};

	struct MeshBlockState {
		std::atomic<MeshState> state;
		std::atomic<DetailTextureState> detail_texture_state;

		// 此处使用引用计数以支持多个观察者，由于流式加载逻辑位于更新任务中，
		// 无法在主线程的网格映射上执行此操作。
		// TODO 优化：这里几乎可以不需要原子操作。
		// 之所以使用原子引用计数，只是因为主线程在接收网格更新时需要读取它。若
		// 网格更新由线程化更新处理，则可以改为非原子，但这样做的
		// 影响比暂时使用原子计数更多。
		SafeRefCount mesh_viewers;
		SafeRefCount collision_viewers;

		// 当此网格数据块被移除时取消，因此若有任务仍在排队为该数据块工作，
		// 它们将被取消
		TaskCancellationToken cancellation_token;

		// 在单次地形更新期间要更新的网格列表中的索引。用于避免将同一个
		// 网格多次放入列表，同时允许在加入列表后更改选项。每次更新后应
		// 重置为 -1（因为列表会被消耗）
		int update_list_index;

		uint8_t transition_mask;
		bool visual_active;
		bool collision_active;

		// 表示自该数据块添加以来是否已完成首次网格化。
		// 仅由主线程写入，在主线程接收网格更新或卸载资源时。
		// 由线程化更新读取，以决定何时细分 LOD。
		std::atomic_bool visual_loaded;
		std::atomic_bool collision_loaded;

		// bool pending_update_has_visuals;
		// bool pending_update_has_collision;

		MeshBlockState() :
				state(MESH_NEVER_UPDATED),
				detail_texture_state(DETAIL_TEXTURE_IDLE),
				update_list_index(-1),
				transition_mask(0),
				visual_active(false),
				collision_active(false),
				visual_loaded(false),
				collision_loaded(false) {}
	};

	// 网格映射的版本，设计主要用于线程化更新任务。
	// 它包含用于决定何时实际加载/卸载网格的状态。
	struct MeshMapState {
		// 该映射中的值应具有稳定的地址。
		StdUnorderedMap<Vector3i, MeshBlockState> map;
		// 当数据块被插入或从映射中移除时，写入需加锁。
		// 若需同时锁定多个 LOD，务必按递增顺序锁定，以避免死锁。
		// 重要提示：
		// - 只有更新任务会向此映射添加和移除数据块。
		// - 更新任务之外的线程绝不能向映射添加或移除数据块（即使加锁也不行），
		//   除非该任务不是并行运行的。
		// - 更新任务之外的线程必须始终对其加锁，除非更新任务未在运行。
		// - 更新任务无需对其加锁，除非在添加或移除数据块时。
		RWLock map_lock;
	};

	struct LoadingDataBlock {
		RefCount viewers;
		TaskCancellationToken cancellation_token;
	};

	struct MeshToUpdate {
		Vector3i position;
		TaskCancellationToken cancellation_token;
		bool require_visual = false;
	};

	struct QuickReloadingBlock {
		std::shared_ptr<VoxelBuffer> voxels;
		Vector3i position;
	};

	// 每个 LOD 工作在一组坐标中，其索引越高，覆盖的体素范围为其 2 倍
	struct Lod {
		// 跟踪正在异步加载的数据块，以免重复加载
		StdUnorderedMap<Vector3i, LoadingDataBlock> loading_blocks;
		BinaryMutex loading_blocks_mutex;

		// 卸载后等待保存的数据块。这是为了在观察者在它们保存完成前
		// 再次需要它们时能正确重新加载。此缓存中的条目在保存后被移除。需要
		// 用互斥锁保护，因为保存通知目前是在主线程上接收的。
		StdUnorderedMap<Vector3i, std::shared_ptr<VoxelBuffer>> unloaded_saving_blocks;
		BinaryMutex unloaded_saving_blocks_mutex;
		// 下次地形更新时，将像加载任务完成一样从保存缓存中加载的数据块。
		// 它在线程化更新运行期间不会运行，因此无需加锁。
		StdVector<QuickReloadingBlock> quick_reloading_blocks;

		// 这些相对于此 LOD，以数据块坐标表示
		Vector3i last_viewer_data_block_pos;
		int last_view_distance_data_blocks = 0;

		MeshMapState mesh_map_state;

		// 下次更新任务运行时将被安排更新的网格数据块位置。
		StdVector<MeshToUpdate> mesh_blocks_pending_update;
		Vector3i last_viewer_mesh_block_pos;
		int last_view_distance_mesh_blocks = 0;

		// 延迟输出到主线程。任务结束后才能读取，因此无需加锁。
		// TODO 这些输出操作并未特别序列化，可能会引起问题（目前尚未发生）。
		StdVector<Vector3i> mesh_blocks_to_unload;
		StdVector<TransitionUpdate> mesh_blocks_to_update_transitions;
		StdVector<Vector3i> mesh_blocks_to_activate_visuals;
		StdVector<Vector3i> mesh_blocks_to_deactivate_visuals;
		StdVector<Vector3i> mesh_blocks_to_activate_collision;
		StdVector<Vector3i> mesh_blocks_to_deactivate_collision;
		StdVector<Vector3i> mesh_blocks_to_drop_visual;
		StdVector<Vector3i> mesh_blocks_to_drop_collision;

		inline bool has_loading_block(const Vector3i &pos) const {
			return loading_blocks.find(pos) != loading_blocks.end();
		}
	};

	struct AsyncEdit {
		IThreadedTask *task;
		Box3i box;
		std::shared_ptr<AsyncDependencyTracker> task_tracker;
	};

	struct RunningAsyncEdit {
		std::shared_ptr<AsyncDependencyTracker> tracker;
		Box3i box;
	};

	struct Stats {
		uint32_t blocked_lods = 0;
		uint32_t time_detect_required_blocks = 0;
		uint32_t time_io_requests = 0;
		uint32_t time_mesh_requests = 0;
		uint32_t time_total = 0;
	};

	struct OctreeItem {
		LodOctree octree;
	};

	struct OctreeStreamingState {
		// 这种地形类型是一个稀疏的八叉树网格。
		// 通过网格坐标索引，其步长是最高 LOD 数据块的大小。
		// 不使用指针，因为 Map 存储是稳定的。
		// TODO 优化：可以用网格数据结构替换
		StdMap<Vector3i, OctreeItem> lod_octrees;
		Box3i last_octree_region_box;
		Vector3i local_viewer_pos_previous_octree_update;

		// 表示是否存在需要分裂或合并但由于挂起的依赖而无法执行的节点。
		// 这影响是否需要在下次更新时再次处理八叉树流式加载。
		bool had_blocked_octree_nodes_previous_update = false;

		bool force_update_octrees_next_update = false;
	};

	// 配对观察者是指与体积边界相交的 VoxelViewer
	struct PairedViewer {
		struct Distances {
			unsigned int horizontal = 0;
			unsigned int vertical = 0;
		};
		struct State {
			Vector3i local_position_voxels;

			// 以数据块坐标表示
			FixedArray<Box3i, constants::MAX_LOD> data_box_per_lod;
			FixedArray<Box3i, constants::MAX_LOD> mesh_box_per_lod;

			Distances view_distance_voxels;
			bool requires_collisions = false;
			bool requires_visuals = false;
		};
		ViewerID id;
		State state;
		State prev_state;
	};

	struct LoadedMeshBlockEvent {
		Vector3i position;
		uint8_t lod_index;
		bool visual;
		bool collision;
	};

	struct ClipboxStreamingState {
		StdVector<PairedViewer> paired_viewers;
		// Vector3i viewer_pos_in_lod0_voxels_previous_update;
		// int lod_distance_in_data_chunks_previous_update = 0;
		// int lod_distance_in_mesh_chunks_previous_update = 0;

		// 主线程在收到数据块时写入。
		// 更新线程读取以触发网格化。
		StdVector<BlockLocation> loaded_data_blocks;
		BinaryMutex loaded_data_blocks_mutex;

		// 主线程在收到网格数据块时写入（且之前没有网格）。
		// 更新线程读取以触发可见性变化。
		StdVector<LoadedMeshBlockEvent> loaded_mesh_blocks;
		BinaryMutex loaded_mesh_blocks_mutex;
	};

	struct EditNotificationInputs {
		// 通知数据变化的入口，将导致数据 LOD 和网格更新。
		// 包含被编辑且需要更新其对应 LOD 版本的数据块。
		// 仅在 LOD0 上安排调度，因为它是唯一可编辑的 LOD。

		// 专门用于生成体素的低 LOD 版本
		StdVector<Vector3i> edited_blocks_lod0;
		// 专门用于更新网格
		// TODO 也许我们可以只使用这一个？之所以单独维护已编辑数据块，是因为编辑可能只影响
		// 特定数据块，而不影响整个区域
		StdVector<Box3i> edited_voxel_areas_lod0;

		BinaryMutex mutex;
	};

	// 由更新任务修改的数据
	struct State {
		OctreeStreamingState octree_streaming;
		ClipboxStreamingState clipbox_streaming;

		FixedArray<Lod, constants::MAX_LOD> lods;

		EditNotificationInputs edit_notifications;

		StdVector<AsyncEdit> pending_async_edits;
		BinaryMutex pending_async_edits_mutex;
		StdVector<RunningAsyncEdit> running_async_edits;

		// 生成内容发生变化的区域。类似于编辑，但非破坏性。
		StdVector<Box3i> changed_generated_areas;
		BinaryMutex changed_generated_areas_mutex;

		Stats stats;
	};

	// 更新任务完成时设为 true
	std::atomic_bool task_is_complete = { true };
	// 更新任务运行期间将保持锁定。
	BinaryMutex completion_mutex;

	Settings settings;
	State state;

	// 所有观察者的副本，因为当前在 VoxelEngine 中直接访问它们并非线程安全
	StdVector<std::pair<ViewerID, VoxelEngine::Viewer>> viewers;

	// 此调用之后无需加锁，因为没有其它线程应再使用该数据。
	// 但它可能阻塞更长时间，因此在做结构性更改（如更改 LOD 数量、
	// LOD 距离或更新逻辑的运行方式）时优先使用。
	void wait_for_end_of_task() {
		MutexLock lock(completion_mutex);
	}
};

} // namespace voxel

#endif // VOXEL_LOD_TERRAIN_UPDATE_DATA_H
