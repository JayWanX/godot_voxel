#ifndef VOXEL_TRIANGLE_H
#define VOXEL_TRIANGLE_H

#include "vector2f.h"
#include "vector3.h"
#include "vector3d.h"
#include "vector3f.h"

namespace voxel::math {

// Geometry::is_point_in_triangle() 的浮点版本
inline bool is_point_in_triangle(const Vector2f &s, const Vector2f &a, const Vector2f &b, const Vector2f &c) {
	const Vector2f an = a - s;
	const Vector2f bn = b - s;
	const Vector2f cn = c - s;

	const bool orientation = cross(an, bn) > 0;

	if ((cross(bn, cn) > 0) != orientation) {
		return false;
	}

	return (cross(cn, an) > 0) == orientation;
}

inline float get_triangle_area_squared_x2(Vector3f p0, Vector3f p1, Vector3f p2) {
	const Vector3f p01 = p1 - p0;
	const Vector3f p02 = p2 - p0;
	const Vector3f c = math::cross(p01, p02);
	return math::length_squared(c);
}

inline float is_triangle_degenerate_approx(Vector3f p0, Vector3f p1, Vector3f p2, float epsilon_squared) {
	return get_triangle_area_squared_x2(p0, p1, p2) < epsilon_squared;
}

// inline float is_triangle_degenerate(Vector3f p0, Vector3f p1, Vector3f p2) {
// 	return p0 == p1 || p1 == p2 || p2 == p0;
// }

// Heron 公式在 SO 上被过度提及，但它要用 4 次平方根。本实现只用一次。
// 平行四边形的面积等于两条相邻边的向量的叉积的模，
// 因此三角形面积为其一半
inline float get_triangle_area(Vector3 p0, Vector3 p1, Vector3 p2) {
	const Vector3 p01 = p1 - p0;
	const Vector3 p02 = p2 - p0;
	const Vector3 c = p01.cross(p02);
	return 0.5 * c.length();
}

template <typename T>
inline T get_triangle_area(const Vector3T<T> p0, const Vector3T<T> p1, const Vector3T<T> p2) {
	const Vector3T<T> p01 = p1 - p0;
	const Vector3T<T> p02 = p2 - p0;
	const Vector3T<T> c = cross(p01, p02);
	return T(0.5) * length(c);
}

inline Vector3f get_triangle_barycentric_coordinates(Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p) {
	const float den = (p1.y - p2.y) * (p0.x - p2.x) + (p2.x - p1.x) * (p0.y - p2.y);
	Vector3f weights;
	if (Math::is_zero_approx(den)) {
		weights.x = 0;
		weights.y = 0;
	} else {
		weights.x = ((p1.y - p2.y) * (p.x - p2.x) + (p2.x - p1.x) * (p.y - p2.y)) / den;
		weights.y = ((p2.y - p0.y) * (p.x - p2.x) + (p0.x - p2.x) * (p.y - p2.y)) / den;
	}
	weights.z = 1.f - weights.x - weights.y;
	return weights;
}

// TODO 模板化？
inline Vector3f get_triangle_random_barycentric(const float rand1, const float rand2) {
	const float s = abs(rand1 - rand2);
	const float t = 0.5f * (rand1 + rand2 - s);
	const float u = 1.f - s - t;
	return Vector3f(s, t, u);
}

template <typename T>
inline T interpolate_triangle(const T a, const T b, const T c, const Vector3f barycentric) {
	return a * barycentric.x + b * barycentric.y + c * barycentric.z;
}

struct TriangleIntersectionResult {
	enum Case { //
		INTERSECTION,
		PARALLEL,
		NO_INTERSECTION
	};

