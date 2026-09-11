#include "voxel_blocky_type_library_ids_dialog.h"
#include "../../../util/containers/container_funcs.h"
#include "../../../util/containers/std_vector.h"
#include "../../../util/godot/classes/button.h"
#include <scene/gui/item_list.h>
#include <scene/gui/box_container.h>
#include "../../../util/godot/core/string.h"
#include <core/version.h>
#include <editor/themes/editor_scale.h>

namespace voxel {

VoxelBlockyTypeLibraryIDSDialog::VoxelBlockyTypeLibraryIDSDialog() {
	const float editor_scale = EDSCALE;

	set_title(VOXEL_TTR("VoxelBlockyTypeLibrary model IDs"));
	set_min_size(Vector2(300, 300) * editor_scale);

	VBoxContainer *v_box_container = memnew(VBoxContainer);
	v_box_container->set_anchor(SIDE_RIGHT, 1);
	v_box_container->set_anchor(SIDE_BOTTOM, 1);
	v_box_container->set_offset(SIDE_LEFT, 4 * editor_scale);
	v_box_container->set_offset(SIDE_TOP, 4 * editor_scale);
	v_box_container->set_offset(SIDE_RIGHT, -4 * editor_scale);
	v_box_container->set_offset(SIDE_BOTTOM, -4 * editor_scale);

	_item_list = memnew(ItemList);
	_item_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	v_box_container->add_child(_item_list);

	Button *button = get_ok_button();
	button->set_custom_minimum_size(Vector2(100 * EDSCALE, 0));

	add_child(v_box_container);
}

void VoxelBlockyTypeLibraryIDSDialog::set_library(Ref<VoxelBlockyTypeLibrary> library) {
	VOXEL_ASSERT_RETURN(library.is_valid());

	PackedStringArray id_map;
	StdVector<uint16_t> used_ids;

	// 我们可以在每次打开此对话框时更新 ID 映射，但严格来说那是一种修改，而且意味着用户
	// 在编辑过程中 ID 更有可能变成未使用。因此我们改用
	// 一个特殊函数：复制当前的 ID 映射并更新副本。
	library->get_id_map_preview(id_map, used_ids);

	_item_list->clear();
	for (int i = 0; i < id_map.size(); ++i) {
		const String name = id_map[i];
		String item_name = String::num_int64(i) + ": " + name;

		// 未优化，需要时再优化
		const bool used = contains(to_span_const(used_ids), uint16_t(i));

		if (!used) {
			item_name += " (unused)";
		}

		const int item_index = _item_list->add_item(item_name);

		if (!used) {
			_item_list->set_item_custom_fg_color(item_index, Color(0.5, 0.5, 0.5));
		}
	}
}

} // namespace voxel
