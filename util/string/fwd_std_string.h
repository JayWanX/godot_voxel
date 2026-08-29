#ifndef VOXEL_FWD_STD_STRING_H
#define VOXEL_FWD_STD_STRING_H

namespace voxel {

// std::string 不能被前置声明。改用类型隧道（type-tunneling）技术。
// http://jonjagger.blogspot.com/2011/04/forward-declaring-stdstring-in-c.html
struct FwdConstStdString;
struct FwdMutableStdString;

} // namespace voxel

#endif // VOXEL_FWD_STD_STRING_H
