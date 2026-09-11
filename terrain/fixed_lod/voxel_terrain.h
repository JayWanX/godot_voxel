#ifndef VOXEL_TERRAIN_H
#define VOXEL_TERRAIN_H

#include "../../constants/voxel_constants.h"
#include "../../engine/meshing_dependency.h"
#include "../../storage/voxel_data.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include <core/object/gdvirtual.gen.h>
#include "../../util/godot/memory.h"
#include "../../util/math/box3i.h"
#include "../voxel_data_block_enter_info.h"
#include "../voxel_mesh_map.h"
#include "../voxel_node.h"
#include "voxel_mesh_block_vt.h"
#include "voxel_terrain_multiplayer_synchronizer.h"

#ifdef TOOLS_ENABLED
#include "../../util/godot/debug_renderer.h"
#endif

namespace voxel {

class AsyncDependencyTracker;

class VoxelTool;
class VoxelSaveCompletionTracker;
class VoxelTerrainMultiplayerSynchronizer;
class BufferedTaskScheduler;

#ifdef VOXEL_ENABLE_INSTANCER
class VoxelInstancer;
#endif

// 由体素数据块组成、具有相同细节层级（LOD）的无限分页地形。
// 体素在大型立方体空间中按与观察者的距离进行多边形化。
// 数据使用 VoxelStream 进行流式传输。
class VoxelTerrain : public VoxelNode {
	GDCLASS(VoxelTerrain, VoxelNode)
public:
	// 当任何地形边界大于它时的最大观察距离
	static const unsigned int MAX_VIEW_DISTANCE_FOR_LARGE_VOLUME = 512;

	VoxelTerrain();
	~VoxelTerrain();

	// 流：负责数据的加载与保存
	void set_stream(Ref<VoxelStream> p_stream) override;
	Ref<VoxelStream> get_stream() const override;

	// 用于生成地形数据的生成器
	void set_generator(Ref<VoxelGenerator> p_generator) override;
	Ref<VoxelGenerator> get_generator() const override;

	// 负责网格化的网格器
	void set_mesher(Ref<VoxelMesher> mesher) override;
	Ref<VoxelMesher> get_mesher() const override;

	// 数据块大小，以 2 的幂表示，以及实际大小
	unsigned int get_data_block_size_pow2() const;
	inline unsigned int get_data_block_size() const {
		return 1 << get_data_block_size_pow2();
	}
	// void set_data_block_size_po2(unsigned int p_block_size_po2);

	// 网格块大小，以 2 的幂表示，以及实际大小
	unsigned int get_mesh_block_size_pow2() const;
	inline unsigned int get_mesh_block_size() const {
		return 1 << get_mesh_block_size_pow2();
	}
	void set_mesh_block_size(unsigned int p_block_size);

	// 编辑后通知地形更新相应区域
	void post_edit_voxel(Vector3i pos);
	void post_edit_area(Box3i box_in_voxels, bool update_mesh);

	// 是否生成碰撞体
	void set_generate_collisions(bool enabled);
	bool get_generate_collisions() const {
		return _generate_collisions;
	}

	// 碰撞层
	void set_collision_layer(int layer);
	int get_collision_layer() const;

	// 碰撞掩码
	void set_collision_mask(int mask);
	int get_collision_mask() const;

	// 碰撞边距
	void set_collision_margin(float margin);
	float get_collision_margin() const;

	// 最大观察距离，以体素为单位
	int get_max_view_distance() const;
	void set_max_view_distance(int distance_in_voxels);

	// 是否启用数据块进入通知
	void set_block_enter_notification_enabled(bool enable);
	bool is_block_enter_notification_enabled() const;

	// 是否启用区域编辑通知
	void set_area_edit_notification_enabled(bool enable);
	bool is_area_edit_notification_enabled() const;

	// 是否自动加载观察者周围的块
	void set_automatic_loading_enabled(bool enable);
	bool is_automatic_loading_enabled() const;

