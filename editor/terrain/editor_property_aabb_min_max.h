#ifndef VOXEL_EDITOR_PROPERTY_AABB_H
#define VOXEL_EDITOR_PROPERTY_AABB_H

#include "../../util/containers/fixed_array.h"
#include "../../util/godot/classes/editor_property.h"
#include "../../util/godot/classes/editor_spin_slider.h"
#include "../../util/macros.h"

namespace voxel {

// Alternative to the default AABB editor which presents it as a minimum and maximum point
class Voxel_EditorPropertyAABBMinMax : public voxel::godot::Voxel_EditorProperty {
	GDCLASS(Voxel_EditorPropertyAABBMinMax, voxel::godot::Voxel_EditorProperty);

public:
	Voxel_EditorPropertyAABBMinMax();

	void setup(double p_min, double p_max, double p_step, bool p_no_slider, const String &p_suffix = String());

protected:
	void _voxel_set_read_only(bool p_read_only) override;
	void _voxel_update_property() override;

private:
	void _on_value_changed(double p_val);
	void _notification(int p_what);

	static void _bind_methods();

	FixedArray<EditorSpinSlider *, 6> _spinboxes;
	bool _ignore_value_change = false;
};

} // namespace voxel

#endif // VOXEL_EDITOR_PROPERTY_AABB_H
