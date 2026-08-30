#ifndef VOXEL_MESH_SDF_GD_H
#define VOXEL_MESH_SDF_GD_H

#include "../storage/voxel_buffer_gd.h"
#include "../util/godot/classes/mesh.h"
#include "../util/godot/classes/resource.h"
#include "../util/math/vector3f.h"
#include "../util/thread/mutex.h"

#ifdef VOXEL_ENABLE_GPU
#include "../engine/gpu/compute_shader_resource.h"
#endif

VOXEL_GODOT_FORWARD_DECLARE(class SceneTree);

namespace voxel {

// 包含网格的已烘焙有符号距离场，可用于雕刻地形。
class VoxelMeshSDF : public Resource {
	GDCLASS(VoxelMeshSDF, Resource)
public:
	enum BakeMode { //
		BAKE_MODE_ACCURATE_NAIVE,
		BAKE_MODE_ACCURATE_PARTITIONED,
		BAKE_MODE_APPROX_INTERP,
		BAKE_MODE_APPROX_FLOODFILL,
		BAKE_MODE_COUNT
	};

	static const int MIN_CELL_COUNT = 2;
	static const int MAX_CELL_COUNT = 256;

	static constexpr float MIN_MARGIN_RATIO = 0.f;
	static constexpr float MAX_MARGIN_RATIO = 1.f;

	static const int MIN_PARTITION_SUBDIV = 2;
	static const int MAX_PARTITION_SUBDIV = 255;

	// 数据在烘焙之前无法使用
	bool is_baked() const;
	bool is_baking() const;

	// SDF 网格单元数量（分辨率）
	int get_cell_count() const;
	void set_cell_count(int cc);

	// 包围盒边距比例
	float get_margin_ratio() const;
	void set_margin_ratio(float mr);

	// 烘焙模式
	BakeMode get_bake_mode() const;
	void set_bake_mode(BakeMode mode);

	// 分区细分数量
	int get_partition_subdiv() const;
	void set_partition_subdiv(int subdiv);

	// 是否修复边界符号
	void set_boundary_sign_fix_enabled(bool enable);
	bool is_boundary_sign_fix_enabled() const;

	// 用于烘焙的网格
	void set_mesh(Ref<Mesh> mesh);
	Ref<Mesh> get_mesh() const;

	// 同步烘焙 SDF
	void bake();

// 使用任务系统的线程异步烘焙 SDF。
// TODO 不应需要 SceneTree 的引用！
// 目前需要它来确保创建 `VoxelServerUpdater`，以便它能驱动任务系统...
	void bake_async(SceneTree *scene_tree);

	// 访问已烘焙的 SDF 数据。
	// 警告：不要修改此缓冲区，只能从中读取。
	// 有一些用法（如修改器）会从不同线程读取它，
	// 但直接修改时没有线程安全保证。
	// TODO 引入 VoxelBufferReadOnly？因为这很可能是面向对象脚本 API 中唯一的方式...
	Ref<godot::VoxelBuffer> get_voxel_buffer() const;

	// 获取模型带内边距的包围盒。这对有符号距离的一致性很重要。
	AABB get_aabb() const;

	// 获取带边距包围盒的最小 / 最大角点位置
	inline Vector3f get_aabb_min_pos() const {
		return _min_pos;
	}
	inline Vector3f get_aabb_max_pos() const {
		return _max_pos;
	}

	#ifdef VOXEL_ENABLE_GPU
	// 获取 GPU 计算资源
	std::shared_ptr<ComputeShaderResource> get_gpu_resource();
	#endif

	// 调试：检查网格的 SDF 数据
	Array debug_check_sdf(Ref<Mesh> mesh);

private:
	void _on_bake_async_completed(Ref<godot::VoxelBuffer> buffer, Vector3 min_pos, Vector3 max_pos);

	Dictionary _b_get_data() const;
	void _b_set_data(Dictionary d);

	static void _bind_methods();

	// 数据
	Ref<godot::VoxelBuffer> _voxel_buffer;
	Vector3f _min_pos;
	Vector3f _max_pos;
#ifdef VOXEL_ENABLE_GPU
	// 以 shared_ptr 存储，以防该资源在重新生成时仍在使用中
	std::shared_ptr<ComputeShaderResource> _gpu_resource;
	Mutex _gpu_resource_mutex;
#endif

	// 状态
	bool _is_baking = false;

	// 烘焙选项
	int _cell_count = 64;
	float _margin_ratio = 0.25;
	BakeMode _bake_mode = BAKE_MODE_ACCURATE_PARTITIONED;
	uint8_t _partition_subdiv = 32;
	bool _boundary_sign_fix = true;
	// 注意，此处引用网格只是为了方便。将其设为 null 不会清除 SDF。
	// SDF 应该可以在不将原始网格加载到显卡的情况下使用。
	// 网格仅用于烘焙。
	Ref<Mesh> _mesh;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelMeshSDF::BakeMode);

#endif // VOXEL_MESH_SDF_GD_H
