#ifndef VOXEL_NON_COPYABLE_H
#define VOXEL_NON_COPYABLE_H

namespace voxel {

class NonCopyable {
protected:
	NonCopyable() = default;
	~NonCopyable() = default;

	NonCopyable(NonCopyable const &) = delete;
	void operator=(NonCopyable const &x) = delete;
};

} // namespace voxel

#endif // VOXEL_NON_COPYABLE_H
