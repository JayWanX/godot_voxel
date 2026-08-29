#ifndef VOXEL_THREAD_H
#define VOXEL_THREAD_H

#include <cstdint>

namespace voxel {

struct ThreadImpl;

class Thread {
public:
	enum Priority { //
		PRIORITY_LOW,
		PRIORITY_NORMAL,
		PRIORITY_HIGH
	};

	typedef uint64_t ID;

	typedef void (*Callback)(void *p_userdata);

	Thread();
	~Thread();

	void start(Callback p_callback, void *p_userdata, Priority priority = PRIORITY_NORMAL);
	bool is_started() const;
	void wait_to_finish();

	// 获取原生支持的并发线程数的提示
	static unsigned int get_hardware_concurrency();

	// 针对当前线程
	static void set_name(const char *name);
	static void sleep_usec(uint32_t microseconds);

	// 获取当前线程的 ID
	static ID get_caller_id();

private:
	ThreadImpl *_impl = nullptr;
};

} // namespace voxel

#endif // VOXEL_THREAD_H