	// 覆盖默认的网格材质
	void set_material_override(Ref<Material> material);
	Ref<Material> get_material_override() const;

#ifdef VOXEL_ENABLE_GPU
	// 生成器是否使用 GPU
	void set_generator_use_gpu(bool enabled);
	bool get_generator_use_gpu() const;
#endif

	// 获取地形数据存储
	VoxelData &get_storage() const override;

	std::shared_ptr<VoxelData> get_storage_shared() const {
		return _data;
	}

	// 获取用于编辑地形的工具
	Ref<VoxelTool> get_voxel_tool() override;

	// 在给定位置创建或覆盖任何已有的数据块数据。
	// 使用场景是多人在线、客户端侧。
	// 如果本地没有观察者在范围内，数据将不会被应用，该函数返回 `false`。
	bool try_set_block_data(Vector3i position, std::shared_ptr<VoxelBuffer> &voxel_data);

	// 是否存在指定位置的数据块
	bool has_data_block(Vector3i position) const;

	// 地形数据范围的边界框
	void set_bounds(Box3i box);
	Box3i get_bounds() const;

	// 重启流，重新网格化所有块
	void restart_stream() override;
	void remesh_all_blocks() override;

	// 请求在给定位置异步生成（或重新生成）一个数据块。
	// 如果在数据块生成完成时它已经存在，该请求将被取消。
	// 如果数据块超出任何观察者的范围，它将被取消。
	void generate_block_async(Vector3i block_position);

	struct Stats {
		int updated_blocks = 0;
		int dropped_block_loads = 0;
		int dropped_block_meshs = 0;
		uint32_t time_detect_required_blocks = 0;
		uint32_t time_request_blocks_to_load = 0;
		uint32_t time_process_load_responses = 0;
		uint32_t time_request_blocks_to_update = 0;
	};

	// 获取更新统计信息
	const Stats &get_stats() const;

	// struct BlockToSave {
	// 	std::shared_ptr<VoxelBuffer> voxels;
	// 	Vector3i position;
	// };

	Node3D *convert_to_nodes(const BitField<NodeConversionFlags> flags) const override;

	// 调试

	enum DebugDrawFlag {
		DEBUG_DRAW_VOLUME_BOUNDS = 0,
		DEBUG_DRAW_VISUAL_AND_COLLISION_BLOCKS = 1,
		DEBUG_DRAW_VOXEL_METADATA = 2,

		DEBUG_DRAW_FLAGS_COUNT = 3
	};

	// 是否启用调试绘制
	void debug_set_draw_enabled(bool enabled);
	bool debug_is_draw_enabled() const;

	// 设置 / 查询调试绘制标志
	void debug_set_draw_flag(DebugDrawFlag flag_index, bool enabled);
	bool debug_get_draw_flag(DebugDrawFlag flag_index) const;

	// 是否绘制阴影遮挡物
	void debug_set_draw_shadow_occluders(bool enable);
	bool debug_get_draw_shadow_occluders() const;

	// 内部

#ifdef VOXEL_ENABLE_INSTANCER
	// 设置实例化器
	void set_instancer(VoxelInstancer *instancer);
#endif
	// 获取已网格化的块位置及其网格表面数据
	void get_meshed_block_positions(StdVector<Vector3i> &out_positions) const;
	Array get_mesh_block_surface(Vector3i block_pos) const;

	VolumeID get_volume_id() const override {
		return _volume_id;
	}

	std::shared_ptr<StreamingDependency> get_streaming_dependency() const override {
		return _streaming_dependency;
	}

	// 获取指定区域内的观察者 ID 列表
	void get_viewers_in_area(StdVector<ViewerID> &out_viewer_ids, Box3i voxel_box) const;

	// 多人在线同步器
	void set_multiplayer_synchronizer(VoxelTerrainMultiplayerSynchronizer *synchronizer);
	const VoxelTerrainMultiplayerSynchronizer *get_multiplayer_synchronizer() const;

#ifdef TOOLS_ENABLED
	void get_configuration_warnings(PackedStringArray &warnings) const override;
#endif // TOOLS_ENABLED

	void on_format_changed() override;

protected:
	void _notification(int p_what);

