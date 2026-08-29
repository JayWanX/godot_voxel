#ifndef VOXEL_LOD_TERRAIN_HPP
#define VOXEL_LOD_TERRAIN_HPP

#include "../../engine/voxel_engine.h"
#include "../../meshers/mesh_block_task.h"
#include "../../storage/voxel_data.h"
#include "../../util/containers/std_map.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include "../voxel_mesh_map.h"
#include "../voxel_node.h"
#include "lod_octree.h"
#include "shader_material_pool_vlt.h"
#include "voxel_lod_terrain_update_data.h"
#include "voxel_mesh_block_vlt.h"

#ifdef TOOLS_ENABLED
#include "../../util/godot/debug_renderer.h"
#endif

namespace voxel {

class VoxelTool;
class VoxelStream;
class VoxelSaveCompletionTracker;

#ifdef VOXEL_ENABLE_INSTANCER
class VoxelInstancer;
#endif

// 由可变细节层级体素数据块构成的分页地形。
// 面向最高视距设计，最好使用平滑体素。
// 体素以距离为依据在观察者周围一个非常大的球体内被多边形化，通常延伸到远裁剪面之外。
// VoxelStream 和 VoxelGenerator 必须支持 LOD。
class VoxelLodTerrain : public VoxelNode {
	GDCLASS(VoxelLodTerrain, VoxelNode)
public:
	VoxelLodTerrain();
	~VoxelLodTerrain();

	Ref<Material> get_material() const;
	void set_material(Ref<Material> p_material);

	Ref<VoxelStream> get_stream() const override;
	void set_stream(Ref<VoxelStream> p_stream) override;

	Ref<VoxelGenerator> get_generator() const override;
	void set_generator(Ref<VoxelGenerator> p_stream) override;

	Ref<VoxelMesher> get_mesher() const override;
	void set_mesher(Ref<VoxelMesher> p_mesher) override;

	int get_view_distance() const;
	void set_view_distance(int p_distance_in_voxels);

	void set_lod_distance(float p_lod_distance);
	float get_lod_distance() const;

	void set_secondary_lod_distance(float p_lod_distance);
	float get_secondary_lod_distance() const;

	void set_lod_count(int p_lod_count);
	int get_lod_count() const;

	void set_generate_collisions(bool enabled);
	bool get_generate_collisions() const;

	// 设置碰撞将生成到哪一层 LOD。-1 表示全部生成。
	void set_collision_lod_count(int lod_count);
	int get_collision_lod_count() const;

	void set_collision_layer(int layer);
	int get_collision_layer() const;

	void set_collision_mask(int mask);
	int get_collision_mask() const;

	void set_collision_margin(float margin);
	float get_collision_margin() const;

	int get_data_block_region_extent() const;
	int get_mesh_block_region_extent() const;

	Vector3i voxel_to_data_block_position(Vector3 vpos, int lod_index) const;
	Vector3i voxel_to_mesh_block_position(Vector3 vpos, int lod_index) const;

	unsigned int get_data_block_size_pow2() const;
	unsigned int get_data_block_size() const;
	// void set_data_block_size_po2(unsigned int p_block_size_po2);

	unsigned int get_mesh_block_size_pow2() const;
	unsigned int get_mesh_block_size() const;
	void set_mesh_block_size(unsigned int mesh_block_size);

	void set_full_load_mode_enabled(bool enabled);
	bool is_full_load_mode_enabled() const;

	void set_threaded_update_enabled(bool enabled);
	bool is_threaded_update_enabled() const;

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
	void set_normalmap_enabled(bool enable);
	bool is_normalmap_enabled() const;

	void set_octahedral_normal_encoding(bool enable);
	bool get_octahedral_normal_encoding() const;

	void set_normalmap_tile_resolution_min(int resolution);
	int get_normalmap_tile_resolution_min() const;

	void set_normalmap_tile_resolution_max(int resolution);
	int get_normalmap_tile_resolution_max() const;

	void set_normalmap_begin_lod_index(int lod_index);
	int get_normalmap_begin_lod_index() const;

	void set_normalmap_max_deviation_degrees(int angle);
	int get_normalmap_max_deviation_degrees() const;

	void set_normalmap_generator_override(Ref<VoxelGenerator> generator_override);
	Ref<VoxelGenerator> get_normalmap_generator_override() const;

	void set_normalmap_generator_override_begin_lod_index(int lod_index);
	int get_normalmap_generator_override_begin_lod_index() const;

#ifdef VOXEL_ENABLE_GPU
	void set_normalmap_use_gpu(bool enabled);
	bool get_normalmap_use_gpu() const;
#endif
#endif

#ifdef VOXEL_ENABLE_GPU
	void set_generator_use_gpu(const bool enabled);
	bool get_generator_use_gpu() const;
#endif

