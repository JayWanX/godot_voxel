#ifndef VOXEL_DSTACK_H
#define VOXEL_DSTACK_H

#include "containers/std_vector.h"
#include "string/fwd_std_string.h"

#ifdef DEBUG_ENABLED
#define VOXEL_DSTACK_ENABLED
#endif

#ifdef VOXEL_DSTACK_ENABLED
// Put this macro on top of each function you want to track in debug stack traces.
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
	// Constructs a copy of the current stack gathered so far from VOXEL_DSTACK() calls
	Info();
	void to_string(FwdMutableStdString s) const;

private:
	StdVector<Frame> _frames;
};

} // namespace dstack
} // namespace voxel

#endif // VOXEL_DSTACK_H