	void _on_gi_mode_changed() override;
	void _on_shadow_casting_changed() override;
	void _on_render_layers_mask_changed() override;

private:
	void process();
	void process_viewers();
	void process_viewer_data_box_change(
			const ViewerID viewer_id,
			const Box3i prev_data_box,
			const Box3i new_data_box,
			const bool can_load_blocks
	);
	// void process_received_data_blocks();
	void process_meshing();
	void apply_mesh_update(const VoxelEngine::BlockMeshOutput &ob);
	void apply_data_block_response(VoxelEngine::BlockDataOutput &ob);

	void _on_stream_params_changed();
	// void _set_block_size_po2(int p_block_size_po2);
	// void make_all_view_dirty();
	void start_updater();
	void stop_updater();
	void start_streamer();
	void stop_streamer();
	void reset_map();
	void clear_mesh_map();

	// void view_data_block(Vector3i bpos, uint32_t viewer_id, bool require_notification);
	void view_mesh_block(Vector3i bpos, bool mesh_flag, bool collision_flag);
	// void unview_data_block(Vector3i bpos);
	void unview_mesh_block(Vector3i bpos, bool mesh_flag, bool collision_flag);
	// void unload_data_block(Vector3i bpos);
	void unload_mesh_block(Vector3i bpos);
	// void make_data_block_dirty(Vector3i bpos);
	void try_schedule_mesh_update(VoxelMeshBlockVT &block);
	void try_schedule_mesh_update_from_data(const Box3i &box_in_voxels);

	void save_all_modified_blocks(bool with_copy, std::shared_ptr<AsyncDependencyTracker> tracker);
	void get_viewer_pos_and_direction(Vector3 &out_pos, Vector3 &out_direction) const;
	void send_data_load_requests();
	void consume_block_data_save_requests(
			BufferedTaskScheduler &task_scheduler,
			std::shared_ptr<AsyncDependencyTracker> saving_tracker,
			bool with_flush
	);

	void emit_data_block_loaded(Vector3i bpos);
	void emit_data_block_unloaded(Vector3i bpos);

	void emit_mesh_block_entered(Vector3i bpos);
	void emit_mesh_block_exited(Vector3i bpos);

	bool try_get_paired_viewer_index(ViewerID id, size_t &out_i) const;

	void notify_data_block_enter(const VoxelDataBlock &block, Vector3i bpos, ViewerID viewer_id);

	bool is_area_meshed(const Box3i &box_in_voxels) const;

#ifdef TOOLS_ENABLED
	void process_debug_draw();
#endif

	// 每当一个数据块进入观察者的区域时被调用。
	// 这可以发生在数据块已存在且观察者足够接近时，也可以发生在数据块被加载时。
	// 仅当启用了数据块进入通知时才发生。
	GDVIRTUAL1(_on_data_block_entered, VoxelDataBlockEnterInfo *);

	// 每当某个区域内的体素被编辑时被调用。
	GDVIRTUAL2(_on_area_edited, Vector3i, Vector3i);

	static void _bind_methods();

	// 绑定
	Vector3i _b_voxel_to_data_block(Vector3 pos) const;
	Vector3i _b_data_block_to_voxel(Vector3i pos) const;
	// void _force_load_blocks_binding(Vector3 center, Vector3 extents) { force_load_blocks(center, extents); }
	Ref<VoxelSaveCompletionTracker> _b_save_modified_blocks();
	void _b_save_block(Vector3i p_block_pos);
	void _b_set_bounds(AABB aabb);
	AABB _b_get_bounds() const;
	bool _b_try_set_block_data(Vector3i position, Ref<godot::VoxelBuffer> voxel_data);
	Dictionary _b_get_statistics() const;
	PackedInt32Array _b_get_viewer_network_peer_ids_in_area(Vector3i area_origin, Vector3i area_size) const;
	void _b_rpc_receive_block(PackedByteArray data);
	void _b_rpc_receive_area(PackedByteArray data);
	bool _b_is_area_meshed(AABB aabb) const;

	VolumeID _volume_id;