	void set_cache_generated_blocks(bool enabled);
	bool get_cache_generated_blocks() const;

	// 这些必须在编辑后调用
	void post_edit_area(Box3i p_box, bool update_mesh);
	void post_edit_modifiers(Box3i p_voxel_box);

	// TODO 目前这仍然很糟糕，因为编辑仍将在主线程上运行
	void push_async_edit(IThreadedTask *task, Box3i box, std::shared_ptr<AsyncDependencyTracker> tracker);
	void abort_async_edits();

	void set_voxel_bounds(Box3i p_box);

	inline Box3i get_voxel_bounds() const {
		VOXEL_ASSERT(_data != nullptr);
		return _data->get_bounds();
	}

	void set_collision_update_delay(int delay_msec);
	int get_collision_update_delay() const;

	void set_lod_fade_duration(float seconds);
	float get_lod_fade_duration() const;

	enum ProcessCallback { //
		PROCESS_CALLBACK_IDLE = 0,
		PROCESS_CALLBACK_PHYSICS,
		PROCESS_CALLBACK_DISABLED
	};

	// 这最初是为了修复刚体传送与漂浮世界原点的问题而添加的：
	// 由于变换更新的延迟，玩家以不同于世界其余部分的速度传送，
	// 导致世界在 3 帧时间内完全卸载然后重新加载，
	// 产生闪烁和 CPU 卡顿。更改处理模式可以对齐更新速率，
	// 并在传送期间冻结 LOD。
	void set_process_callback(ProcessCallback mode);
	ProcessCallback get_process_callback() const {
		return _process_callback;
	}

	Ref<VoxelTool> get_voxel_tool() override;

	struct Stats {
		// 等待数据的八叉树节点数量。当一切加载完成时应归零。
		uint32_t blocked_lods = 0;
		// 本帧被拒绝的数据块数量（例如因加载过晚）。
		uint32_t dropped_block_loads = 0;
		// 本帧被拒绝的网格数据块数量（例如因加载过晚）。
		uint32_t dropped_block_meshs = 0;
		// 上次更新中卸载未使用数据块及检测所需数据块所花时间，单位微秒
		uint32_t time_detect_required_blocks = 0;
		// 上次更新中请求数据块所花时间，单位微秒
		uint32_t time_io_requests = 0;
		// 上次更新中请求网格所花时间，单位微秒
		uint32_t time_mesh_requests = 0;
		// 上次更新任务所花总时间，单位微秒。
		// 仅包含可线程化的部分，而非整个 `process` 函数。
		uint32_t time_update_task = 0;
	};

	const Stats &get_stats() const;

	void restart_stream() override;
	void remesh_all_blocks() override;

	bool is_area_meshed(const Box3i &box_in_voxels, unsigned int lod_index) const;

	enum StreamingSystem : uint8_t { //
		STREAMING_SYSTEM_LEGACY_OCTREE = VoxelLodTerrainUpdateData::STREAMING_SYSTEM_LEGACY_OCTREE,
		STREAMING_SYSTEM_CLIPBOX = VoxelLodTerrainUpdateData::STREAMING_SYSTEM_CLIPBOX
	};

	// 这是临时的，以便在新系统改进过程中不破坏现有项目，并允许逐步过渡。
	StreamingSystem get_streaming_system() const;
	void set_streaming_system(StreamingSystem v);

	Node3D *convert_to_nodes(const BitField<NodeConversionFlags> flags) const override;

	// 调试

	Array debug_raycast_mesh_block(Vector3 world_origin, Vector3 world_direction) const;
	Dictionary debug_get_data_block_info(Vector3 fbpos, int lod_index) const;
	Dictionary debug_get_mesh_block_info(Vector3 fbpos, int lod_index) const;
	Array debug_get_octree_positions() const;
	Array debug_get_octrees_detailed() const;

	enum DebugDrawFlag {
		DEBUG_DRAW_OCTREE_NODES = 0,
		DEBUG_DRAW_OCTREE_BOUNDS = 1,
		DEBUG_DRAW_MESH_UPDATES = 2,
		DEBUG_DRAW_EDIT_BOXES = 3,
		DEBUG_DRAW_VOLUME_BOUNDS = 4,
		DEBUG_DRAW_EDITED_BLOCKS = 5,
		DEBUG_DRAW_MODIFIER_BOUNDS = 6,
		DEBUG_DRAW_ACTIVE_MESH_BLOCKS = 7,
		DEBUG_DRAW_VIEWER_CLIPBOXES = 8,
		DEBUG_DRAW_LOADED_VISUAL_AND_COLLISION_BLOCKS = 9,
		DEBUG_DRAW_ACTIVE_VISUAL_AND_COLLISION_BLOCKS = 10,
		DEBUG_DRAW_VOXEL_METADATA = 11,

