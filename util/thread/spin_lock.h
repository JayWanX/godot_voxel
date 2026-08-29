#ifndef VOXEL_SPIN_LOCK_H
#define VOXEL_SPIN_LOCK_H

#include <atomic>

namespace voxel {

class SpinLock {
public:
	inline void lock() {
		while (_locked.test_and_set(std::memory_order_acquire)) {
			; // 继续。
			// 注意：以后可研究使用内建指令让出 CPU 是否能提升性能？
			// https://rigtorp.se/spinlock/
			// 另外我们以后也可以实现 RWSpinLock
		}
	}

	inline void unlock() {
		_locked.clear(std::memory_order_release);
	}

private:
	std::atomic_flag _locked = ATOMIC_FLAG_INIT;
};

} // namespace voxel

#endif // VOXEL_SPIN_LOCK_H
