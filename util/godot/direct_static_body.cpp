#include "direct_static_body.h"
#include "../profiling.h"
#include "classes/array_mesh.h"
#include "classes/physics_server_3d.h"
#include <core/version.h>
#include <scene/resources/3d/world_3d.h>

namespace voxel::godot {

DirectStaticBody::DirectStaticBody() {
	// 这里什么都没有。它只是一个轻量的 RID 封装，
	// 在调用某个函数之前不会对 PhysicsServer3D 发出任何调用。
}

DirectStaticBody::~DirectStaticBody() {
	destroy();
}

void DirectStaticBody::create() {
	ERR_FAIL_COND(_body.is_valid());
	PhysicsServer3D &ps = *PhysicsServer3D::get_singleton();
	_body = ps.body_create();
	ps.body_set_ray_pickable(_body, false);
	ps.body_set_mode(_body, PhysicsServer3DEnums::BODY_MODE_STATIC);
}

void DirectStaticBody::destroy() {
	if (_body.is_valid()) {
		PhysicsServer3D &ps = *PhysicsServer3D::get_singleton();
		free_physics_server_rid(ps, _body);
		_body = RID();
		// 形状需要在刚体销毁之后销毁
		_shape.unref();
	}
	if (_debug_mesh_instance.is_valid()) {
		_debug_mesh_instance.destroy();
	}
}

bool DirectStaticBody::is_valid() const {
	return _body.is_valid();
}

void DirectStaticBody::set_transform(Transform3D transform) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(!_body.is_valid());
	PhysicsServer3D::get_singleton()->body_set_state(_body, PhysicsServer3DEnums::BODY_STATE_TRANSFORM, transform);

	if (_debug_mesh_instance.is_valid()) {
		_debug_mesh_instance.set_transform(transform);
	}
}

void DirectStaticBody::add_shape(Ref<Shape3D> shape) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(!_body.is_valid());
	PhysicsServer3D::get_singleton()->body_add_shape(_body, shape->get_rid(), Transform3D(), false);
	// 目前还没有多形状的使用场景
	_shape = shape;

	if (_debug_mesh_instance.is_valid()) {
		Ref<Mesh> mesh = _shape->get_debug_mesh();
		_debug_mesh_instance.set_mesh(mesh);
	}
}

void DirectStaticBody::remove_shape(int shape_index) {
	ERR_FAIL_COND(!_body.is_valid());
	PhysicsServer3D::get_singleton()->body_remove_shape(_body, shape_index);
	_shape.unref();

	if (_debug_mesh_instance.is_valid()) {
		_debug_mesh_instance.set_mesh(Ref<Mesh>());
	}
}

Ref<Shape3D> DirectStaticBody::get_shape(int shape_index) const {
	ERR_FAIL_COND_V(shape_index < 0 || shape_index > 1, Ref<Shape3D>());
	return _shape;
}

void DirectStaticBody::set_world(World3D *world) {
	ERR_FAIL_COND(!_body.is_valid());
	PhysicsServer3D &ps = *PhysicsServer3D::get_singleton();
	ps.body_set_space(_body, world != nullptr ? world->get_space() : RID());

	if (_debug_mesh_instance.is_valid()) {
		_debug_mesh_instance.set_world(world);
	}
}

void DirectStaticBody::set_shape_enabled(int shape_index, bool enabled) {
	ERR_FAIL_COND(!_body.is_valid());
	PhysicsServer3D &ps = *PhysicsServer3D::get_singleton();
	ps.body_set_shape_disabled(_body, shape_index, !enabled);

	if (_debug_mesh_instance.is_valid()) {
		_debug_mesh_instance.set_visible(enabled);
	}
}

void DirectStaticBody::set_attached_object(const Object *obj) {
	// 用于高层级碰撞查询结果，`collider` 将包含附加的对象
	ERR_FAIL_COND(!_body.is_valid());
	PhysicsServer3D::get_singleton()->body_attach_object_instance_id(
			_body, obj != nullptr ? obj->get_instance_id() : ObjectID()
	);
}

void DirectStaticBody::set_collision_layer(int layer) {
	ERR_FAIL_COND(!_body.is_valid());
	PhysicsServer3D::get_singleton()->body_set_collision_layer(_body, layer);
}

void DirectStaticBody::set_collision_mask(int mask) {
	ERR_FAIL_COND(!_body.is_valid());
	PhysicsServer3D::get_singleton()->body_set_collision_mask(_body, mask);
}

void DirectStaticBody::set_debug(bool enabled, World3D *world) {
	ERR_FAIL_COND(world == nullptr);

	if (enabled && !_debug_mesh_instance.is_valid()) {
		_debug_mesh_instance.create();
		_debug_mesh_instance.set_interpolated(false);
		_debug_mesh_instance.set_world(world);

		const Transform3D transform =
				PhysicsServer3D::get_singleton()->body_get_state(_body, PhysicsServer3DEnums::BODY_STATE_TRANSFORM);
		_debug_mesh_instance.set_transform(transform);

		if (_shape.is_valid()) {
			Ref<Mesh> mesh = _shape->get_debug_mesh();
			_debug_mesh_instance.set_mesh(mesh);
		}

	} else if (!enabled && _debug_mesh_instance.is_valid()) {
		_debug_mesh_instance.destroy();
	}
}

} // namespace voxel::godot