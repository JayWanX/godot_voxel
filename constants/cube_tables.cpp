#include "cube_tables.h"

namespace voxel::Cube {

// 以下表格遵循以下约定
//
//    7-------6
//   /|      /|
//  / |     / |  角点
// 4-------5  |
// |  3----|--2
// | /     | /     y z
// |/      |/      |/   OpenGL 坐标轴约定
// 0-------1    x--o
//
//
//     o---10----o
//    /|        /|
//  11 7       9 6   边
//  /  |      /  |
// o----8----o   |
// |   o---2-|---o
// 4  /      5  /
// | 3       | 1
// |/        |/
// o----0----o
//
// 面按 Voxel::Side 枚举顺序排列。
// 边按 Voxel::Edge 枚举顺序排列（仅 g_edge_inormals！）。
//

// clang-format off

// 按照立方体角点示意图的顺序排列
const Vector3f g_corner_position[CORNER_COUNT] = {
	Vector3f(1, 0, 0), //
	Vector3f(0, 0, 0), //
	Vector3f(0, 0, 1), //
	Vector3f(1, 0, 1), //
	Vector3f(1, 1, 0), //
	Vector3f(0, 1, 0), //
	Vector3f(0, 1, 1), //
	Vector3f(1, 1, 1)
};

// 3---2
// |   |
// 0---1
const int g_side_quad_triangles[SIDE_COUNT][6] = {
	{ 0, 2, 1, 0, 3, 2 }, // 左 (+x)
	{ 0, 2, 1, 0, 3, 2 }, // 右 (-x)
	{ 0, 2, 1, 0, 3, 2 }, // 底 (-y)
	{ 0, 2, 1, 0, 3, 2 }, // 顶 (+y)
	{ 0, 2, 1, 0, 3, 2 }, // 后 (-z)
	{ 0, 2, 1, 0, 3, 2 }, // 前 (+z)
};

// const int g_side_quad_triangles_alt[6] = { 0, 3, 1, 1, 3, 2 };

// const unsigned int g_side_coord[SIDE_COUNT] = { 0, 0, 1, 1, 2, 2 };
// const unsigned int g_side_sign[SIDE_COUNT] = { 0, 1, 0, 1, 0, 1 };

const Vector3i g_side_normals[SIDE_COUNT] = {
	// TODO 缺陷：错误！LEFT 应为 -X
	Vector3i(1, 0, 0), // LEFT
	Vector3i(-1, 0, 0), // RIGHT

	Vector3i(0, -1, 0), // BOTTOM
	Vector3i(0, 1, 0), // TOP

	// TODO 缺陷：错误！FRONT 应为 -Z
	Vector3i(0, 0, -1), // BACK
	Vector3i(0, 0, 1), // FRONT
};

const float g_side_tangents[SIDE_COUNT][4] = { //
	{ 0.f, 0.f, -1.f, 1.f }, //
	{ 0.f, 0.f, 1.f, 1.f }, //

	{ 1.f, 0.f, 0.f, 1.f }, //
	{ -1.f, 0.f, 0.f, 1.f }, //

	{ -1.f, 0.f, 0.f, 1.f }, //
	{ 1.f, 0.f, 0.f, 1.f }
};

// 角点的环绕顺序相同，相对于面的法线方向。
// X 和 Z 面最上面的角点排在最后。
const unsigned int g_side_corners[SIDE_COUNT][4] = {
	{ 3, 0, 4, 7 }, //
	{ 1, 2, 6, 5 }, //
	{ 1, 0, 3, 2 }, //
	{ 4, 5, 6, 7 }, //
	{ 0, 1, 5, 4 }, //
	{ 2, 3, 7, 6 } //
};

const unsigned int g_side_edges[SIDE_COUNT][4] = { //
	{ 3, 7, 11, 4 }, //
	{ 1, 6, 9, 5 }, //
	{ 0, 1, 2, 3 }, //
	{ 8, 9, 10, 11 }, //
	{ 0, 5, 8, 4 }, //
	{ 2, 6, 10, 7 }
};

// 3---2
// | / | {0,1,2,0,2,3}
// 0---1
// static const unsigned int g_vertex_to_corner[Voxel::SIDE_COUNT][6] = {
//    { 0, 3, 7, 0, 7, 4 },
//    { 2, 1, 5, 2, 5, 6 },
//    { 0, 1, 2, 0, 2, 3 },
//    { 7, 6, 5, 7, 5, 4 },
//    { 1, 0, 4 ,1, 4, 5 },
//    { 3, 2, 6, 3, 6, 7 }
//};

const Vector3i g_corner_inormals[CORNER_COUNT] = {
	Vector3i(1, -1, -1), //
	Vector3i(-1, -1, -1), //
	Vector3i(-1, -1, 1), //
	Vector3i(1, -1, 1), //

	Vector3i(1, 1, -1), //
	Vector3i(-1, 1, -1), //
	Vector3i(-1, 1, 1), //
	Vector3i(1, 1, 1) //
};

const Vector3i g_edge_inormals[EDGE_COUNT] = {
	Vector3i(0, -1, -1), //
	Vector3i(-1, -1, 0), //
	Vector3i(0, -1, 1), //
	Vector3i(1, -1, 0), //

	Vector3i(1, 0, -1), //
	Vector3i(-1, 0, -1), //
	Vector3i(-1, 0, 1), //
	Vector3i(1, 0, 1), //

	Vector3i(0, 1, -1), //
	Vector3i(-1, 1, 0), //
	Vector3i(0, 1, 1), //
	Vector3i(1, 1, 0) //
};

const unsigned int g_edge_corners[EDGE_COUNT][2] = {
	{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, //
	{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }, //
	{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 } //
};

// 顺序无关紧要
const Vector3i g_moore_neighboring_3d[MOORE_NEIGHBORING_3D_COUNT] = {
	Vector3i(-1, -1, -1),
	Vector3i(0, -1, -1),
	Vector3i(1, -1, -1),
	Vector3i(-1, -1, 0),
	Vector3i(0, -1, 0),
	Vector3i(1, -1, 0),
	Vector3i(-1, -1, 1),
	Vector3i(0, -1, 1),
	Vector3i(1, -1, 1),

	Vector3i(-1, 0, -1),
	Vector3i(0, 0, -1),
	Vector3i(1, 0, -1),
	Vector3i(-1, 0, 0),
	// Vector3i(0,0,0),
	Vector3i(1, 0, 0),
	Vector3i(-1, 0, 1),
	Vector3i(0, 0, 1),
	Vector3i(1, 0, 1),

	Vector3i(-1, 1, -1),
	Vector3i(0, 1, -1),
	Vector3i(1, 1, -1),
	Vector3i(-1, 1, 0),
	Vector3i(0, 1, 0),
	Vector3i(1, 1, 0),
	Vector3i(-1, 1, 1),
	Vector3i(0, 1, 1),
	Vector3i(1, 1, 1),
};

// 顺序很重要：
// 这用于多线程环境，我们可能按 XYZ 顺序遍历区块，以避免死锁。
const Vector3i g_ordered_moore_area_3d[MOORE_AREA_3D_COUNT] = { //
	Vector3i(-1, -1, -1), //
	Vector3i(0, -1, -1), //
	Vector3i(1, -1, -1), //
	Vector3i(-1, 0, -1), //
	Vector3i(0, 0, -1), //
	Vector3i(1, 0, -1), //
	Vector3i(-1, 1, -1), //
	Vector3i(0, 1, -1), //
	Vector3i(1, 1, -1), //

	Vector3i(-1, -1, 0), //
	Vector3i(0, -1, 0), //
	Vector3i(1, -1, 0), //
	Vector3i(-1, 0, 0), //
	Vector3i(0, 0, 0), //
	Vector3i(1, 0, 0), //
	Vector3i(-1, 1, 0), //
	Vector3i(0, 1, 0), //
	Vector3i(1, 1, 0), //

	Vector3i(-1, -1, 1), //
	Vector3i(0, -1, 1), //
	Vector3i(1, -1, 1), //
	Vector3i(-1, 0, 1), //
	Vector3i(0, 0, 1), //
	Vector3i(1, 0, 1), //
	Vector3i(-1, 1, 1), //
	Vector3i(0, 1, 1), //
	Vector3i(1, 1, 1)
};

const int g_opposite_side[6] = {
	Cube::SIDE_NEGATIVE_X, //
	Cube::SIDE_POSITIVE_X, //
	Cube::SIDE_POSITIVE_Y, //
	Cube::SIDE_NEGATIVE_Y, //
	Cube::SIDE_POSITIVE_Z, //
	Cube::SIDE_NEGATIVE_Z //
};

// clang-format on

Cube::Side dir_to_side(Vector3i d) {
	for (unsigned int side = 0; side < Cube::SIDE_COUNT; ++side) {
		if (Cube::g_side_normals[side] == d) {
			return Cube::Side(side);
		}
	}
	VOXEL_PRINT_ERROR("Side not found");
	return Cube::SIDE_FRONT;
}

} // namespace voxel::Cube
