#include "ortho_basis.h"

namespace voxel::math {

// 我们可以把原点放在立方体的 8 个角上得到所有基的列表。在某个角上，基的
// 各轴可能与立方体的 3 条棱对齐。每个角可将对应棱旋转 3 次（X->Y、Y->Z、Z->X）。
// 因此共有 3 * 8 = 24 个基。
// 另一种方法是选一条轴，然后将基旋转使该轴分别指向 6 个
// 方向（需约定上/下），再绕该轴将基旋转 4 次，
// 这样得到 4 * 6 = 24 个基。

// 数值取自 Godot 的 GridMap 代码。顺序任意，但必须保持不变以匹配枚举值。
// clang-format off
static const OrthoBasis g_ortho_bases[ORTHOGONAL_BASIS_COUNT] = {
	OrthoBasis(Vector3i( 1,  0,  0), Vector3i( 0,  1,  0), Vector3i( 0,  0,  1)), // 恒等
	OrthoBasis(Vector3i( 0, -1,  0), Vector3i( 1,  0,  0), Vector3i( 0,  0,  1)), //
	OrthoBasis(Vector3i(-1,  0,  0), Vector3i( 0, -1,  0), Vector3i( 0,  0,  1)), //
	OrthoBasis(Vector3i( 0,  1,  0), Vector3i(-1,  0,  0), Vector3i( 0,  0,  1)), //

	OrthoBasis(Vector3i( 1,  0,  0), Vector3i( 0,  0, -1), Vector3i( 0,  1,  0)), //
	OrthoBasis(Vector3i( 0,  0,  1), Vector3i( 1,  0,  0), Vector3i( 0,  1,  0)), //
	OrthoBasis(Vector3i(-1,  0,  0), Vector3i( 0,  0,  1), Vector3i( 0,  1,  0)), //
	OrthoBasis(Vector3i( 0,  0, -1), Vector3i(-1,  0,  0), Vector3i( 0,  1,  0)), //

	OrthoBasis(Vector3i( 1,  0,  0), Vector3i( 0, -1,  0), Vector3i( 0,  0, -1)), //
	OrthoBasis(Vector3i( 0,  1,  0), Vector3i( 1,  0,  0), Vector3i( 0,  0, -1)), //
	OrthoBasis(Vector3i(-1,  0,  0), Vector3i( 0,  1,  0), Vector3i( 0,  0, -1)), //
	OrthoBasis(Vector3i( 0, -1,  0), Vector3i(-1,  0,  0), Vector3i( 0,  0, -1)), //

	OrthoBasis(Vector3i( 1,  0,  0), Vector3i( 0,  0,  1), Vector3i( 0, -1,  0)), //
	OrthoBasis(Vector3i( 0,  0, -1), Vector3i( 1,  0,  0), Vector3i( 0, -1,  0)), //
	OrthoBasis(Vector3i(-1,  0,  0), Vector3i( 0,  0, -1), Vector3i( 0, -1,  0)), //
	OrthoBasis(Vector3i( 0,  0,  1), Vector3i(-1,  0,  0), Vector3i( 0, -1,  0)), //

	OrthoBasis(Vector3i( 0,  0,  1), Vector3i( 0,  1,  0), Vector3i(-1,  0,  0)), //
	OrthoBasis(Vector3i( 0, -1,  0), Vector3i( 0,  0,  1), Vector3i(-1,  0,  0)), //
	OrthoBasis(Vector3i( 0,  0, -1), Vector3i( 0, -1,  0), Vector3i(-1,  0,  0)), //
	OrthoBasis(Vector3i( 0,  1,  0), Vector3i( 0,  0, -1), Vector3i(-1,  0,  0)), //

	OrthoBasis(Vector3i( 0,  0,  1), Vector3i( 0, -1,  0), Vector3i( 1,  0,  0)), //
	OrthoBasis(Vector3i( 0,  1,  0), Vector3i( 0,  0,  1), Vector3i( 1,  0,  0)), //
	OrthoBasis(Vector3i( 0,  0, -1), Vector3i( 0,  1,  0), Vector3i( 1,  0,  0)), //
	OrthoBasis(Vector3i( 0, -1,  0), Vector3i( 0,  0, -1), Vector3i( 1,  0,  0)) //
};
// clang-format on

OrthoBasis get_ortho_basis_from_index(int i) {
	VOXEL_ASSERT(i >= 0 && i < ORTHOGONAL_BASIS_COUNT);
	return g_ortho_bases[i];
}

int get_index_from_ortho_basis(const OrthoBasis &b) {
	for (int i = 0; i < ORTHOGONAL_BASIS_COUNT; ++i) {
		const OrthoBasis &ob = g_ortho_bases[i];
		if (ob == b) {
			return i;
		}
	}
	return -1;
}

// clang-format off
const char *s_rotation_names[ORTHO_ROTATION_COUNT] = {
	"identity",
	"z_270",
	"z_180",
	"z_90",
	"x_270",
	"x_270_y_270",
	"x_270_y_180",
	"x_270_y_90",
	"z_180_y_180",
	"z_90_y_180",
	"y_180",
	"z_270_y_180",
	"x_90",
	"x_90_y_90",
	"x_90_y_180",
	"x_90_y_270",
	"y_270",
	"z_270_y_270",
	"z_180_y_270",
	"z_90_y_270",
	"z_180_y_90",
	"z_90_y_90",
	"y_90",
	"z_270_y_90",
};
// clang-format on

const char *ortho_rotation_to_string(int i) {
	VOXEL_ASSERT_RETURN_V(i >= 0 && i < ORTHO_ROTATION_COUNT, "<error>");
	return s_rotation_names[i];
}

OrthoBasis OrthoBasis::from_axis_turns(const Vector3i::Axis axis, const int turns) {
	// 若旋转次数为负，则执行等价的正数旋转
	const int mturns = turns >= 0 ? turns % 4 : 4 - ((-turns) % 4);
	if (mturns == 0) {
		return OrthoBasis();
	}
	// 旋转轴指向我们时的顺时针方向
	switch (axis) {
		case Vector3i::AXIS_X:
			switch (mturns) {
				case 1:
					return { Vector3i(1, 0, 0), Vector3i(0, 0, -1), Vector3i(0, 1, 0) };
				case 2:
					return { Vector3i(1, 0, 0), Vector3i(0, -1, 0), Vector3i(0, 0, -1) };
				case 3:
					return { Vector3i(1, 0, 0), Vector3i(0, 0, 1), Vector3i(0, -1, 0) };
				default:
					break;
			}
			break;
		case Vector3i::AXIS_Y:
			switch (mturns) {
				case 1:
					return { Vector3i(0, 0, 1), Vector3i(0, 1, 0), Vector3i(-1, 0, 0) };
				case 2:
					return { Vector3i(-1, 0, 0), Vector3i(0, 1, 0), Vector3i(0, 0, -1) };
				case 3:
					return { Vector3i(0, 0, -1), Vector3i(0, 1, 0), Vector3i(1, 0, 0) };
				default:
					break;
			}
			break;
		case Vector3i::AXIS_Z:
			switch (mturns) {
				case 1:
					return { Vector3i(0, -1, 0), Vector3i(1, 0, 0), Vector3i(0, 0, 1) };
				case 2:
					return { Vector3i(-1, 0, 0), Vector3i(0, -1, 0), Vector3i(0, 0, 1) };
				case 3:
					return { Vector3i(0, 1, 0), Vector3i(-1, 0, 0), Vector3i(0, 0, 1) };
				default:
					break;
			}
			break;
		default:
			VOXEL_PRINT_ERROR("Invalid axis");
	}
	return OrthoBasis();
}

bool OrthoBasis::is_orthonormal() const {
	return Vector3iUtil::is_unit_vector(x) && //
			Vector3iUtil::is_unit_vector(y) && //
			Vector3iUtil::is_unit_vector(z) && //
			math::dot(x, y) == 0 && //
			math::dot(x, z) == 0 && //
			math::dot(y, z) == 0;
}

} // namespace voxel::math