	Case case_id;
	double distance;
};

// 最初来自 Godot Engine，经过调整以满足需求

// 最初来自 Godot Engine，经过调整以满足需求
inline TriangleIntersectionResult ray_intersects_triangle(
		const Vector3f &p_from,
		const Vector3f &p_dir,
		const Vector3f &p_v0,
		const Vector3f &p_v1,
		const Vector3f &p_v2
) {
	const Vector3f e1 = p_v1 - p_v0;
	const Vector3f e2 = p_v2 - p_v0;
	const Vector3f h = math::cross(p_dir, e2);
	const float a = math::dot(e1, h);

	if (Math::abs(a) < 0.00001f) {
		return { TriangleIntersectionResult::PARALLEL, -1 };
	}

	const float f = 1.0f / a;

	const Vector3f s = p_from - p_v0;
	const float u = f * math::dot(s, h);

	if ((u < 0.0f) || (u > 1.0f)) {
		return { TriangleIntersectionResult::NO_INTERSECTION, -1 };
	}

	const Vector3f q = math::cross(s, e1);

	const float v = f * math::dot(p_dir, q);

	if ((v < 0.0f) || (u + v > 1.0f)) {
		return { TriangleIntersectionResult::NO_INTERSECTION, -1 };
	}

	// 此时我们可以计算 t 来确定
	// 交点在直线的什么位置。
	const float t = f * math::dot(e2, q);

	if (t > 0.00001f) { // 射线相交
		// r_res = p_from + p_dir * t;
		return { TriangleIntersectionResult::INTERSECTION, t };

	} else { // 这意味着存在直线相交，但不存在射线相交。
		return { TriangleIntersectionResult::NO_INTERSECTION, -1 };
	}
}

inline TriangleIntersectionResult ray_intersects_triangle(
		const Vector3d &p_from,
		const Vector3d &p_dir,
		const Vector3d &p_v0,
		const Vector3d &p_v1,
		const Vector3d &p_v2
) {
	const Vector3d e1 = p_v1 - p_v0;
	const Vector3d e2 = p_v2 - p_v0;
	const Vector3d h = math::cross(p_dir, e2);
	const double a = math::dot(e1, h);

	if (Math::abs(a) < 0.000000001) { // 平行性测试。
		return { TriangleIntersectionResult::PARALLEL, -1 };
	}

	const double f = 1.0 / a;

	const Vector3d s = p_from - p_v0;
	const double u = f * math::dot(s, h);

	if ((u < 0.0) || (u > 1.0)) {
		return { TriangleIntersectionResult::NO_INTERSECTION, -1 };
	}

	const Vector3d q = math::cross(s, e1);
	const double v = f * math::dot(p_dir, q);

	if ((v < 0.0) || (u + v > 1.0)) {
		return { TriangleIntersectionResult::NO_INTERSECTION, -1 };
	}

	// 此时我们可以计算 t 来确定
	// 交点在直线的什么位置。
	const double t = f * math::dot(e2, q);

	if (t > 0.000000001) { // 射线相交
		// r_res = p_from + p_dir * t;
		return { TriangleIntersectionResult::INTERSECTION, t };

	} else { // 这意味着存在直线相交，但不存在射线相交。
		return { TriangleIntersectionResult::NO_INTERSECTION, -1 };
	}
}

// 如果你需要对三角形用同一方向进行大量射线检测，
struct BakedIntersectionTriangleForFixedDirection {
	Vector3f v0;
	Vector3f e1; // v1 - v0
	Vector3f e2; // v2 - v0
	Vector3f h;
	float f;

	bool bake(Vector3f p_v0, Vector3f p_v1, Vector3f p_v2, Vector3f p_dir) {
		v0 = p_v0;

		e1 = p_v1 - v0;
		e2 = p_v2 - v0;
		h = math::cross(p_dir, e2);
		const float a = math::dot(e1, h);
		if (Math::abs(a) < 0.00001f) {
			// 平行，永远不会命中
			return false;
		}
		f = 1.0f / a;
		return true;
	}

	// 注意，`p_dir` 必须与 `bake` 中使用的值相同
	inline TriangleIntersectionResult intersect(const Vector3f &p_from, const Vector3f &p_dir) {
		const Vector3f s = p_from - v0;
		const float u = f * math::dot(s, h);

		if ((u < 0.0f) || (u > 1.0f)) {
			return { TriangleIntersectionResult::NO_INTERSECTION, -1 };
		}

		const Vector3f q = math::cross(s, e1);

		const float v = f * math::dot(p_dir, q);

		if ((v < 0.0f) || (u + v > 1.0f)) {
			return { TriangleIntersectionResult::NO_INTERSECTION, -1 };
		}

		// 此时我们可以计算 t 来确定
		// 交点在直线的什么位置。
		const float t = f * math::dot(e2, q);

		if (t > 0.00001f) { // 射线相交
			// r_res = p_from + p_dir * t;
			return { TriangleIntersectionResult::INTERSECTION, t };

		} else { // 这意味着存在直线相交，但不存在射线相交。
			return { TriangleIntersectionResult::NO_INTERSECTION, -1 };
		}
	}
};

} // namespace voxel::math

#endif // VOXEL_TRIANGLE_H
