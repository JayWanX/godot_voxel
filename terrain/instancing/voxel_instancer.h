#ifndef VOXEL_INSTANCER_H
#define VOXEL_INSTANCER_H

#include "../../constants/voxel_constants.h"
#include "../../streams/instance_data.h"
#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_unordered_set.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/node_3d.h"
#include "../../util/godot/direct_multimesh_instance.h"
#include "../../util/math/box3i.h"
#include "../../util/memory/memory.h"
#include "instance_library_item_listener.h"
#include "up_mode.h"

#ifdef TOOLS_ENABLED
#include "../../util/godot/core/version.h"
#include "../../util/godot/debug_renderer.h"
#endif

#include <limits>

// 我实现了一个替代 API 以更轻松地自定义实例查询。其方法是使用虚函数和
// 临时缓冲区而不是模板，因为模板会导致大量膨胀。但当用于现有的
// 浮动移除函数时，它使其略慢（最多长 20%）。因此保留旧实现。
// 注意：如果我们使用自己的实例缓存而不是从 Godot 获取，性能差异可能可以
// 消除。
#define VOXEL_INSTANCER_USE_SPECIALIZED_FLOATING_INSTANCE_REMOVAL_IMPLEMENTATION

VOXEL_GODOT_FORWARD_DECLARE(class PhysicsBody3D);

namespace voxel {

class AsyncDependencyTracker;

class VoxelNode;
class VoxelInstancerRigidBody;
class VoxelInstanceComponent;
class VoxelInstanceLibrary;
class VoxelInstanceLibraryItem;
class VoxelInstanceLibrarySceneItem;
class VoxelInstanceLibraryMultiMeshItem;
class VoxelTool;
class SaveBlockDataTask;
class BufferedTaskScheduler;
struct InstanceBlockData;
struct InstancerQuickReloadingCache;
struct InstancerTaskOutputQueue;
struct InstanceLibraryMultiMeshItemSettings;

// 注意：此节点的大部分内容可以通用化，以支持仅在卦限内实例化的想法？
// 如果“网格”的概念被解耦，即使是 gridmap 之类的节点也可以在此之上重建。
// 由于性能原因，它目前与地形耦合。

// 体素节点的附加组件，允许在表面生成元素。
// 这些元素使用硬件实例化渲染，可以具有碰撞，也可以持久化。
class VoxelInstancer : public Node3D, public IInstanceLibraryItemListener {
	GDCLASS(VoxelInstancer, Node3D)
public:
	static const int MAX_LOD = 8;

	// 我不想在 C++ 侧放置此枚举，因为它会阻止前置声明其所在的类。
	// 但 Godot 迫使我这样做。
	// `VARIANT_ENUM_CAST(ns1::ns2::Enum)` 假定枚举在类中，因此它生成的名称是 `ns2.Enum`，
	// 这会让文档混乱。似乎也没有办法将该枚举注册为全局。
	using UpMode = voxel::UpMode;

	VoxelInstancer();
	~VoxelInstancer();

	// 属性

	// 实例的"向上"方向
	void set_up_mode(UpMode mode);
	UpMode get_up_mode() const;

	// 实例库
	void set_library(Ref<VoxelInstanceLibrary> library);
	Ref<VoxelInstanceLibrary> get_library() const;

	// 每帧用于网格 LOD 更新的预算
	int get_mesh_lod_update_budget_microseconds() const;
	void set_mesh_lod_update_budget_microseconds(const int p_micros);

	// 每帧用于碰撞更新的预算
	int get_collision_update_budget_microseconds() const;
	void set_collision_update_budget_microseconds(const int p_micros);

	// 是否启用淡入淡出
	void set_fading_enabled(const bool enabled);
	bool get_fading_enabled() const;

	// 淡入淡出时长
	void set_fading_duration(const float fading);
	float get_fading_duration() const;

	// 操作

	// 保存所有已修改的数据块
	void save_all_modified_blocks(
			BufferedTaskScheduler &tasks,
			std::shared_ptr<AsyncDependencyTracker> tracker,
			bool with_flush
	);

	// 移除球体内的实例
	void remove_instances_in_sphere(const Vector3 p_center, const float p_radius);

	enum NodeConversionFlags {
		NODE_CONVERSION_DUPLICATE_MESHES = 1 << 0,
		NODE_CONVERSION_DUPLICATE_MULTIMESHES = 1 << 1,
		NODE_CONVERSION_INCLUDE_MATERIAL_OVERRIDES = 1 << 2
	};

	// 将实例转换为场景节点
	Node3D *convert_to_nodes(const uint32_t flags) const;

	// 事件处理程序

