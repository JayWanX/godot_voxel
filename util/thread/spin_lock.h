#ifndef VOXEL_SPIN_LOCK_H
#define VOXEL_SPIN_LOCK_H

#include <atomic>

namespace voxel {

class SpinLock {
public:
	inline void lock() {
		while (_locked.test_and_set(std::memory_order_acquire)) {
			; // Continue.
			// Note: eventually investigate if yielding with intrinsics improves performance?
			// https://rigtorp.se/spinlock/
			// Also we could eventually implement RWSpinLock
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
