#ifndef VOXEL_UP_MODE_H
#define VOXEL_UP_MODE_H

namespace voxel {

// 说明如何解释当前体积中的“向上”方向
enum UpMode : uint8_t {
	// 世界是一个平面，因此高度由 Y 坐标获得，向上始终朝向 +Y。
	UP_MODE_POSITIVE_Y,
	// 世界是一个球体（行星），因此高度由到原点 (0,0,0) 的距离获得，
	// 向上是从原点到当前位置的归一化向量。
	UP_MODE_SPHERE,
	// 共有多少种向上模式
	UP_MODE_COUNT
};

} // namespace voxel

#endif // VOXEL_UP_MODE_H
