#ifndef VOXEL_GODOT_OBJECT_WEAK_REF_H
#define VOXEL_GODOT_OBJECT_WEAK_REF_H

#include "classes/object.h"

namespace voxel::godot {

// 持有对 Godot 对象的弱引用。
// 主要用于更安全地引用场景树节点，因为与 RefCounted 对象相比，节点的所有权模型更难用指针处理。
// 不适用于 RefCounted 对象。
// 警告：如果对象可能被其他线程销毁，那么这样使用并不安全。
template <typename T>
class ObjectWeakRef {
public:
	void set(T *obj) {
		_id = obj != nullptr ? obj->get_instance_id() : ObjectID();
	}

	T *get() const {
		if (!_id.is_valid()) {
			return nullptr;
		}
		Object *obj = ObjectDB::get_instance(_id);
		if (obj == nullptr) {
			// 可能已被销毁。
			// _node_object_id = ObjectID();
			return nullptr;
		}
		T *tobj = Object::cast_to<T>(obj);
		// 我们不希望 Godot 为不同对象复用同一个 ObjectID
		ERR_FAIL_COND_V(tobj == nullptr, nullptr);
		return tobj;
	}

private:
	ObjectID _id;
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_OBJECT_WEAK_REF_H
