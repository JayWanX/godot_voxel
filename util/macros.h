#ifndef VOXEL_MACROS_H
#define VOXEL_MACROS_H

// Macros I couldn't put anywhere specific

// Tell the compiler to favour a certain branch of a condition.
// Until C++20 can be used with the [[likely]] and [[unlikely]] attributes.
#if defined(__GNUC__)
#define VOXEL_LIKELY(x) __builtin_expect(!!(x), 1)
#define VOXEL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define VOXEL_LIKELY(x) x
#define VOXEL_UNLIKELY(x) x
#endif

#define VOXEL_INTERNAL_CONCAT(x, y) x##y
// Helper to concatenate macro arguments if one of them is itself a macro like `__LINE__`,
// otherwise doing `x##y` directly would not expand the arguments that are a macro
// https://stackoverflow.com/questions/1597007/creating-c-macro-with-and-line-token-concatenation-with-positioning-macr
#define VOXEL_CONCAT(x, y) VOXEL_INTERNAL_CONCAT(x, y)

// Gets the name of a class as a C-string with static lifetime, and causes a compiling error if the class doesn't exist.
#define VOXEL_CLASS_NAME_C(klass)                                                                                         \
	[]() {                                                                                                             \
		static_assert(sizeof(klass) > 0);                                                                              \
		return #klass;                                                                                                 \
	}()

// Gets a method name as a C-string with static lifetime, and causes a compiling error if either the class or method
// doesn't exist.
#define VOXEL_METHOD_NAME_C(klass, method)                                                                                \
	[]() {                                                                                                             \
		static_assert(sizeof(&klass::method != nullptr));                                                              \
		return #method;                                                                                                \
	}()

#endif // VOXEL_MACROS_H