	// void on_data_block_loaded(Vector3i grid_position, unsigned int lod_index, UniquePtr<InstanceBlockData>
	// instances);
	// 网格块进入视口时的回调
	void on_mesh_block_enter(
			const Vector3i render_grid_position,
			const unsigned int lod_index,
			Array surface_arrays,
			const int32_t vertex_range_end,
			const int32_t index_range_end
	);
	// 网格块退出视口时的回调
	void on_mesh_block_exit(const Vector3i render_grid_position, const unsigned int lod_index);
	// 体素区域被编辑时的回调
	void on_area_edited(Box3i p_voxel_box);
	// 刚体被移除时的回调
	void on_body_removed(Vector3i data_block_position, unsigned int render_block_index, unsigned int instance_index);
	// 场景实例被移除时的回调
	void on_scene_instance_removed(
			Vector3i data_block_position,
			unsigned int render_block_index,
			unsigned int instance_index
	);
	// 场景实例被修改时的回调
	void on_scene_instance_modified(Vector3i data_block_position, unsigned int render_block_index);
	// 数据块被保存时的回调
	void on_data_block_saved(Vector3i data_grid_position, unsigned int lod_index);

	// 内部属性

	// 设置网格块与数据块尺寸
	void set_mesh_block_size_po2(unsigned int p_mesh_block_size_po2);
	void set_data_block_size_po2(unsigned int p_data_block_size_po2);
	// 从父节点更新网格 LOD 距离
	void update_mesh_lod_distances_from_parent();

	// 从渲染块索引获取库项目 ID
	int get_library_item_id_from_render_block_index(unsigned render_block_index) const;

	// 调试

	// 数据块数量
	int debug_get_block_count() const;
	// 统计各图层的实例数量
	void debug_get_instance_counts(StdUnorderedMap<uint32_t, uint32_t> &counts_per_layer) const;
	// 将实例导出为场景文件
	void debug_dump_as_scene(String fpath) const;
	// 将实例导出为节点树
	Node *debug_dump_as_nodes() const;

	// 是否启用调试绘制
	void debug_set_draw_enabled(bool enabled);
	bool debug_is_draw_enabled() const;

	enum DebugDrawFlag { //
		DEBUG_DRAW_ALL_BLOCKS,
		DEBUG_DRAW_EDITED_BLOCKS,
		DEBUG_DRAW_FLAGS_COUNT
	};

	// 设置或获取调试绘制标志
	void debug_set_draw_flag(DebugDrawFlag flag_index, bool enabled);
	bool debug_get_draw_flag(DebugDrawFlag flag_index) const;

	// 获取指定位置的块调试信息
	Dictionary debug_get_block_infos(const Vector3 world_position, const int item_id);

	// 编辑器

#ifdef TOOLS_ENABLED
#if defined(VOXEL_GODOT)
	PackedStringArray get_configuration_warnings() const override;
#endif
	virtual void get_configuration_warnings(PackedStringArray &warnings) const;
#endif

protected:
	void _notification(int p_what);

private:
	struct Layer;

	void process();
	void process_task_results();
	void process_mesh_lods();
	void process_collision_distances();
	void process_fading();

	void add_layer(int layer_id, int lod_index);
	void remove_layer(int layer_id);
	unsigned int create_block(
			Layer &layer,
			const uint16_t layer_id,
			const Vector3i grid_position,
			const bool pending_instances
	);
	void remove_block(const unsigned int block_index, const bool with_fade_out);
	void set_world(World3D *world);
	void clear_blocks();
	void clear_blocks_in_layer(int layer_id);
	void clear_layers();
	void update_visibility();
	SaveBlockDataTask *save_block(
			Vector3i data_grid_pos,
			int lod_index,
			std::shared_ptr<AsyncDependencyTracker> tracker,
			bool with_flush,
			bool cache_while_saving
	);

	// 获取一个图层，假定它存在
	Layer &get_layer(int id);
	const Layer &get_layer_const(int id) const;

	void regenerate_layer(uint16_t layer_id, bool regenerate_blocks);
	void update_layer_meshes(int layer_id);
	void update_layer_scenes(int layer_id);
	void create_render_blocks(
			const Vector3i grid_position,
			const int lod_index,
			Array surface_arrays,
			const int32_t vertex_range_end,
			const int32_t index_range_end
	);

	struct Block;

	class IAreaOperation {
	public:
		struct Result {
			bool modified;
		};
		virtual ~IAreaOperation() {}
		virtual Result execute(Block &block) = 0;
	};

	void do_area_operation(const AABB p_aabb, IAreaOperation &op);
	void do_area_operation(const Box3i p_voxel_box, IAreaOperation &op);

#ifdef TOOLS_ENABLED
	void process_gizmos();
#endif

	struct SceneInstance {
		// 由场景树拥有。
		VoxelInstanceComponent *component = nullptr;
		Node3D *root = nullptr;
	};

