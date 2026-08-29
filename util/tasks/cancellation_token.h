#ifndef VOXEL_CANCELLATION_TOKEN_H
#define VOXEL_CANCELLATION_TOKEN_H

#include "../errors.h"
#include "../memory/memory.h"
#include <atomic>

namespace voxel {

// 在任务与其请求者之间共享的简单对象。允许请求者在任务
// 运行或完成前取消它。
class TaskCancellationToken {
public:
	// TODO 可以优化
	// - 指向原子引用计数的指针？
	// - 索引到一个（分页的）原子整数池中？

	static TaskCancellationToken create() {
		TaskCancellationToken token;
		token._cancelled = make_shared_instance<std::atomic_bool>(false);
		return token;
	}

	inline bool is_valid() const {
		return _cancelled != nullptr;
	}

	inline void cancel() {
#ifdef TOOLS_ENABLED
		VOXEL_ASSERT(_cancelled != nullptr);
#endif
		*_cancelled = true;
	}

	inline bool is_cancelled() const {
#ifdef TOOLS_ENABLED
		VOXEL_ASSERT(_cancelled != nullptr);
#endif
		return *_cancelled;
	}

private:
	std::shared_ptr<std::atomic_bool> _cancelled;
};

} // namespace voxel

#endif // VOXEL_CANCELLATION_TOKEN_H
