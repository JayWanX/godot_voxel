#ifndef VOXEL_DSTACK_H
#define VOXEL_DSTACK_H

#include "containers/std_vector.h"
#include "string/fwd_std_string.h"

#ifdef DEBUG_ENABLED
#define VOXEL_DSTACK_ENABLED
#endif

#ifdef VOXEL_DSTACK_ENABLED
// 将该宏放在你希望在调试堆栈跟踪中追踪的每个函数开头。
#define VOXEL_DSTACK() voxel::dstack::Scope dstack_scope_##__LINE__(__FILE__, __LINE__, __FUNCTION__)
#else
#define VOXEL_DSTACK()
#endif

namespace voxel {
namespace dstack {

void push(const char *file, unsigned int line, const char *fname);
void pop();

struct Scope {
	Scope(const char *file, unsigned int line, const char *function) {
		push(file, line, function);
	}
	~Scope() {
		pop();
	}
};

struct Frame {
	const char *file = nullptr;
	const char *function = nullptr;
	unsigned int line = 0;
};

struct Info {
public:
	// 构造一个到目前为止从 VOXEL_DSTACK() 调用收集到的当前堆栈的副本
	Info();
	void to_string(FwdMutableStdString s) const;

private:
	StdVector<Frame> _frames;
};

} // namespace dstack
} // namespace voxel

#endif // VOXEL_DSTACK_H
