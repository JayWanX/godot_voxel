#ifndef VOXEL_FILE_LOCKER_H
#define VOXEL_FILE_LOCKER_H

#include "../containers/std_unordered_map.h"
#include "../errors.h"
#include "../string/std_string.h"
#include "../thread/mutex.h"
#include "../thread/rw_lock.h"

namespace voxel {

// 对路径执行软件层面的加锁，
// 这样（由本模块控制的）多个线程想要访问同一个文件时会锁定一个共享互斥量。
class FileLocker {
public:
	void lock_read(const StdString &fpath) {
		lock(fpath, true);
	}

	void lock_write(const StdString &fpath) {
		lock(fpath, false);
	}

	void unlock(const StdString &fpath) {
		unlock_internal(fpath);
	}

private:
	struct File {
		RWLock lock;
		bool read_only;
	};

	void lock(const StdString &fpath, bool read_only) {
		File *fp = nullptr;
		{
			MutexLock lock(_files_mutex);
			// 获取或创建。
			// 注意，我们永远不会从 map 中移除条目
			fp = &_files[fpath];
		}

		if (read_only) {
			fp->lock.read_lock();
			// 已获取读锁。意味着没有人在写入。
			fp->read_only = true;

		} else {
			fp->lock.write_lock();
			// 已获取写锁。意味着只有一个线程在写入。
			fp->read_only = false;
		}
	}

	void unlock_internal(const StdString &fpath) {
		File *fp = nullptr;
		{
			MutexLock lock(_files_mutex);
			auto it = _files.find(fpath);
			if (it != _files.end()) {
				fp = &it->second;
			}
		}
		VOXEL_ASSERT_RETURN(fp != nullptr);
		// TODO 可能已经调用过 FileAccess::reopen，这会使我强制执行线程同步的努力白费 :|
		// 所以目前请不要那样做

		if (fp->read_only) {
			fp->lock.read_unlock();
		} else {
			fp->lock.write_unlock();
		}
	}

private:
	Mutex _files_mutex;
	StdUnorderedMap<StdString, File> _files;
};

} // namespace voxel

#endif // VOXEL_FILE_LOCKER_H
