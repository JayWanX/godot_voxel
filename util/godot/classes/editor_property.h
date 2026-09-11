#include <core/version.h>
#ifndef VOXEL_GODOT_EDITOR_PROPERTY_H
#define VOXEL_GODOT_EDITOR_PROPERTY_H


#include <core/version.h>

#include <editor/inspector/editor_inspector.h>


#include "../../containers/span.h"

namespace voxel::godot {

// 这个晦涩的方法实际用于获取暴露坐标控件的 XYZW 着色颜色。
// 在模块中，这就是 `_get_property_colors`。
Span<const Color> editor_property_get_colors(EditorProperty &self);

class Voxel_EditorProperty : public EditorProperty {
	GDCLASS(Voxel_EditorProperty, EditorProperty)
public:
	void update_property() override;

protected:
	// 此方法在 core 中是受保护的，但依然可被重写。
	void _set_read_only(bool p_read_only) override;

protected:
	virtual void _voxel_update_property();
	virtual void _voxel_set_read_only(bool p_read_only);

private:
	static void _bind_methods() {}
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_EDITOR_PROPERTY_H
