#ifndef VOXEL_STD_STRINGSTREAM_H
#define VOXEL_STD_STRINGSTREAM_H

#include "../memory/std_allocator.h"
#include <iosfwd>

namespace voxel {

using StdStringStream = std::basic_stringstream<char, std::char_traits<char>, StdDefaultAllocator<char>>;

} // namespace voxel

#endif // VOXEL_STD_STRINGSTREAM_H
