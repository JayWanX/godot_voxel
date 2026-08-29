#include "voxel_instance_library_multimesh_item_inspector_plugin.h"
#include "../../terrain/instancing/voxel_instance_library_multimesh_item.h"
#include "../../util/godot/classes/button.h"
#include "../../util/godot/classes/label.h"
#include "../../util/godot/core/array.h"
#include "../../util/godot/core/string.h"
#include "voxel_instance_library_editor_plugin.h"
#include "voxel_instance_library_multimesh_item_editor_plugin.h"

#ifdef VOXEL_GODOT
#include "../../util/godot/core/callable_mp.h"
#include "../../util/godot/core/class_db.h"
#endif

namespace voxel {

bool VoxelInstanceLibraryMultiMeshItemInspectorPlugin::_voxel_can_handle(const Object *p_object) const {
	return Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(p_object) != nullptr;
}

void VoxelInstanceLibraryMultiMeshItemInspectorPlugin::_voxel_parse_group(Object *p_object, const String &p_group) {
	const VoxelInstanceLibraryMultiMeshItem *item = Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(p_object);
	ERR_FAIL_COND(item == nullptr);

	if (p_group == VoxelInstanceLibraryMultiMeshItem::MANUAL_SETTINGS_GROUP_NAME) {
		if (item->get_scene().is_null()) {
			ERR_FAIL_COND(listener == nullptr);
			// TODO 我更希望这个按钮在分组末尾，但 Godot 没有暴露任何方法来做这件事。
			// 这是一个遗留工作流，我们看看以后能否移除它。
			Button *button = memnew(Button);
			button->set_tooltip_text(
					VOXEL_TTR("Set properties based on an existing scene. This might copy mesh and material data if "
						   "the scene embeds them. Properties will not update if the scene changes later.")
			);
			button->set_text(VOXEL_TTR("Update from scene..."));

			// 使用 bind() 而不是依赖编辑器插件中“当前编辑的”条目，这样可以支持
			// 多个子检查器。插件不会按被检查对象实例化，但自定义控件会。
			button->connect(
					"pressed",
					callable_mp(
							listener,
							&VoxelInstanceLibraryMultiMeshItemEditorPlugin::_on_update_from_scene_button_pressed
					)
							.bind(item)
			);

			add_custom_control(button);

		} else {
			Label *label = memnew(Label);
			label->set_text(VOXEL_TTR("Properties are defined by the scene property."));
			add_custom_control(label);
		}
	}
	// TODO 在编辑器中打开场景的按钮，因为 Godot 的资源选择器菜单里没有这个？
	// 也许这更应该作为对 Godot 的功能请求。
	// else if (p_group == VoxelInstanceLibraryMultiMeshItem::SCENE_SETTINGS_GROUP_NAME) {
	// 	ERR_FAIL_COND(listener == nullptr);
	// 	Button *button = memnew(Button);
	// 	button->set_text(TTR("Open scene in editor"));
	// }
}

bool VoxelInstanceLibraryMultiMeshItemInspectorPlugin::_voxel_parse_property(
		Object *p_object,
		const Variant::Type p_type,
		const String &p_path,
		const PropertyHint p_hint,
		const String &p_hint_text,
		const BitField<PropertyUsageFlags> p_usage,
		const bool p_wide
) {
	// TODO 在检查一个 Array 的条目时，Godot 会在所有编辑器插件上调用 `parse_property`！
	// 参见 https://github.com/godotengine/godot/issues/71236
	if (p_object == nullptr) {
		// 我们不关心非对象属性
		return false;
	}
	// 如果指定了场景，则隐藏手动属性，因为场景会覆盖它们
	const VoxelInstanceLibraryMultiMeshItem *item = Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(p_object);
	ERR_FAIL_COND_V_MSG(
			item == nullptr,
			false,
			String("Did not expect {0}, see https://github.com/godotengine/godot/issues/71236")
					.format(varray(p_object->get_class()))
	);
	if (item->get_scene().is_null()) {
		return false;
	}
	// TODO 我只想让 "Manual properties" 分类中的所有属性变为只读……
	// 但 Godot 似乎没有暴露任何实现它的方法，所以我只能一个一个地隐藏它们
	static const char *s_manual_properties[] = {
		"cast_shadow", //
		"collision_layer", //
		"collision_mask", //
		"collision_shapes", //
		"material_override", //
		"render_layer", //
		"mesh", //
		"mesh_lod1", //
		"mesh_lod2", //
		"mesh_lod3" //
	};
	for (const char *name : s_manual_properties) {
		if (p_path == name) {
			return true;
		}
	}
	return false;
}

} // namespace voxel
