#ifndef VOXEL_BLOCKY_LIBRARY_H
#define VOXEL_BLOCKY_LIBRARY_H

#include "../../util/containers/std_vector.h"
#include "voxel_blocky_library_base.h"
#include "voxel_blocky_model.h"

namespace voxel {

// 以简单数组形式公开每个模型的库。数组中的索引对应于体素数据。
// 旋转和变体必须手动设置为单独的模型。如果你的模型很简单，
// 或者想组织自己的体素类型体系，可以使用这个库。
//
// 本应命名为 `VoxelBlockyModelLibrary` 或 `VoxelBlockyLibrarySimple`，但为了兼容之前的版本保留了此名称。
class VoxelBlockyLibrary : public VoxelBlockyLibraryBase {
	GDCLASS(VoxelBlockyLibrary, VoxelBlockyLibraryBase)

public:
	VoxelBlockyLibrary();
	~VoxelBlockyLibrary();

	// 加载默认模型
	void load_default() override;
	// 清空所有模型
	void clear() override;

	// 烘焙所有模型
	void bake() override;

	// 从资源名称获取对应的模型索引
	int get_model_index_from_resource_name(String resource_name) const;

	// 返回所添加模型索引的便捷方法
	int add_model(Ref<VoxelBlockyModel> model);

	//-------------------------
	// 内部使用

	// inline bool has_model(unsigned int id) const {
	// 	return id < _voxel_models.size() && _voxel_models[id].is_valid();
	// }

	// unsigned int get_model_count() const;

	// inline const VoxelBlockyModel &get_model_const(unsigned int id) const {
	// 	const Ref<VoxelBlockyModel> &model = _voxel_models[id];
	// 	VOXEL_ASSERT(model.is_valid());
	// 	return **model;
	// }

#ifdef TOOLS_ENABLED
	void get_configuration_warnings(PackedStringArray &out_warnings) const override;
#endif

private:
	Ref<VoxelBlockyModel> _b_get_model(unsigned int id) const;

	TypedArray<VoxelBlockyModel> _b_get_models() const;
	void _b_set_models(TypedArray<VoxelBlockyModel> models);

	bool _set(const StringName &p_name, const Variant &p_value);

	static void _bind_methods();

private:
	// 索引很重要，它们对应于体素数据
	StdVector<Ref<VoxelBlockyModel>> _voxel_models;
};

} // namespace voxel

#endif // VOXEL_BLOCKY_LIBRARY_H
