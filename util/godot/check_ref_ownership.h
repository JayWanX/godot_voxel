#define VOXEL_GODOT_CHECK_REF_COUNT_DOES_NOT_CHANGE(m_ref)                                                                \
	VOXEL_ASSERT(m_ref.is_valid());                                                                                       \
	voxel::godot::CheckRefCountDoesNotChange VOXEL_CONCAT(ref_count_checker_, __LINE__)(__FUNCTION__, m_ref.ptr())
#ifndef VOXEL_GODOT_CHECK_REF_OWNERSHIP_H
#define VOXEL_GODOT_CHECK_REF_OWNERSHIP_H

#ifdef TOOLS_ENABLED
#include "../errors.h"
#include "../macros.h"
#include "../string/format.h"
#include "classes/ref_counted.h"
#include "core/string.h"

namespace voxel::godot {

// 检查在作用域开始到结束之间，没有任何东西额外持有某个 RefCounted 对象的引用。
// 当调用 GDVIRTUAL 方法并传入一个对象、且该对象在调用结束后不能被调用方持有时，可使用此检查。
// 即在调用结束时该对象不应被调用方持有。
class CheckRefCountDoesNotChange {
public:
	// 理想情况下不应需要在项目设置中关闭此检查，但像 C# 这样的语言由于垃圾回收内存模型会保留
	// 对对象的引用。在这种情况下，这项健全性检查的策略就会失效，
	// 因为我们无从判断发生了什么……
	static void set_enabled(bool enabled);
	static bool is_enabled();

	// 注意：为方便起见不接收 `const Ref<RefCounted>&`，因为这可能涉及类型转换，意味着 C++ 会
	// 按值而非按引用传递 Ref<T>，从而增加引用计数。
	inline CheckRefCountDoesNotChange(const char *method_name, RefCounted *rc) :
			_method_name(method_name), _rc(rc), _initial_count(rc->get_reference_count()) {}

	inline ~CheckRefCountDoesNotChange() {
		if (!is_enabled()) {
			return;
		}
		const int after_count = _rc->get_reference_count();
		if (after_count != _initial_count && !was_reported()) {
			mark_reported();
			VOXEL_PRINT_ERROR(
					format("Holding a reference to the passed {} outside {} is not allowed (count before: {}, "
						   "count after: {}). If you are using a garbage-collected language (like C#), "
						   "you may want to turn off this check in ProjectSettings.",
						   _rc->get_class(),
						   _method_name,
						   _initial_count,
						   after_count)
			);
		}
	}

private:
	static bool was_reported();
	static void mark_reported();

	const char *_method_name;
	const RefCounted *_rc;
	const int _initial_count;
};

} // namespace voxel::godot

#define VOXEL_GODOT_CHECK_REF_COUNT_DOES_NOT_CHANGE(m_ref)                                                                \
	VOXEL_ASSERT(m_ref.is_valid());                                                                                       \
	voxel::godot::CheckRefCountDoesNotChange VOXEL_CONCAT(ref_count_checker_, __LINE__)(__FUNCTION__, m_ref.ptr())

#else // TOOLS_ENABLED

#define VOXEL_GODOT_CHECK_REF_COUNT_DOES_NOT_CHANGE(m_ref)

#endif // TOOLS_ENABLED

#endif // VOXEL_GODOT_CHECK_REF_OWNERSHIP_H
