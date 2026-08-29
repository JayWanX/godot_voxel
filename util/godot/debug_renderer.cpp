#include "debug_renderer.h"
#include "../containers/fixed_array.h"
#include "../memory/memory.h"
#include "classes/array_mesh.h"
#include "core/packed_arrays.h"
#include "direct_mesh_instance.h"
#include "direct_multimesh_instance.h"

namespace voxel::godot {

namespace {

Ref<Mesh> create_debug_wirecube(Color color) {
	const Vector3 positions_raw[] = {
		Vector3(0, 0, 0), //
		Vector3(1, 0, 0), //
		Vector3(1, 0, 1), //
		Vector3(0, 0, 1), //
		Vector3(0, 1, 0), //
		Vector3(1, 1, 0), //
		Vector3(1, 1, 1), //
		Vector3(0, 1, 1) //
	};
	PackedVector3Array positions;
	copy_to(positions, Span<const Vector3>(positions_raw, 8));

	PackedColorArray colors;
	// 不预先调整数组大小，改为逐项追加写入。
	for (int i = 0; i < positions.size(); ++i) {
		colors.push_back(color);
	}

	const int32_t indices_raw[] = {
		0, 1, //
		1, 2, //
		2, 3, //
		3, 0, //

		4, 5, //
		5, 6, //
		6, 7, //
		7, 4, //

		0, 4, //
		1, 5, //
		2, 6, //
		3, 7 //
	};
	PackedInt32Array indices;
	copy_to(indices, Span<const int32_t>(indices_raw, 24));

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = positions;
	arrays[Mesh::ARRAY_COLOR] = colors;
	arrays[Mesh::ARRAY_INDEX] = indices;
	Ref<ArrayMesh> mesh = memnew(ArrayMesh);
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);

	return mesh;
}

} // namespace

DebugRenderer::DebugRenderer() {}

void DebugRenderer::init() {
	_multimesh_instance.create();
	_multimesh_instance.set_interpolated(false);
	// TODO 开启阴影投射时，平行光阴影会完全失效。
	// 原因仍不清楚。
	// 反正它本就该关闭，但这种情况仍令人担忧。
	_multimesh_instance.set_cast_shadows_setting(RenderingServerEnums::SHADOW_CASTING_SETTING_OFF);
	_multimesh.instantiate();
	Ref<Mesh> wirecube = create_debug_wirecube(Color(1, 1, 1));
	_multimesh->set_mesh(wirecube);
	_multimesh->set_transform_format(MultiMesh::TRANSFORM_3D);
	// TODO 优化：Godot 需要在 multimesh 上恢复 8 位颜色属性，32 位颜色太浪费了
	//_multimesh->set_color_format(MultiMesh::COLOR_8BIT);
	_multimesh->set_use_colors(true);
	_multimesh->set_use_custom_data(false);
	_multimesh_instance.set_multimesh(_multimesh);
	_material.instantiate();
	_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	_material->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	_multimesh_instance.set_material_override(_material);
}

DebugRenderer::~DebugRenderer() {
	// 不使用节点创建的 MultiMeshInstance 不会持有其材质的引用。
	// 因此需要在本析构函数末尾释放所有权之前先销毁它。
	// 否则 RenderingServer 会报错。
	_multimesh_instance.destroy();
}

void DebugRenderer::set_world(World3D *world) {
	if (_initialized == false) {
		init();
		_initialized = true;
	}

	_multimesh_instance.set_world(world);
	_world = world;
}

void DebugRenderer::begin() {
	ERR_FAIL_COND(_inside_block);
	ERR_FAIL_COND(_world == nullptr);
	_inside_block = true;
}

void DebugRenderer::draw_box(const Transform3D &t, Color8 color) {
	_items.push_back(DirectMultiMeshInstance::TransformAndColor32{ t, Color(color) });
}

void DebugRenderer::end() {
	ERR_FAIL_COND(!_inside_block);
	_inside_block = false;

	DirectMultiMeshInstance::make_transform_and_color32_3d_bulk_array(to_span_const(_items), _bulk_array);
	if (_items.size() != static_cast<unsigned int>(_multimesh->get_instance_count())) {
		_multimesh->set_instance_count(_items.size());
	}

	// 显然 Godot 不喜欢空的批量数组，这会导致 RasterizerStorageGLES3 出错
	if (_items.size() > 0) {
		//_multimesh->set_as_bulk_array(_bulk_array);
		RenderingServer::get_singleton()->multimesh_set_buffer(_multimesh->get_rid(), _bulk_array);
	}

	_items.clear();
}

void DebugRenderer::clear() {
	_items.clear();
	// 若尚未调用 `init()` 则可能为空
	if (_multimesh.is_valid()) {
		_multimesh->set_instance_count(0);
	}
}

} // namespace voxel::godot
