#ifndef VOXEL_BLOCKY_LIBRARY_BASE_H
#define VOXEL_BLOCKY_LIBRARY_BASE_H

#include "../../util/containers/dynamic_bitset.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/material.h"
#include "../../util/godot/classes/resource.h"
#include "../../util/thread/rw_lock.h"
#include "blocky_baked_library.h"

namespace voxel {

// 可与 VoxelMesherBlocky 一起使用的库的基类。
// 库提供一组预处理过的模型，这些模型可以高效地批量合并进体素网格。
// 根据库的类型不同，这些模型的提供方式也不同。
class VoxelBlockyLibraryBase : public Resource {
	GDCLASS(VoxelBlockyLibraryBase, Resource)

public:
	static constexpr unsigned int MAX_MODELS = blocky::MAX_MODELS;
	static constexpr unsigned int MAX_FLUIDS = blocky::MAX_FLUIDS;
	static constexpr unsigned int MAX_MATERIALS = blocky::MAX_MATERIALS;

	static constexpr uint32_t NULL_INDEX = 0xFFFFFFFF;

	bool get_bake_tangents() const {
		return _bake_tangents;
	}
	void set_bake_tangents(bool bt);

	//

	virtual void load_default();
	virtual void clear();

	virtual void bake();

	//-------------------------
	// 内部使用

	const blocky::BakedLibrary &get_baked_data() const {
		return _baked_data;
	}
	const RWLock &get_baked_data_rw_lock() const {
		return _baked_data_rw_lock;
	}

	Ref<Material> get_material_by_index(unsigned int index) const;
	unsigned int get_material_index_count() const;

#ifdef TOOLS_ENABLED
	virtual void get_configuration_warnings(PackedStringArray &out_warnings) const;
#endif

private:
	// 获取烘焙后所有已索引材质的便捷方法，
	// 可传给 VoxelMesher::build 用于测试
	TypedArray<Material> _b_get_materials() const;
	void _b_bake();

	static void _bind_methods();

protected:
	int _legacy_atlas_size = 16;
	bool _needs_baking = true;
	bool _bake_tangents = true;

	// 网格生成器在多线程环境中使用。除 bake() 外不要修改它。
	RWLock _baked_data_rw_lock;
	blocky::BakedLibrary _baked_data;
	// 其中一个条目可以为 null，表示"默认材质"。如果所有非空模型都有材质，则不会有 null 条目。
	StdVector<Ref<Material>> _indexed_materials;
};

namespace blocky {

void generate_side_culling_matrix(blocky::BakedLibrary &baked_data);

} // namespace blocky

} // namespace voxel

#endif // VOXEL_BLOCKY_LIBRARY_BASE_H
