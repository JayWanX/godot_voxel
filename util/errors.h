#ifndef VOXEL_ERRORS_H
#define VOXEL_ERRORS_H

#include "io/log.h"
#include "macros.h"

// 异常终止程序
#ifdef _MSC_VER
#define VOXEL_GENERATE_TRAP() __debugbreak()
#else
#define VOXEL_GENERATE_TRAP() __builtin_trap()
#endif

// 以下宏会打印一条消息并使程序崩溃。

#define VOXEL_CRASH_MSG(msg)                                                                                              \
	voxel::print_error("FATAL: Method/function failed.", msg, __FUNCTION__, __FILE__, __LINE__);                      \
	voxel::flush_stdout();                                                                                            \
	VOXEL_GENERATE_TRAP()

#define VOXEL_CRASH() VOXEL_CRASH_MSG("")

// 以下宏会检查一个条件。若失败，则打印消息并使程序崩溃。

#define VOXEL_ASSERT_MSG(cond, msg)                                                                                       \
	if (VOXEL_UNLIKELY(!(cond))) {                                                                                        \
		voxel::print_error(                                                                                           \
				"FATAL: Assertion failed: \"" #cond "\" is false.", msg, __FUNCTION__, __FILE__, __LINE__              \
		);                                                                                                             \
		voxel::flush_stdout();                                                                                        \
		VOXEL_GENERATE_TRAP();                                                                                            \
	} else                                                                                                             \
		((void)0)

#define VOXEL_ASSERT(cond) VOXEL_ASSERT_MSG(cond, "")

// 以下宏会检查一个条件。若失败，则打印消息，然后 return 或 continue。

#define VOXEL_INTERNAL_ASSERT_ACT(cond, act, msg)                                                                         \
	if (VOXEL_UNLIKELY(!(cond))) {                                                                                        \
		voxel::print_error("Assertion failed: \"" #cond "\" is false.", msg, __FUNCTION__, __FILE__, __LINE__);       \
		act;                                                                                                           \
	} else                                                                                                             \
		((void)0)

#define VOXEL_ASSERT_RETURN(cond) VOXEL_INTERNAL_ASSERT_ACT(cond, return, "")
#define VOXEL_ASSERT_RETURN_MSG(cond, msg) VOXEL_INTERNAL_ASSERT_ACT(cond, return, msg)
#define VOXEL_ASSERT_RETURN_V(cond, retval) VOXEL_INTERNAL_ASSERT_ACT(cond, return retval, "")
#define VOXEL_ASSERT_RETURN_V_MSG(cond, retval, msg) VOXEL_INTERNAL_ASSERT_ACT(cond, return retval, msg)
#define VOXEL_ASSERT_CONTINUE(cond) VOXEL_INTERNAL_ASSERT_ACT(cond, continue, "")
#define VOXEL_ASSERT_CONTINUE_MSG(cond, msg) VOXEL_INTERNAL_ASSERT_ACT(cond, continue, msg)

#endif // VOXEL_ERRORS_H
