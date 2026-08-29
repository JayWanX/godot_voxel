#ifndef VOXEL_GODOT_OBJECT_H
#define VOXEL_GODOT_OBJECT_H

#if defined(VOXEL_GODOT)
#include <core/object/object.h>
// `GDCLASS` 宏在继承 `Object` 时无法编译，除非同时包含 `class_db.h`
#include <core/object/class_db.h>
#endif

#include "../../containers/std_vector.h"


namespace voxel::godot {

// 从给定对象的属性获取哈希。如果属性本身也是对象，则递归
// 解析。注意，限制为可编辑属性很重要，可以避免像纹理或网格这类带有对象的昂贵属性。
uint64_t get_deep_hash(
		const Object &obj,
		uint32_t property_usage = PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_EDITOR,
		uint64_t hash = 0
);

// 属性信息的统一表示。
struct PropertyInfoWrapper {
	Variant::Type type;
	String name;
	uint32_t usage;
};
void get_property_list(const Object &obj, StdVector<PropertyInfoWrapper> &out_properties);

// 事实证明这些函数目前只在编辑器中使用。
// 它们是通用的，但我必须包装它们，否则 GCC 会把"未使用"当作错误警告抛出。
#ifdef TOOLS_ENABLED

void set_object_edited(Object &obj);

#endif

} // namespace voxel::godot

#endif // VOXEL_GODOT_OBJECT_H
