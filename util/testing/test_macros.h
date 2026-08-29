#ifndef VOXEL_TEST_MACROS_H
#define VOXEL_TEST_MACROS_H

#include "../errors.h"

// TODO 这些宏实际上应该先让测试返回，然后再失败。这是为了允许测试目录等东西
// 能够被清理

#define VOXEL_TEST_ASSERT(m_cond)                                                                                         \
	if ((m_cond) == false) {                                                                                           \
		voxel::print_error("FATAL: Condition \"" #m_cond "\" is false.", __FUNCTION__, __FILE__, __LINE__);           \
		VOXEL_GENERATE_TRAP();                                                                                            \
	}

#define VOXEL_TEST_ASSERT_V(m_cond, m_retval)                                                                             \
	if ((m_cond) == false) {                                                                                           \
		voxel::print_error("FATAL: Condition \"" #m_cond "\" is false.", __FUNCTION__, __FILE__, __LINE__);           \
		VOXEL_GENERATE_TRAP();                                                                                            \
	}

#define VOXEL_TEST_ASSERT_MSG(m_cond, m_msg)                                                                              \
	if ((m_cond) == false) {                                                                                           \
		voxel::print_error("FATAL: Condition \"" #m_cond "\" is false. " #m_msg, __FUNCTION__, __FILE__, __LINE__);   \
		VOXEL_GENERATE_TRAP();                                                                                            \
	}

#endif // VOXEL_TEST_MACROS_H
