#include "voxel_instance_library_inspector_plugin.h"
#include "../../constants/voxel_string_names.h"
#include "control_sizer.h"
#include "voxel_instance_library_editor_plugin.h"
#include "voxel_instance_library_list_editor.h"

namespace voxel {

bool VoxelInstanceLibraryInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return Object::cast_to<VoxelInstanceLibrary>(p_object) != nullptr;
}

void VoxelInstanceLibraryInspectorPlugin::_voxel_parse_begin(Object *p_object) {
	// TODO 我怎样才能确保这些按钮位于 "VoxelInstanceLibrary" 分类的开头？
	// 比起 Spatial 编辑器工具栏（如果你不在 3D 标签页，它会被隐藏），这里是个更好的位置，
	// 但它会出现在检查器的最顶部，甚至在 "VoxelInstanceLibrary" 属性分类
	// 之上。这看起来有点别扭，而且如果该类被继承，就会开始变得
	// 混乱，因为这些按钮针对的是 "VoxelInstanceLibrary" 特有的属性列表。
	// 我不能使用 `parse_property` 或 `parse_category`，因为当列表为空时，
	// 该类既不返回属性，也不返回分类。
}

bool VoxelInstanceLibraryInspectorPlugin::_voxel_parse_property(
		Object *p_object,
		const Variant::Type p_type,
		const String &p_path,
		const PropertyHint p_hint,
		const String &p_hint_text,
		const BitField<PropertyUsageFlags> p_usage,
		const bool p_wide
) {
	// 我们用这个属性作为锚点，把我们的列表放在它上面
	if (p_path == "_selected_item") {
		Ref<VoxelInstanceLibrary> library(Object::cast_to<VoxelInstanceLibrary>(p_object));

		VoxelInstanceLibraryListEditor *list_editor = memnew(VoxelInstanceLibraryListEditor);
		list_editor->setup(icon_provider, plugin);
		list_editor->set_library(library);
		add_custom_control(list_editor);

		Voxel_ControlSizer *sizer = memnew(Voxel_ControlSizer);
		sizer->set_target_control(list_editor);
		add_custom_control(sizer);
	}
	return false;
}

} // namespace voxel
