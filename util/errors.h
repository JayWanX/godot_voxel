#ifndef VOXEL_ERRORS_H
#define VOXEL_ERRORS_H

#include "io/log.h"
#include "macros.h"

// Abnormally terminate the program
#ifdef _MSC_VER
#define VOXEL_GENERATE_TRAP() __debugbreak()
#else
#define VOXEL_GENERATE_TRAP() __builtin_trap()
#endif

// The following macros print a message and crash the program.

#define VOXEL_CRASH_MSG(msg)                                                                                              \
	voxel::print_error("FATAL: Method/function failed.", msg, __FUNCTION__, __FILE__, __LINE__);                      \
	voxel::flush_stdout();                                                                                            \
	VOXEL_GENERATE_TRAP()

#define VOXEL_CRASH() VOXEL_CRASH_MSG("")

// The following macros check a condition. If it fails, they print a message and crash the program.

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

// The following macros check a condition. If it fails, they print a message, then return or continue.

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
