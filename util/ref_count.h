#ifndef VOXEL_REF_COUNT_H
#define VOXEL_REF_COUNT_H

#include "errors.h"

namespace voxel {

// Simple reference counter.
// This one is not thread-safe.
class RefCount {
public:
	RefCount() {}
	RefCount(unsigned int initial_count) : _count(initial_count) {}

	inline void add() {
		++_count;
	}

	inline void remove() {
		VOXEL_ASSERT_RETURN_MSG(_count != 0, "Trying to decrease refcount when it's already zero");
		--_count;
	}

	inline unsigned int get() const {
		return _count;
	}

private:
	unsigned int _count = 0;
};

} // namespace voxel

#endif // VOXEL_REF_COUNT_H
