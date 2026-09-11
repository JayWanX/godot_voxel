#ifndef VOXEL_STORAGE_FUNCS_H
#define VOXEL_STORAGE_FUNCS_H

#include "../constants/voxel_constants.h"
#include "../util/containers/span.h"
#include "../util/math/vector3i.h"
#include "../util/math/ortho_basis.h"
#include <cstdint>

namespace voxel {

inline void clip_copy_region_coord(int &src_min, int &src_max, const int src_size, int &dst_min, const int dst_size) {
	// 夹取源区域并收缩目标区域以处理移动的边界
	if (src_min < 0) {
		dst_min += -src_min;
		src_min = 0;
	}
	if (src_max > src_size) {
		src_max = src_size;
	}
	// 夹取目标区域并收缩源区域以处理移动的边界
	if (dst_min < 0) {
		src_min += -dst_min;
		dst_min = 0;
	}
	const int dst_w = src_max - src_min;
	const int dst_max = dst_min + dst_w;
	if (dst_max > dst_size) {
		src_max -= dst_max - dst_size;
	}
	// 此时源区域的大小可能为负数，这意味着没有任何内容可复制。
	// 这必须由调用方检查。
}

// 裁剪用于将 3D 容器的子区域复制到另一个 3D 容器的坐标。
// 结果可能为零或负大小，因此必须先检查再继续。
inline void clip_copy_region(
		Vector3i &src_min,
		Vector3i &src_max,
		const Vector3i &src_size,
		Vector3i &dst_min,
		const Vector3i &dst_size
) {
	clip_copy_region_coord(src_min.x, src_max.x, src_size.x, dst_min.x, dst_size.x);
	clip_copy_region_coord(src_min.y, src_max.y, src_size.y, dst_min.y, dst_size.y);
	clip_copy_region_coord(src_min.z, src_max.z, src_size.z, dst_min.z, dst_size.z);
}

void copy_3d_region_zxy(
		Span<uint8_t> dst,
		Vector3i dst_size,
		Vector3i dst_min,
		Span<const uint8_t> src,
		Vector3i src_size,
		Vector3i src_min,
		Vector3i src_max,
		size_t item_size
);

template <typename T>
inline void copy_3d_region_zxy(
		Span<T> dst,
		Vector3i dst_size,
		Vector3i dst_min,
		Span<const T> src,
		Vector3i src_size,
		Vector3i src_min,
		Vector3i src_max
) {
	// 用 GCC 编译时，方法名前的 `template` 关键字是必需的
	copy_3d_region_zxy(
			dst.template reinterpret_cast_to<uint8_t>(),
			dst_size,
			dst_min,
			src.template reinterpret_cast_to<const uint8_t>(),
			src_size,
			src_min,
			src_max,
			sizeof(T)
	);
}

template <typename T>
void fill_3d_region_zxy(Span<T> dst, Vector3i dst_size, Vector3i dst_min, Vector3i dst_max, const T value) {
	using namespace math;
	Vector3iUtil::sort_min_max(dst_min, dst_max);
	dst_min.x = clamp(dst_min.x, 0, dst_size.x);
	dst_min.y = clamp(dst_min.y, 0, dst_size.y);
	dst_min.z = clamp(dst_min.z, 0, dst_size.z);
	dst_max.x = clamp(dst_max.x, 0, dst_size.x);
	dst_max.y = clamp(dst_max.y, 0, dst_size.y);
	dst_max.z = clamp(dst_max.z, 0, dst_size.z);
	const Vector3i area_size = dst_max - dst_min;
	if (area_size.x <= 0 || area_size.y <= 0 || area_size.z <= 0) {
		// 退化的区域，不复制任何内容。
		return;
	}

#ifdef DEBUG_ENABLED
	VOXEL_ASSERT_RETURN(Vector3iUtil::get_volume_u64(area_size) <= dst.size());
#endif

	if (area_size == dst_size) {
		for (unsigned int i = 0; i < dst.size(); ++i) {
			dst[i] = value;
		}

	} else {
		const unsigned int dst_row_offset = dst_size.y;
		Vector3i pos;
		for (pos.z = 0; pos.z < area_size.z; ++pos.z) {
			unsigned int dst_ri = Vector3iUtil::get_zxy_index(dst_min + pos, dst_size);
			for (pos.x = 0; pos.x < area_size.x; ++pos.x) {
				// 填充一行
				for (pos.y = 0; pos.y < area_size.y; ++pos.y) {
					dst[dst_ri + pos.y] = value;
				}
				dst_ri += dst_row_offset;
			}
		}
	}
}

// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#fundamentals-fixedconv
// 将 int8 值转换为 [-1..1] 范围内的 float，其中 0 有精确的对应值。
// -128 是 int8 中一个没有对应结果的值，它会被夹取到 -1。
inline constexpr float s8_to_snorm(int8_t v) {
	return math::max(v / 127.f, -1.f);
}
inline constexpr float s8_to_snorm_noclamp(int8_t v) {
	return v / 127.f;
}

// 将 [-1..1] 范围内的 float 值转换为 int8。
// 如果 float 超出预期范围，则会被夹取。
inline constexpr int8_t snorm_to_s8(float v) {
	return math::clamp(v, -1.f, 1.f) * 127;
}

// 将 int8 值转换为 [-1..1] 范围内的 float，其中 0 有精确的对应值。
// -32767 是 int16 中一个没有对应结果的值，它会被夹取到 -1。
inline constexpr float s16_to_snorm(int16_t v) {
	return math::max(v / 32767.f, -1.f);
}
inline constexpr float s16_to_snorm_noclamp(int16_t v) {
	return v / 32767.f;
}

// 将 [-1..1] 范围内的 float 值转换为 int16。
// 如果 float 超出预期范围，则会被夹取。
inline constexpr int16_t snorm_to_s16(float v) {
	return math::clamp(v, -1.f, 1.f) * 32767;
}

namespace legacy {

inline float u8_to_snorm(uint8_t v) {
	return (static_cast<float>(v) - 0x7f) * constants::INV_0x7f;
}

inline float u16_to_snorm(uint16_t v) {
	return (static_cast<float>(v) - 0x7fff) * constants::INV_0x7fff;
}

inline uint8_t snorm_to_u8(float v) {
	return voxel::math::clamp(static_cast<int>(128.f * v + 128.f), 0, 0xff);
}

inline uint16_t snorm_to_u16(float v) {
	return voxel::math::clamp(static_cast<int>(0x8000 * v + 0x8000), 0, 0xffff);
}

} // namespace legacy

// 获取要添加到变换后 3D 坐标的偏移量，使变换能保持单元格位于目标数组内
//（否则旋转可能导致负坐标，这不是 3D 数组想要的）
Vector3i get_3d_array_transform_origin(const math::OrthoBasis &basis, const Vector3i src_size, Vector3i *out_dst_size);

// 使用基旋转/翻转/转置 3D 数组的内容。
// 返回变换后的大小。体积保持不变。
// 数组的坐标约定使用 ZXY（index+1 对应 Y+1）。
template <typename T>
Vector3i transform_3d_array_zxy(
		Span<const T> src_grid,
		Span<T> dst_grid,
		Vector3i src_size,
		math::OrthoBasis basis,
		Vector3i *out_transform_origin = nullptr
) {
	VOXEL_ASSERT_RETURN_V(Vector3iUtil::is_unit_vector(basis.x), src_size);
	VOXEL_ASSERT_RETURN_V(Vector3iUtil::is_unit_vector(basis.y), src_size);
	VOXEL_ASSERT_RETURN_V(Vector3iUtil::is_unit_vector(basis.z), src_size);
	VOXEL_ASSERT_RETURN_V(src_grid.size() == Vector3iUtil::get_volume_u64(src_size), src_size);
	VOXEL_ASSERT_RETURN_V(dst_grid.size() == Vector3iUtil::get_volume_u64(src_size), src_size);

	Vector3i dst_size;
	const Vector3i origin = get_3d_array_transform_origin(basis, src_size, &dst_size);
	if (out_transform_origin != nullptr) {
		*out_transform_origin = origin;
	}

	int src_i = 0;

	for (int z = 0; z < src_size.z; ++z) {
		for (int x = 0; x < src_size.x; ++x) {
			for (int y = 0; y < src_size.y; ++y) {
				// TODO 优化：可以移到外层循环，我们只需要给 dst_i 加上一个数
				const int dst_x = origin.x + x * basis.x.x + y * basis.y.x + z * basis.z.x;
				const int dst_y = origin.y + x * basis.x.y + y * basis.y.y + z * basis.z.y;
				const int dst_z = origin.z + x * basis.x.z + y * basis.y.z + z * basis.z.z;
				const int dst_i = dst_y + dst_size.y * (dst_x + dst_size.x * dst_z);
				dst_grid[dst_i] = src_grid[src_i];
				++src_i;
			}
		}
	}

	return dst_size;
}

} // namespace voxel

#endif // VOXEL_STORAGE_FUNCS_H
