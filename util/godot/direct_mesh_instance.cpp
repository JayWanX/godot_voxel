#include "direct_mesh_instance.h"
#include "../profiling.h"
#include "classes/material.h"
#include <core/version.h>
#include <scene/resources/3d/world_3d.h>

namespace voxel::godot {

DirectMeshInstance::DirectMeshInstance() {
	// 此处无内容。它只是 RenderingServer 的 RID 薄封装，
	// 在调用其中一个函数之前不会向 RenderingServer 发出任何调用。
}

DirectMeshInstance::DirectMeshInstance(DirectMeshInstance &&src) {
	_mesh_instance = src._mesh_instance;
	_mesh = src._mesh;

	src._mesh_instance = RID();
	src._mesh = Ref<Mesh>();
}

DirectMeshInstance::~DirectMeshInstance() {
	destroy();
}

bool DirectMeshInstance::is_valid() const {
	return _mesh_instance.is_valid();
}

void DirectMeshInstance::create() {
	ERR_FAIL_COND(_mesh_instance.is_valid());
	RenderingServer &vs = *RenderingServer::get_singleton();
	_mesh_instance = vs.instance_create();
	vs.instance_set_visible(_mesh_instance, true); // TODO 是否需要？
}

void DirectMeshInstance::destroy() {
	if (_mesh_instance.is_valid()) {
		VOXEL_PROFILE_SCOPE();
		RenderingServer &vs = *RenderingServer::get_singleton();
		free_rendering_server_rid(vs, _mesh_instance);
		_mesh_instance = RID();
	}
	_mesh.unref();
}

void DirectMeshInstance::set_world(World3D *world) {
	ERR_FAIL_COND(!_mesh_instance.is_valid());
	RenderingServer &vs = *RenderingServer::get_singleton();
	if (world != nullptr) {
		vs.instance_set_scenario(_mesh_instance, world->get_scenario());
	} else {
		vs.instance_set_scenario(_mesh_instance, RID());
	}
}

void DirectMeshInstance::set_transform(Transform3D world_transform) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(!_mesh_instance.is_valid());
	RenderingServer &vs = *RenderingServer::get_singleton();
	vs.instance_set_transform(_mesh_instance, world_transform);
}

void DirectMeshInstance::set_mesh(Ref<Mesh> mesh) {
	ERR_FAIL_COND(!_mesh_instance.is_valid());
	RenderingServer &vs = *RenderingServer::get_singleton();
	if (mesh.is_valid()) {
		if (_mesh != mesh) {
			vs.instance_set_base(_mesh_instance, mesh->get_rid());
		}
	} else {
		vs.instance_set_base(_mesh_instance, RID());
	}
	_mesh = mesh;
}

void DirectMeshInstance::set_material_override(Ref<Material> material) {
	ERR_FAIL_COND(!_mesh_instance.is_valid());
	RenderingServer &vs = *RenderingServer::get_singleton();
	if (material.is_valid()) {
		vs.instance_geometry_set_material_override(_mesh_instance, material->get_rid());
	} else {
		vs.instance_geometry_set_material_override(_mesh_instance, RID());
	}
}

void DirectMeshInstance::set_visible(bool visible) {
	ERR_FAIL_COND(!_mesh_instance.is_valid());
	RenderingServer &vs = *RenderingServer::get_singleton();
	vs.instance_set_visible(_mesh_instance, visible);
}

void DirectMeshInstance::set_cast_shadows_setting(RenderingServerEnums::ShadowCastingSetting mode) {
	ERR_FAIL_COND(!_mesh_instance.is_valid());
	RenderingServer &vs = *RenderingServer::get_singleton();
	vs.instance_geometry_set_cast_shadows_setting(_mesh_instance, mode);
}

void DirectMeshInstance::set_shader_instance_parameter(StringName key, Variant value) {
	ERR_FAIL_COND(!_mesh_instance.is_valid());
	RenderingServer &vs = *RenderingServer::get_singleton();
	vs.instance_geometry_set_shader_parameter(_mesh_instance, key, value);
}

Ref<Mesh> DirectMeshInstance::get_mesh() const {
	return _mesh;
}

const Mesh *DirectMeshInstance::get_mesh_ptr() const {
	return _mesh.ptr();
}

void DirectMeshInstance::set_gi_mode(GeometryInstance3D::GIMode mode) {
	set_geometry_instance_gi_mode(_mesh_instance, mode);
}

void DirectMeshInstance::set_render_layers_mask(int mask) {
	ERR_FAIL_COND(!_mesh_instance.is_valid());
	RenderingServer &vs = *RenderingServer::get_singleton();
	vs.instance_set_layer_mask(_mesh_instance, mask);
}

void DirectMeshInstance::set_interpolated(const bool enabled) {
	// 该功能在 Godot 4.4 中加入，后在 4.5 中移到了 SceneTree
	// 参见 https://github.com/godotengine/godot/pull/104269
}

void DirectMeshInstance::operator=(DirectMeshInstance &&src) {
	if (_mesh_instance == src._mesh_instance) {
		return;
	}

	destroy();

	_mesh_instance = src._mesh_instance;
	_mesh = src._mesh;

	src._mesh_instance = RID();
	src._mesh.unref();
}

} // namespace voxel::godot