	// 配对的观察者是那些与体积边界相交的 VoxelViewer
	struct PairedViewer {
		struct State {
			Vector3i local_position_voxels;
			Box3i data_box; // 以数据块坐标表示
			Box3i mesh_box;
			int horizontal_view_distance_voxels = 0;
			int vertical_view_distance_voxels = 0;
			bool requires_collisions = false;
			bool requires_meshes = false;
		};
		ViewerID id;
		State state;
		State prev_state;
	};

	StdVector<PairedViewer> _paired_viewers;

	// 体素存储。使用 shared_ptr 以便线程任务可以安全地使用它。
	std::shared_ptr<VoxelData> _data;

	// 网格存储
	VoxelMeshMap<VoxelMeshBlockVT> _mesh_map;
	uint32_t _mesh_block_size_po2 = constants::DEFAULT_BLOCK_SIZE_PO2;

	unsigned int _max_view_distance_voxels = 128;

	// TODO 地形只需要处理体素的可见部分，这可以减少需要处理边界的块的数量。
	// 因此，简单的网格（grid）是否比哈希表更好用？

	struct LoadingBlock {
		RefCount viewers;
		// TODO 优化此处的分配
		StdVector<ViewerID> viewers_to_notify;
	};

	// 当前正在加载的数据块。
	StdUnorderedMap<Vector3i, LoadingBlock> _loading_blocks;
	// 应该在下次 process 调用时加载的数据块。
	// 该列表中的顺序无关紧要。
	StdVector<Vector3i> _blocks_pending_load;
	// 应该在下次 process 调用时更新的数据块网格。
	// 该列表中的顺序无关紧要。
	StdVector<Vector3i> _blocks_pending_update;
	// 应该在下次 process 调用时保存的数据块。
	// 该列表中的顺序无关紧要。
	StdVector<VoxelData::BlockToSave> _blocks_to_save;
	// 已卸载且需要保存的数据块。在保存完成前会临时存储在这里，
	// 并且在加载新数据块之前会先检查它们。这是为了应对玩家离开一个区域后
	// 在保存完成前又返回的情况，否则从流加载会返回过期的版本。
	StdUnorderedMap<Vector3i, std::shared_ptr<VoxelBuffer>> _unloaded_saving_blocks;
	// 将在下次 process 调用时用于模拟加载响应的数据块列表。
	struct QuickReloadingBlock {
		std::shared_ptr<VoxelBuffer> voxels;
		Vector3i position;
	};
	StdVector<QuickReloadingBlock> _quick_reloading_blocks;

	Ref<VoxelMesher> _mesher;

	// 使用共享指针存储数据，以便可以发送给异步任务，并且这些任务可以通过将 bool 设为 false
	// 并重新实例化该结构来取消
	std::shared_ptr<StreamingDependency> _streaming_dependency;
	std::shared_ptr<MeshingDependency> _meshing_dependency;

	bool _generate_collisions = true;
	unsigned int _collision_layer = 1;
	unsigned int _collision_mask = 1;
	float _collision_margin = constants::DEFAULT_COLLISION_MARGIN;
	// bool _stream_enabled = false;
	bool _block_enter_notification_enabled = false;
	bool _area_edit_notification_enabled = false;
	// 如果启用，VoxelViewer 将使数据块在其周围自动加载。
	bool _automatic_loading_enabled = true;
	bool _generator_use_gpu = false;

	Ref<Material> _material_override;

	voxel::godot::ObjectUniquePtr<VoxelDataBlockEnterInfo> _data_block_enter_info_obj;

	// 对外部节点的引用。
#ifdef VOXEL_ENABLE_INSTANCER
	VoxelInstancer *_instancer = nullptr;
#endif
	VoxelTerrainMultiplayerSynchronizer *_multiplayer_synchronizer = nullptr;

	Stats _stats;

#ifdef TOOLS_ENABLED
	bool _debug_draw_enabled = false;
	uint8_t _debug_draw_flags = 0;

	voxel::godot::DebugRenderer _debug_renderer;

	bool _debug_draw_shadow_occluders = false;
#endif
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelTerrain::DebugDrawFlag)

#endif // VOXEL_TERRAIN_H
