#ifndef VOXEL_RW_LOCK_H
#define VOXEL_RW_LOCK_H

#include <shared_mutex>

//#define VOXEL_PROFILE_RWLOCK
#ifdef VOXEL_PROFILE_RWLOCK
#include "../profiling.h"
#endif

namespace voxel {

class RWLock {
public:
	// 锁定读写锁，若已被其他线程以写方式锁定则阻塞。
	// 警告：同一线程不可锁定两次，否则属于未定义行为。
	void read_lock() const {
#ifdef VOXEL_PROFILE_RWLOCK
		VOXEL_PROFILE_SCOPE();
#endif
		_mutex.lock_shared();
	}

	// 解锁读写锁，让其他线程继续
	void read_unlock() const {
		_mutex.unlock_shared();
	}

	// 尝试锁定读写锁，成功返回 `true`，`false` 表示无法锁定。
	bool read_try_lock() const {
		return _mutex.try_lock_shared();
	}

	// 锁定读写锁，若已被他人锁定则阻塞
	void write_lock() {
#ifdef VOXEL_PROFILE_RWLOCK
		VOXEL_PROFILE_SCOPE();
#endif
		_mutex.lock();
	}

	// 解锁读写锁，让其他写入者继续
	void write_unlock() {
		_mutex.unlock();
	}

	// 尝试锁定读写锁，成功返回 `true`，`false` 表示无法锁定。
	bool write_try_lock() {
		return _mutex.try_lock();
	}

private:
	mutable std::shared_timed_mutex _mutex;
};

class RWLockRead {
public:
	RWLockRead(const RWLock &p_lock) : _lock(p_lock) {
		_lock.read_lock();
	}
	~RWLockRead() {
		_lock.read_unlock();
	}

private:
	const RWLock &_lock;
};

class RWLockWrite {
public:
	RWLockWrite(RWLock &p_lock) : _lock(p_lock) {
		_lock.write_lock();
	}
	~RWLockWrite() {
		_lock.write_unlock();
	}

private:
	RWLock &_lock;
};

} // namespace voxel

#endif // VOXEL_RW_LOCK_H
