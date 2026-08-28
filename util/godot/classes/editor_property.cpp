#include "editor_property.h"
#include "../../containers/fixed_array.h"

namespace voxel::godot {

Span<const Color> editor_property_get_colors(EditorProperty &self) {
	static FixedArray<Color, 4> s_colors;
	const StringName sn_editor("Editor");
	s_colors[0] = self.get_theme_color(StringName("property_color_x"), sn_editor);
	s_colors[1] = self.get_theme_color(StringName("property_color_y"), sn_editor);
	s_colors[2] = self.get_theme_color(StringName("property_color_z"), sn_editor);
	s_colors[3] = self.get_theme_color(StringName("property_color_w"), sn_editor);
	return to_span(s_colors);
}

#if defined(VOXEL_GODOT)

void VOXEL_EditorProperty::update_property() {
	_voxel_update_property();
}

#elif defined(VOXEL_GODOT_EXTENSION)

void VOXEL_EditorProperty::_update_property() {
	_voxel_update_property();
}

#endif

void VOXEL_EditorProperty::_set_read_only(bool p_read_only) {
	_voxel_set_read_only(p_read_only);
}

void VOXEL_EditorProperty::_voxel_update_property() {}
void VOXEL_EditorProperty::_voxel_set_read_only(bool p_read_only) {}

} // namespace voxel::godot
