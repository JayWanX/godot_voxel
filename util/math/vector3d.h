#ifndef VOXEL_VECTOR3D_H
#define VOXEL_VECTOR3D_H

#include "vector3t.h"

namespace voxel {

// 无论编译选项如何，均使用 64 位浮点数的三维向量
typedef Vector3T<double> Vector3d;

} // namespace voxel

#endif // VOXEL_VECTOR3D_H