	SceneInstance create_scene_instance(
			const VoxelInstanceLibrarySceneItem &scene_item,
			int instance_index,
			unsigned int block_index,
			Transform3D transform,
			int data_block_size_po2
	);

	void update_block_from_transforms(
			int block_index,
			Span<const Transform3f> transforms,
			const Vector3i grid_position,
			Layer &layer,
			const VoxelInstanceLibraryItem &item_base,
			const uint16_t layer_id,
			World3D &world,
			const Transform3D &block_transform,
			const Vector3 block_local_position
	);

	void update_multimesh_block_from_transforms(
			Block &block,
			const unsigned int block_index,
			const Transform3D &block_global_transform,
			const Vector3 block_local_position,
			Span<const Transform3f> transforms,
			const VoxelInstanceLibraryMultiMeshItem &item,
			World3D &world
	);

	void update_scene_block_from_transforms(
			Block &block,
			const unsigned int block_index,
			const Vector3 block_local_position,
			Span<const Transform3f> transforms,
			const VoxelInstanceLibrarySceneItem &scene_item
	);

	void update_multimesh_block_colliders(
			Block &block,
			const uint32_t block_index,
			const InstanceLibraryMultiMeshItemSettings &settings,
			Span<const Transform3f> transforms,
			const Vector3 block_local_position
	);

	void destroy_multimesh_block_colliders(Block &block);

	void on_library_item_changed(int item_id, IInstanceLibraryItemListener::ChangeType change) override;

	struct MMRemovalAction {
		struct Context {
			VoxelInstancer *instancer = nullptr;
			VoxelInstanceLibraryMultiMeshItem *item = nullptr;
		};
		Context context;

		typedef void (*Callback)(Context, const Transform3D &);
		Callback callback = nullptr;

		inline bool is_valid() const {
			return callback != nullptr;
		}

		inline void call(const Transform3D &t) const {
#ifdef DEV_ENABLED
			VOXEL_ASSERT(callback != nullptr);
#endif
			(*callback)(context, t);
		}
	};

	static MMRemovalAction get_mm_removal_action(VoxelInstancer *instancer, VoxelInstanceLibraryMultiMeshItem *mm_item);

	void remove_floating_instances(const Box3i voxel_box);

#ifdef VOXEL_INSTANCER_USE_SPECIALIZED_FLOATING_INSTANCE_REMOVAL_IMPLEMENTATION
	static void remove_floating_multimesh_instances(
			Block &block,
			const Transform3D &parent_transform,
			const Box3i p_voxel_box,
			const VoxelTool &voxel_tool,
			const int block_size_po2,
			const float sd_threshold,
			const float sd_offset,
			const bool bidirectional,
			const MMRemovalAction removal_action
	);

	static void remove_floating_scene_instances(
			Block &block,
			const Transform3D &parent_transform,
			const Box3i p_voxel_box,
			const VoxelTool &voxel_tool,
			const int block_size_po2,
			const float sd_threshold,
			const float sd_offset,
			const bool bidirectional
	);
#endif

	static void get_instance_positions_local(
			const Block &block,
			const unsigned int base_block_size,
			StdVector<Vector3f> &dst_positions,
			StdVector<Vector3f> *dst_normals
	);

	static void get_instance_transforms_local(const Block &block, StdVector<Transform3f> &dst);

	static void remove_instances_by_index(
			Block &block,
			const uint32_t base_block_size,
			Span<const uint32_t> ascending_indices,
			const MMRemovalAction mm_removal_action
	);

	static void remove_scene_instances_by_index(Block &block, Span<const uint32_t> ascending_indices);

	static void remove_multimesh_instances_by_index(
			Block &block,
			const uint32_t base_block_size,
			Span<const uint32_t> ascending_indices,
			const MMRemovalAction action
	);

	static void update_mesh_from_mesh_lod(
			Block &block,
			const InstanceLibraryMultiMeshItemSettings &settings,
			bool hide_beyond_max_lod,
			bool instancer_is_visible
	);

	Dictionary _b_debug_get_instance_counts() const;

	static void _bind_methods();

	// TODO 重命名 RenderBlock？
	struct Block {
		uint16_t layer_id = 0;
		// 基于距离的 LOD 索引。
		// 如果应在最后一个 LOD 之外隐藏，则可以比最大网格 LOD 计数高一个索引
		uint8_t current_mesh_lod = 0;
		// 与地形的底层数据块系统对应的 LOD 索引
		uint8_t lod_index = 0;
		// 如果为 true，则该数据块正在等待异步填充。我们以此状态创建数据块，
		// 以便异步生成完成时可以检查该数据块是否仍然存在。
		// TODO 未使用？
		bool pending_instances = false;
		// 用于距离过滤碰撞体功能
		bool distance_colliders_active = false;
		// 网格数据块坐标系中的位置
		Vector3i grid_position;
		voxel::godot::DirectMultiMeshInstance multimesh_instance;
		// 物理方面我们使用节点，因为更易于管理。
		// 此类实例的数量可能较少。
		// 如果与此数据块关联的项目没有碰撞，则为空。
		// 向量中的索引对应于 multimesh 中实例的索引。
		StdVector<VoxelInstancerRigidBody *> bodies;
		StdVector<SceneInstance> scene_instances;
	};

