#ifndef VOXEL_ORTHO_BASIS_H
#define VOXEL_ORTHO_BASIS_H

#include "conv.h"
#include "vector3i.h"
#include "vector3t.h"

namespace voxel::math {

// 正交基适用于仅使用 90 度步进角度的三维旋转。由于只有 24 种
// 可能情况，它们通常可编码为单个字节。通过查表即可恢复为常规基。
// table.

static const int ORTHOGONAL_BASIS_COUNT = 24;
static const int ORTHOGONAL_BASIS_IDENTITY_INDEX = 0;

// 每个轴都是指向 -X、+X、-Y、+Y、-Z 或 +Z 的单位向量，且各轴彼此
// 垂直。
// 对此类基进行运算不会损失精度，且可安全使用相等比较。
struct OrthoBasis {
	// Axes
	Vector3i x;
	Vector3i y;
	Vector3i z;

	OrthoBasis() : x(1, 0, 0), y(0, 1, 0), z(0, 0, 1) {}
	OrthoBasis(Vector3i p_x, Vector3i p_y, Vector3i p_z) : x(p_x), y(p_y), z(p_z) {}

	static OrthoBasis from_axis_turns(const Vector3i::Axis axis, const int turns);

	bool is_orthonormal() const;

	Vector3i get_axis(const int i) const {
		// TODO 优化：可改用与数组的联合体
		switch (i) {
			case Vector3i::AXIS_X:
				return x;
			case Vector3i::AXIS_Y:
				return y;
			case Vector3i::AXIS_Z:
				return z;
			default:
				VOXEL_CRASH();
		}
		return Vector3i();
	}

	inline bool operator==(const OrthoBasis &other) const {
		return x == other.x && y == other.y && z == other.z;
	}

	inline void transpose() {
		// x A A
		// B x A
		// B B x
		// 我们只需沿对角线将 A 与 B 互换。
		SWAP(x.y, y.x);
		SWAP(x.z, z.x);
		SWAP(y.z, z.y);
	}

	inline void invert() {
		// 正交矩阵的逆等于其转置。
		// https://math.stackexchange.com/questions/1936020/why-is-the-inverse-of-an-orthogonal-matrix-equal-to-its-transpose
		transpose();
	}

	inline OrthoBasis inverted() const {
		OrthoBasis b = *this;
		b.invert();
		return b;
	}

	inline Vector3i xform(const Vector3i p) const {
		return p.x * x + p.y * y + p.z * z;
	}

	inline math::OrthoBasis operator*(const math::OrthoBasis other) const {
		return math::OrthoBasis(xform(other.x), xform(other.y), xform(other.z));
	}

	inline void rotate_x_90_cw() {
		x = math::rotate_x_90_cw(x);
		y = math::rotate_x_90_cw(y);
		z = math::rotate_x_90_cw(z);
	}

	inline void rotate_x_90_ccw() {
		x = math::rotate_x_90_ccw(x);
		y = math::rotate_x_90_ccw(y);
		z = math::rotate_x_90_ccw(z);
	}

	inline void rotate_y_90_cw() {
		x = math::rotate_y_90_cw(x);
		y = math::rotate_y_90_cw(y);
		z = math::rotate_y_90_cw(z);
	}

	inline void rotate_y_90_ccw() {
		x = math::rotate_y_90_ccw(x);
		y = math::rotate_y_90_ccw(y);
		z = math::rotate_y_90_ccw(z);
	}

	inline void rotate_z_90_cw() {
		x = math::rotate_z_90_cw(x);
		y = math::rotate_z_90_cw(y);
		z = math::rotate_z_90_cw(z);
	}

	inline void rotate_z_90_ccw() {
		x = math::rotate_z_90_ccw(x);
		y = math::rotate_z_90_ccw(y);
		z = math::rotate_z_90_ccw(z);
	}

