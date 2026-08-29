#ifndef VOXEL_SPATIAL_LOCK_3D_H
#define VOXEL_SPATIAL_LOCK_3D_H

#include "../containers/std_vector.h"
#include "../math/box_bounds_3i.h"
#include "mutex.h"
#include "semaphore.h"
#include "short_lock.h"
#include "thread.h"

#ifdef TOOLS_ENABLED
#define VOXEL_SPATIAL_LOCK_3D_CHECKS
#endif

namespace voxel {

// 对大型体素数据结构加锁可以用这个，而无需在每个区块或
// 每个八叉树节点上都放读写锁。这也显著减少了所需互斥量的数量（在某些具有
// 较低限制的平台上很重要；由于互斥量实现可能占用大量字节，它也减少了内存占用。）
//
// 某些方法允许 `try` 锁定区域。它们应被用于阻塞比
// 延迟操作更糟的场景。这类场景随后可推迟或取消其工作。若做不到，则它
// 必须等待，或在主线程之外运行以维持应用的响应性。
//
// 在执行任务前，不要尝试同时锁定多个包围盒。如果另一个线程这样做，
// 则根据发生的顺序不同，可能导致死锁。
class SpatialLock3D {
public:
	enum Mode { //
		MODE_READ = 0,
		MODE_WRITE = 1
	};

	struct Box {
		BoxBounds3i bounds;
		Mode mode;
#ifdef VOXEL_SPATIAL_LOCK_3D_CHECKS
		Thread::ID thread_id;
#endif
	};

	SpatialLock3D();

	~SpatialLock3D() {
		VOXEL_ASSERT_RETURN(_boxes.size() == 0);
	}

	bool try_lock_read(const BoxBounds3i &box) {
		_boxes_mutex.lock();
		if (can_lock_for_read(box)) {
			_boxes.push_back(
					Box{ box,
						 MODE_READ,
#ifdef VOXEL_SPATIAL_LOCK_3D_CHECKS
						 Thread::get_caller_id()
#endif
					}
			);
			_boxes_mutex.unlock();
			return true;
		} else {
			_boxes_mutex.unlock();
			return false;
		}
	}

	void lock_read(const BoxBounds3i &box) {
		while (try_lock_read(box) == false) {
			_semaphore.wait();
		}
	}

	inline void unlock_read(const BoxBounds3i &box) {
		unlock(box, MODE_READ);
	}

	bool try_lock_write(const BoxBounds3i &box) {
		_boxes_mutex.lock();
		if (can_lock_for_write(box)) {
			_boxes.push_back(
					Box{ box,
						 MODE_WRITE,
#ifdef VOXEL_SPATIAL_LOCK_3D_CHECKS
						 Thread::get_caller_id()
#endif
					}
			);
			_boxes_mutex.unlock();
			return true;
		} else {
			_boxes_mutex.unlock();
			return false;
		}
	}

	void lock_write(const BoxBounds3i &box) {
		while (try_lock_write(box) == false) {
			_semaphore.wait();
		}
	}

	inline void unlock_write(const BoxBounds3i &box) {
		unlock(box, MODE_WRITE);
	}

	inline int get_locked_boxes_count() const {
		ShortLockScope rlock(_boxes_mutex);
		return _boxes.size();
	}

	// 作用域辅助类

	struct Read {
		Read(SpatialLock3D &p_locker, const BoxBounds3i p_box) : locker(p_locker), box(p_box) {
			locker.lock_read(box);
		}
		~Read() {
			locker.unlock_read(box);
		}
		SpatialLock3D &locker;
		const BoxBounds3i box;
	};

	struct Write {
		Write(SpatialLock3D &p_locker, const BoxBounds3i p_box) : locker(p_locker), box(p_box) {
			locker.lock_write(box);
		}
		~Write() {
			locker.unlock_write(box);
		}
		SpatialLock3D &locker;
		const BoxBounds3i box;
	};

private:
	bool can_lock_for_read(const BoxBounds3i &box) {
#ifdef VOXEL_SPATIAL_LOCK_3D_CHECKS
		const Thread::ID thread_id = Thread::get_caller_id();
#endif

		for (unsigned int i = 0; i < _boxes.size(); ++i) {
			const Box &existing_box = _boxes[i];
#ifdef VOXEL_SPATIAL_LOCK_3D_CHECKS
			// 每个线程一次只能锁定一个包围盒，否则根据
			// 加锁的顺序不同可能发生死锁。例如：
			// - 线程 1 锁定 A
			// - 线程 2 锁定 B
			// - 线程 1 锁定 B，但因已被锁定而阻塞
			// - 线程 2 锁定 A，但因已被锁定而阻塞：
			// 这就是死锁。
			// 注意：若线程只加读锁则不成立，但如果我们从不写入，也就不会用锁了。
			// 注意：若线程改用 `try_lock` 也不成立！
			VOXEL_ASSERT_RETURN_V_MSG(
					existing_box.thread_id != thread_id, false, "Locking two areas from the same threads is not allowed"
			);
#endif
			if (existing_box.bounds.intersects(box) && existing_box.mode == MODE_WRITE) {
				return false;
				break;
			}
		}
		return true;
	}

	bool can_lock_for_write(const BoxBounds3i &box) {
#ifdef VOXEL_SPATIAL_LOCK_3D_CHECKS
		const Thread::ID thread_id = Thread::get_caller_id();
#endif

		for (unsigned int i = 0; i < _boxes.size(); ++i) {
			const Box &existing_box = _boxes[i];

#ifdef VOXEL_SPATIAL_LOCK_3D_CHECKS
			VOXEL_ASSERT_RETURN_V_MSG(
					existing_box.thread_id != thread_id, false, "Locking two areas from the same threads is not allowed"
			);
#endif
			if (existing_box.bounds.intersects(box)) {
				return false;
				break;
			}
		}
		return true;
	}

	void remove_box(const BoxBounds3i &box, Mode mode);

	void unlock(const BoxBounds3i &box, Mode mode) {
		_boxes_mutex.lock();
		remove_box(box, mode);
		_boxes_mutex.unlock();
		// 通知可能正在等待的线程，它们现在或许可以锁定自己的包围盒了。
		_semaphore.post();
	}

	// 当前已锁定包围盒的列表。
	// 实际上，每个线程一次最多锁定 1 个包围盒（极少数允许的情况下可能稍多），因此
	// 需要存储的包围盒不会很多。
	StdVector<Box> _boxes;
	// 该互斥量预期只被锁定极短时间，仅用于查找、添加或移除包围盒。
	// 因此我们即使在 `try_*` 方法中也会锁定它。长时间锁定的状态是包围盒本身。
	// 另外出于性能它并非可重入互斥量。一旦成功锁定，不要再重复锁定。
	mutable ShortLock _boxes_mutex;
	// 锁失败时线程会等待该信号量。每次有包围盒解锁时它都会被 post，因此任何线程
	// 等待它时都可重试锁定自己的包围盒。
	Semaphore _semaphore;
};

} // namespace voxel

#endif // VOXEL_SPATIAL_LOCK_3D_H
