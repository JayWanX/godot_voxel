#ifndef VOXEL_MACROS_H
#define VOXEL_MACROS_H

// 我无法放进其他特定位置的宏

// 告诉编译器倾向于某个条件分支。
// 在 C++20 之前，可用 [[likely]] 与 [[unlikely]] 属性实现。
#if defined(__GNUC__)
#define VOXEL_LIKELY(x) __builtin_expect(!!(x), 1)
#define VOXEL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define VOXEL_LIKELY(x) x
#define VOXEL_UNLIKELY(x) x
#endif

#define VOXEL_INTERNAL_CONCAT(x, y) x##y
// 若某个宏参数本身也是宏（如 `__LINE__`），用于拼接宏参数的辅助工具，
// 否则直接写 `x##y` 不会展开作为宏的参数
// https://stackoverflow.com/questions/1597007/creating-c-macro-with-and-line-token-concatenation-with-positioning-macr
#define VOXEL_CONCAT(x, y) VOXEL_INTERNAL_CONCAT(x, y)

// 以具有静态生命周期的 C 字符串形式获取类名，若该类不存在则导致编译错误。
#define VOXEL_CLASS_NAME_C(klass)                                                                                         \
	[]() {                                                                                                             \
		static_assert(sizeof(klass) > 0);                                                                              \
		return #klass;                                                                                                 \
	}()

// 以具有静态生命周期的 C 字符串形式获取方法名，若类或方法
// 不存在则导致编译错误。
#define VOXEL_METHOD_NAME_C(klass, method)                                                                                \
	[]() {                                                                                                             \
		static_assert(sizeof(&klass::method != nullptr));                                                              \
		return #method;                                                                                                \
	}()

#endif // VOXEL_MACROS_H