	void rotate_90(const Axis axis, const bool clockwise) {
		voxel::math::rotate_90(Span<Vector3i>(&x, 3), axis, clockwise);
	}
};

// 在不采用特定约定的情况下，每个旋转可用 3 个轴 XYZ 按如下形式命名：
// `+x+y+z`
// `-y+x+z`
// `-x-y+z`
// ...
//
// 可使用更直观的命名：将 -Z 视为前、Y 视为上、X 视为右，所有
// 旋转均为逆时针，并优先采用以下形式之一：
//
// identity
// y+A             (Y rotation)
// z+A             (roll around forward)
// x+90|270        (up or down)
// x+90|270, y+B   (up or down + Y rotation)
// z+A, y+B        (roll around forward + Y rotation)
//
// 注意：一半的旋转是绕 Z 轴滚转的。在 Minecraft 游戏中，这些旋转从未被使用。
//
enum OrthoRotationID {
	ORTHO_ROTATION_IDENTITY = 0,

	ORTHO_ROTATION_Z_270, // 1（滚转 270°）
	ORTHO_ROTATION_Z_180, // 2（滚转 180°）
	ORTHO_ROTATION_Z_90, // 3（滚转 90°）
	ORTHO_ROTATION_X_270, // 4（向下看）
	ORTHO_ROTATION_X_270_Y_270, // 5（向下看，向右转）
	ORTHO_ROTATION_X_270_Y_180, // 6（向下看，绕 Y 转 180°）
	ORTHO_ROTATION_X_270_Y_90, // 7（向下看，向左转）
	ORTHO_ROTATION_Z_180_Y_180, // 8（滚转 180°，绕 Y 转 180°）
	ORTHO_ROTATION_Z_90_Y_180, // 9（滚转 90°，绕 Y 转 180°）
	ORTHO_ROTATION_Y_180, // 10（绕 Y 转 180°）
	ORTHO_ROTATION_Z_270_Y_180, // 11（滚转 270°，绕 Y 转 180°）
	ORTHO_ROTATION_X_90, // 12（向上看）
	ORTHO_ROTATION_X_90_Y_90, // 13（向上看，向左转）
	ORTHO_ROTATION_X_90_Y_180, // 14（向上看，绕 Y 转 180°）
	ORTHO_ROTATION_X_90_Y_270, // 15（向上看，向右转）
	ORTHO_ROTATION_Y_270, // 16（向右转）
	ORTHO_ROTATION_Z_270_Y_270, // 17（滚转 270°，向右转）
	ORTHO_ROTATION_Z_180_Y_270, // 18（滚转 180°，向右转）
	ORTHO_ROTATION_Z_90_Y_270, // 19（滚转 90°，向右转）
	ORTHO_ROTATION_Z_180_Y_90, // 20（滚转 180°，向左转）
	ORTHO_ROTATION_Z_90_Y_90, // 21（滚转 90°，向左转）
	ORTHO_ROTATION_Y_90, // 22（向左转）
	ORTHO_ROTATION_Z_270_Y_90, // 23（滚转 270°，向左转）

	ORTHO_ROTATION_COUNT,

	// 备选方案
	//
	// IDENTITY (FORWARD)
	// FORWARD_ROLL_270
	// FORWARD_ROLL_180
	// FORWARD_ROLL_90
	// DOWN
	// DOWN_ROLL_270
	// DOWN_ROLL_180
	// DOWN_ROLL_90
	// BACK_ROLL_180
	// BACK_ROLL_90
	// BACK
	// BACK_ROLL_270
	// UP
	// UP_ROLL_90
	// UP_ROLL_180
	// UP_ROLL_270
	// RIGHT
	// RIGHT_ROLL_270
	// RIGHT_ROLL_180
	// RIGHT_ROLL_90
	// LEFT_ROLL_180
	// LEFT_ROLL_90
	// LEFT
	// LEFT_ROLL_270
};

OrthoBasis get_ortho_basis_from_index(int i);
int get_index_from_ortho_basis(const OrthoBasis &b);
const char *ortho_rotation_to_string(int i);

} // namespace voxel::math

#endif // VOXEL_ORTHO_BASIS_H
