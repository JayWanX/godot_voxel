#ifndef VOXEL_SHORT_LOCK
#define VOXEL_SHORT_LOCK

// #define VOXEL_SHORT_LOCK_IS_MUTEX

#ifdef VOXEL_SHORT_LOCK_IS_MUTEX
#include "mutex.h"
#else
#include "spin_lock.h"
#endif

namespace voxel {

// 一种类似互斥量的原语，预期只会被短时间锁定。
// 根据测试结果，它可以用自旋锁或互斥量实现。

#ifdef VOXEL_SHORT_LOCK_IS_MUTEX
typedef BinaryMutex ShortLock;
#else
typedef SpinLock ShortLock;
#endif

struct ShortLockScope {
	ShortLock &short_lock;
	ShortLockScope(ShortLock &p_sl) : short_lock(p_sl) {
		short_lock.lock();
	}
	~ShortLockScope() {
		short_lock.unlock();
	}
};

} // namespace voxel

#endif // VOXEL_SHORT_LOCK