	struct Layer {
		unsigned int lod_index;
		// 数据块按网格位置索引。
		// 键遵循网格数据块坐标系。
		StdUnorderedMap<Vector3i, unsigned int> blocks;
	};

	struct MeshLodDistances {
		// Multimesh LOD 基于相机与数据块中心之间的距离进行更新。
		// 使用两个距离来实现滞回，从而避免在 LOD 之间过快振荡。

		// TODO 需要调查 Godot 4 是否为 multimesh 实现了 LOD
		// 尽管如此，由于 Godot 4 实现 LOD 的方式，拥有自定义 LOD 系统可能仍然有益，
		// 这样我们可以切换到替身（impostor）而不仅仅是简化几何体

		// 超过此距离后网格开始使用，优先于距离较低的网格。
		float enter_distance_squared;
		// 低于此距离后网格停止使用
		float exit_distance_squared;
	};

	struct Lod : public NonCopyable {
		// 使用此 LOD 级别的图层 ID 的无序列表。
		StdVector<int> layers;

		// 具有未保存更改的数据块。
		// 键遵循数据块坐标系。
		// 可以包含不存在实例数据块的坐标（可能因为支持渲染数据块
		// 大两倍而发生；不理想，但不应导致问题）
		StdUnorderedSet<Vector3i> modified_blocks;

		// 这是一个临时位置，用于在实例数据尚不可见时存储已加载的实例数据。
		// 这些实例是用户创作的实例。如果数据块在此处没有条目，
		// 它将获得生成的实例。
		// 键遵循数据块坐标系。
		// 不能使用 Godot 的 `HashMap`，因为它缺少移动语义。
		// StdUnorderedMap<Vector3i, UniquePtr<InstanceBlockData>> loaded_instances_data;

		// 包含已编辑数据（非生成数据）的数据块。
		// 键遵循数据块坐标系。
		StdUnorderedSet<Vector3i> edited_data_blocks;

		std::shared_ptr<InstancerQuickReloadingCache> quick_reload_cache;

		// FixedArray<MeshLodDistances, VoxelInstanceLibraryMultiMeshItem::MAX_MESH_LODS> mesh_lod_distances;
	};

	UpMode _up_mode = UP_MODE_POSITIVE_Y;

	FixedArray<Lod, MAX_LOD> _lods;

	// 不包含空值。索引很重要。
	StdVector<UniquePtr<Block>> _blocks;

	// 每个图层对应一个库项目。map 中值的地址应保持稳定。
	StdUnorderedMap<int, Layer> _layers;

	Ref<VoxelInstanceLibrary> _library;

	VoxelNode *_parent = nullptr;
	unsigned int _parent_data_block_size_po2 = constants::DEFAULT_BLOCK_SIZE_PO2;
	unsigned int _parent_mesh_block_size_po2 = constants::DEFAULT_BLOCK_SIZE_PO2;
	FixedArray<float, MAX_LOD> _mesh_lod_distances;
	// Vector3 _mesh_lod_last_update_camera_position;
	// float _mesh_lod_update_camera_threshold_distance = 8.f;
	unsigned int _mesh_lod_time_sliced_block_index = 0;
	uint32_t _mesh_lod_update_budget_microseconds = 500;

	unsigned int _collision_distance_time_sliced_block_index = 0;
	uint32_t _collision_distance_update_budget_microseconds = 500;

	std::shared_ptr<InstancerTaskOutputQueue> _loading_results;

	struct FadingInBlock {
		uint16_t layer_id = 0;
		Vector3i grid_position;
		float progress = 0.f;
	};

	struct FadingOutBlock {
		float progress = 0.f;
		voxel::godot::DirectMultiMeshInstance multimesh_instance;
	};

	StdVector<FadingInBlock> _fading_in_blocks;
	StdVector<FadingOutBlock> _fading_out_blocks;
	float _fading_duration = 0.3f;
	bool _fading_enabled = false;

#ifdef TOOLS_ENABLED
	voxel::godot::DebugRenderer _debug_renderer;
	bool _gizmos_enabled = false;
	uint8_t _debug_draw_flags = 0;
#endif
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelInstancer::UpMode);
VARIANT_ENUM_CAST(voxel::VoxelInstancer::DebugDrawFlag);

#endif // VOXEL_INSTANCER_H