		DEBUG_DRAW_FLAGS_COUNT = 12
	};

	void debug_set_draw_enabled(bool enabled);
	bool debug_is_draw_enabled() const;

	void debug_set_draw_flag(DebugDrawFlag flag_index, bool enabled);
	bool debug_get_draw_flag(DebugDrawFlag flag_index) const;

	void debug_set_draw_shadow_occluders(bool enable);
	bool debug_get_draw_shadow_occluders() const;

#ifdef TOOLS_ENABLED
	void debug_set_draw_flags(uint32_t mask);
#endif

	Node3D *debug_dump_as_nodes(bool include_instancer) const;
	Error debug_dump_as_scene(String fpath, bool include_instancer) const;

	// 编辑器

#ifdef TOOLS_ENABLED
	void get_configuration_warnings(PackedStringArray &warnings) const override;
#endif // TOOLS_ENABLED

	// 内部

#ifdef VOXEL_ENABLE_INSTANCER
	void set_instancer(VoxelInstancer *instancer);
#endif

	VolumeID get_volume_id() const override {
		return _volume_id;
	}
	std::shared_ptr<StreamingDependency> get_streaming_dependency() const override {
		return _streaming_dependency;
	}

	Array get_mesh_block_surface(
			const Vector3i block_pos,
			const int lod_index,
			int &col_vertex_max,
			int &col_index_max
	) const;

	void get_meshed_block_positions_at_lod(int lod_index, StdVector<Vector3i> &out_positions) const;

	VoxelData &get_storage() const override;

	inline std::shared_ptr<VoxelData> get_storage_shared() const {
		return _data;
	}

	void get_lod_distances(Span<float> distances);

	void on_format_changed() override;

protected:
	void _notification(int p_what);

	void _on_gi_mode_changed() override;
	void _on_shadow_casting_changed() override;
	void _on_render_layers_mask_changed() override;

private:
	void process(float delta);
	void apply_quick_reloading_blocks();
	void apply_main_thread_update_tasks();

	void apply_mesh_update(VoxelEngine::BlockMeshOutput &ob);
	void apply_data_block_response(VoxelEngine::BlockDataOutput &ob);

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
	void apply_detail_texture_update(VoxelEngine::BlockDetailTextureOutput &ob);
	void apply_detail_texture_update_to_block(
			VoxelMeshBlockVLT &block,
			DetailTextureOutput &ob,
			unsigned int lod_index
	);
	void try_apply_parent_detail_texture_to_block(VoxelMeshBlockVLT &block, Vector3i bpos, unsigned int lod_index);
#endif

	void start_updater();
	void stop_updater();
	void start_streamer();
	void stop_streamer();
	void reset_maps();
	void reset_mesh_maps();

	Vector3 get_local_viewer_pos() const;
	void _set_lod_count(int p_lod_count);
	void set_mesh_block_visual_active(VoxelMeshBlockVLT &block, bool active, bool with_fading, unsigned int lod_index);

	void _on_stream_params_changed();

	void update_shader_material_pool_template();

	void save_all_modified_blocks(bool with_copy, std::shared_ptr<AsyncDependencyTracker> tracker);

	void process_deferred_collision_updates(uint32_t timeout_msec);
	void process_fading_blocks(float delta);

	struct LocalCameraInfo {
		Vector3 position;
		Vector3 forward;
	};

	LocalCameraInfo get_local_camera_info() const;

#ifdef TOOLS_ENABLED
	void update_gizmos();
#endif

	// 绑定

	Ref<VoxelSaveCompletionTracker> _b_save_modified_blocks();
	void _b_set_voxel_bounds(AABB aabb);
	AABB _b_get_voxel_bounds() const;

	Array _b_debug_print_sdf_top_down(Vector3i center, Vector3i extents);
	int _b_debug_get_mesh_block_count() const;
	int _b_debug_get_data_block_count() const;
	int /*Error*/ _b_debug_dump_as_scene(String fpath, bool include_instancer) const;

	bool _b_is_area_meshed(AABB aabb, int lod_index) const;

	Dictionary _b_get_statistics() const;

	static void _bind_methods();

private:
	VolumeID _volume_id;
	ProcessCallback _process_callback = PROCESS_CALLBACK_IDLE;

	Ref<Material> _material;
	bool _material_uses_lod_info = false;

