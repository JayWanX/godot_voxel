#ifndef VOXEL_GODOT_DEBUG_RENDERER_H
#define VOXEL_GODOT_DEBUG_RENDERER_H

#include "../containers/std_vector.h"
#include "classes/standard_material_3d.h"
#include "direct_multimesh_instance.h"

namespace voxel::godot {

// Helper to draw 3D primitives every frame for debugging purposes
class DebugRenderer {
public:
	DebugRenderer();
	~DebugRenderer();

	// This class does not uses nodes. Call this first to choose in which world it renders.
	void set_world(World3D *world);

	// Call this before issuing drawing commands
	void begin();

	// Draws a box wireframe.
	// The box's origin is its lower corner. Size is defined by the transform's basis.
	void draw_box(const Transform3D &t, Color8 color);

	// Call this after issuing all drawing commands
	void end();

	void clear();

private:
	void init();
	bool _initialized = false;

	StdVector<DirectMultiMeshInstance::TransformAndColor32> _items;
	Ref<MultiMesh> _multimesh;
	DirectMultiMeshInstance _multimesh_instance;
	// TODO World3D is a reference, do not store it by pointer
	World3D *_world = nullptr;
	bool _inside_block = false;
	PackedFloat32Array _bulk_array;
	Ref<StandardMaterial3D> _material;
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_DEBUG_RENDERER_H
