#ifndef VOXEL_GODOT_EDITOR_PROPERTY_H
#define VOXEL_GODOT_EDITOR_PROPERTY_H

#if defined(VOXEL_GODOT)

#include "../core/version.h"

#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR <= 4
#include <editor/editor_inspector.h>
#else
#include <editor/inspector/editor_inspector.h>
#endif

#endif

#include "../../containers/span.h"

namespace voxel::godot {

// This obscure method is actually used to get the XYZW tinting colors for controls that expose coordinates.
// In modules, this is `_get_property_colors`.
Span<const Color> editor_property_get_colors(EditorProperty &self);

class Voxel_EditorProperty : public EditorProperty {
	GDCLASS(Voxel_EditorProperty, EditorProperty)
public:
#if defined(VOXEL_GODOT)
	void update_property() override;
#endif

#ifdef VOXEL_GODOT
protected:
#endif
	// This method is protected in core, but still overridable.
	void _set_read_only(bool p_read_only) override;

protected:
	virtual void _voxel_update_property();
	virtual void _voxel_set_read_only(bool p_read_only);

private:
	static void _bind_methods() {}
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_EDITOR_PROPERTY_H
