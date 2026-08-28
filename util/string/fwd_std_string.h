#ifndef VOXEL_FWD_STD_STRING_H
#define VOXEL_FWD_STD_STRING_H

namespace voxel {

// std::string can't be forward-declared. Using type-tunneling instead.
// http://jonjagger.blogspot.com/2011/04/forward-declaring-stdstring-in-c.html
struct FwdConstStdString;
struct FwdMutableStdString;

} // namespace voxel

#endif // VOXEL_FWD_STD_STRING_H
