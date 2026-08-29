#ifndef VOXEL_MUTEX_H
#define VOXEL_MUTEX_H

// #include "../profiling.h"
#include <mutex>

namespace voxel {

template <class StdMutexT>
class MutexImpl {
	mutable StdMutexT mutex;

public:
	inline void lock() const {
		// VOXEL_PROFILE_SCOPE();
		mutex.lock();
	}

	inline void unlock() const {
		mutex.unlock();
	}

	inline bool try_lock() const {
		return mutex.try_lock();
	}
};

template <class MutexT>
class MutexLock {
	const MutexT &mutex;

public:
	inline explicit MutexLock(const MutexT &p_mutex) : mutex(p_mutex) {
		mutex.lock();
	}

	inline ~MutexLock() {
		mutex.unlock();
	}
};

using Mutex = MutexImpl<std::recursive_mutex>; // 可重入，用于一般用途
using BinaryMutex = MutexImpl<std::mutex>; // 不可重入，使用时需谨慎

// 注意：Godot 使用了 `extern template` 与 `_ALWAYS_INLINE_` 编译器特定宏的组合，而非
// `inline`。在该设置下，若没有 `_ALWAYS_INLINE_`，GCC 在调试构建中不会内联方法，从而导致
// 链接时出现 `undefined reference` 错误。然而考虑到 GCC 在优化构建中确实会内联方法，我
// 不理解这种设置带来什么好处……因此我采用简单的 `inline`。
//
// 不要在这些模板被使用的每个文件中都实例化，只实例化一次
// extern template class MutexImpl<std::recursive_mutex>;
// extern template class MutexImpl<std::mutex>;
// extern template class MutexLock<MutexImpl<std::recursive_mutex>>;
// extern template class MutexLock<MutexImpl<std::mutex>>;

} // namespace voxel

#endif // VOXEL_MUTEX_H