	// 这个池存在的首要原因是这个：https://github.com/godotengine/godot/issues/34741
	// 数据块需要为多个特性使用各自的着色器参数，
	// 因此会创建大量使用同一着色器的 ShaderMaterial 副本。
	// 地形必须能在编辑器中运行，但在那种情况下，Godot 会将 Shader 的信号连接到
	// 使用它的每个 ShaderMaterial。Godot 这样做是为了在检查器中更新属性，如果着色器
	// 发生变化（这点值得商榷，因为只有被编辑的材质需要这样，甚至可能根本没被编辑！）。
	// 问题是，这也意味着每次调用 `ShaderMaterial::duplicate()` 时，当它赋值 `shader` 时，
	// 都必须向一个巨大的列表添加连接。这非常慢，足以导致卡顿。
	ShaderMaterialPoolVLT _shader_material_pool;

	FixedArray<VoxelMeshMap<VoxelMeshBlockVLT>, constants::MAX_LOD> _mesh_maps_per_lod;

	// 仅用于淡出的网格副本。
	// 在过渡掩码改变时使用。若不平滑淡出，可能会出现空洞。
	struct FadingOutMesh {
		// 相对于体积的局部空间坐标位置
		Vector3 local_position;
		voxel::godot::DirectMeshInstance mesh_instance;
		// 属性变化是我们可能想淡出网格的原因，因此我们可以在淡出开始前保留一份
		// 具有相应属性的材质副本。
		Ref<ShaderMaterial> shader_material;
		// 从 1 到 0 变化
		float progress;
	};

	// 这些是“发射后不管”
	StdVector<FadingOutMesh> _fading_out_meshes;

	unsigned int _collision_lod_count = 0;
	unsigned int _collision_layer = 1;
	unsigned int _collision_mask = 1;
	float _collision_margin = constants::DEFAULT_COLLISION_MARGIN;
	int _collision_update_delay = 0;
	FixedArray<StdVector<Vector3i>, constants::MAX_LOD> _deferred_collision_updates_per_lod;

	float _lod_fade_duration = 0.f;
	// 注意，指向网格数据块的直接指针应当是安全的，因为这些数据块总是在更新淡出数据块的同一
	// 线程上被销毁。若某个网格数据块被销毁，这些映射应同时更新。
	// TODO 优化：改用 FlatMap？需要检查有多少数据块会进入其中，可能不多
	FixedArray<StdMap<Vector3i, VoxelMeshBlockVLT *>, constants::MAX_LOD> _fading_blocks_per_lod;

	struct FadingDetailTexture {
		Vector3i block_position;
		uint32_t lod_index;
		float progress;
	};

	StdVector<FadingDetailTexture> _fading_detail_textures;

#ifdef VOXEL_ENABLE_INSTANCER
	VoxelInstancer *_instancer = nullptr;
#endif

	Ref<VoxelMesher> _mesher;

	// 数据以共享指针存储，以便发送给异步任务
	bool _threaded_update_enabled = false;
	std::shared_ptr<VoxelData> _data;
	std::shared_ptr<VoxelLodTerrainUpdateData> _update_data;
	std::shared_ptr<StreamingDependency> _streaming_dependency;
	std::shared_ptr<MeshingDependency> _meshing_dependency;

	struct ApplyMeshUpdateTask : public ITimeSpreadTask {
		void run(TimeSpreadTaskContext &ctx) override;

		VolumeID volume_id;
		VoxelLodTerrain *self = nullptr;
		VoxelEngine::BlockMeshOutput data;
	};

	FixedArray<StdUnorderedMap<Vector3i, RefCount>, constants::MAX_LOD> _queued_main_thread_mesh_updates;

#ifdef TOOLS_ENABLED
	bool _debug_draw_enabled = false;
	uint8_t _edited_blocks_gizmos_lod_index = 0;
	bool _debug_draw_shadow_occluders = false;
	uint16_t _debug_draw_flags = 0;

	voxel::godot::DebugRenderer _debug_renderer;

	struct DebugMeshUpdateItem {
		static constexpr uint32_t LINGER_FRAMES = 10;
		Vector3i position;
		uint32_t lod;
		uint32_t remaining_frames;
	};

	StdVector<DebugMeshUpdateItem> _debug_mesh_update_items;

	struct DebugEditItem {
		static constexpr uint32_t LINGER_FRAMES = 10;
		Box3i voxel_box;
		uint32_t remaining_frames;
	};

	StdVector<DebugEditItem> _debug_edit_items;
#endif

	Stats _stats;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelLodTerrain::ProcessCallback)
VARIANT_ENUM_CAST(voxel::VoxelLodTerrain::DebugDrawFlag)
VARIANT_ENUM_CAST(voxel::VoxelLodTerrain::StreamingSystem);

#endif // VOXEL_LOD_TERRAIN_HPP
