#ifndef VOXEL_BLOCKY_BAKED_LIBRARY_H
#define VOXEL_BLOCKY_BAKED_LIBRARY_H

#include "../../constants/cube_tables.h"
#include "../../util/containers/dynamic_bitset.h"
#include "../../util/containers/fixed_array.h"
#include "../../util/containers/std_unordered_map.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/core/aabb.h"
#include <core/math/color.h>
#include "../../util/math/vector2f.h"
#include "../../util/math/vector3f.h"

// 这是由 blocky 网格生成器直接消费的数据。

namespace voxel::blocky {

// 上限基于 VoxelMesherBlocky 支持的最大值。
// 支持更多需要将体素大小翻倍（32 位），但这种情况很可疑。Minecraft 的方块状态
// 甚至都达不到该上限的四分之一。需要更多听起来就不是
// 正确的做法。
static constexpr unsigned int MAX_MODELS = 65536;
// 与烘焙流体索引方式相关的上限
static constexpr unsigned int MAX_FLUIDS = 256;

// 材质数量必须控制在最低限度。256 已经很多了，但这只影响性能。这个上限
// 是超出后代码将无法工作的临界值。理论上仍可提高（需要一些代码修改，
// 将 uint16_t 改为 uint32_t），但没有实际意义。
static constexpr unsigned int MAX_MATERIALS = 65536;

// 约定表示"无"。
// 不要在此索引处分配非空模型。
static const uint16_t AIR_ID = 0;
static const uint8_t NULL_FLUID_INDEX = 255;
static constexpr uint32_t MAX_SURFACES = 2;

// 网格生成器严格使用的纯数据。
// 它之所以独立，是因为它将在多线程环境中使用，
// 而生成该数据的配置可随时被用户修改。
// 此外，它比 Godot 资源更轻量。
struct BakedModel {
	struct SideSurface {
		StdVector<Vector3f> positions;
		StdVector<Vector2f> uvs;
		StdVector<int> indices;
		StdVector<float> tangents;
		// 不存储法线，因为假定整个侧面的法线相同

		void clear() {
			positions.clear();
			uvs.clear();
			indices.clear();
			tangents.clear();
		}
	};

	struct Surface {
		// 模型的内部部分。
		StdVector<Vector3f> positions;
		StdVector<Vector3f> normals;
		StdVector<Vector2f> uvs;
		StdVector<int> indices;
		StdVector<float> tangents;

		uint32_t material_id = 0;
		bool collision_enabled = true;

		void clear() {
			positions.clear();
			normals.clear();
			uvs.clear();
			indices.clear();
			tangents.clear();
		}
	};

	struct Model {
		// 一个模型最多可以有 2 种材质。
		// 如果需要更多，或分析结果说明用 vector 更好，我们可以改用它？
		FixedArray<Surface, MAX_SURFACES> surfaces;
		// 模型侧面：将它们分开存放，这样便于遮挡剔除。
		FixedArray<FixedArray<SideSurface, MAX_SURFACES>, Cube::SIDE_COUNT> sides_surfaces;
		unsigned int surface_count = 0;
		// 缓存的信息，用于提前检查这种情况。
		// 位按 Cube::Side 枚举索引。
		uint8_t empty_sides_mask = 0;
		uint8_t full_sides_mask = 0;

		// 表示每个侧面的"形状"，以便在与相邻块接触时快速剔除。
		// 侧面图案仍基于所有表面的组合来确定。
		FixedArray<uint32_t, Cube::SIDE_COUNT> side_pattern_indices;
		// 侧面剔除是全有或全无。
		// 如果要用烘焙模型支持部分剔除（处理"阶梯"状流体模型时需要），
		// 我们需要另一张查找表：给定两个侧面图案，输出预先裁剪好的替代几何数据。
		// 但这需要大量额外数据和预计算，而且需要此功能的场景
		// 可以改用其他方法，例如程序化生成几何体。

		// [side][neighbor_shape_id] => 预先裁剪的 SideSurfaces
		// 当侧面通过可见性测试且启用镂空时，尝试使用的 Surface。
		// 如果该容器中的 SideSurface 为空或未找到，则回退到完整表面
		FixedArray<StdUnorderedMap<uint32_t, FixedArray<SideSurface, MAX_SURFACES>>, Cube::SIDE_COUNT>
				cutout_side_surfaces;
		// TODO ^ 改成 UniquePtr？该数组占用的空间，对于这一本质上属于小众的功能而言太大

		void clear() {
			for (Surface &surface : surfaces) {
				surface.clear();
			}
			for (FixedArray<SideSurface, MAX_SURFACES> &side_surfaces : sides_surfaces) {
				for (SideSurface &side_surface : side_surfaces) {
					side_surface.clear();
				}
			}
		}
	};

	Model model;
	Color color;
	uint8_t transparency_index;
	bool culls_neighbors;
	bool contributes_to_ao;
	bool empty = true;
	bool is_random_tickable;
	bool is_transparent;
	bool cutout_sides_enabled = false;
	uint8_t fluid_index = NULL_FLUID_INDEX;
	uint8_t fluid_level;
	bool lod_skirts;

	uint32_t box_collision_mask;
	uint32_t tags_mask;
	StdVector<AABB> box_collision_aabbs;

	inline void clear() {
		model.clear();
		empty = true;
	}
};

struct BakedFluid {
	static constexpr float TOP_HEIGHT = 0.9375f;
	static constexpr float BOTTOM_HEIGHT = 0.0625f;

	struct Surface {
		StdVector<Vector3f> positions;
		StdVector<int> indices;
		StdVector<float> tangents;
		// 不存储法线，因为假定整个侧面的法线相同
		// 没有 UV，因为 UV 是为动画生成的

		void clear() {
			positions.clear();
			indices.clear();
			tangents.clear();
		}
	};

	FixedArray<Surface, Cube::SIDE_COUNT> side_surfaces;

	// StdVector<uint16_t> level_model_indices;
	uint32_t material_id = 0;
	uint8_t max_level = 1;
	bool dip_when_flowing_down = false;
	// uint32_t box_collision_mask = 0;
};

struct BakedLibrary {
	// 二维数组：{ X : 图案 A, Y : 图案 B } => A 是否遮挡 B
	// 索引为 X + Y * 图案数量
	DynamicBitset side_pattern_culling;
	unsigned int side_pattern_count = 0;
	// 大量数据可能被移动，但仅在加载时发生。
	StdVector<BakedModel> models;
	StdVector<BakedFluid> fluids;

	// struct VariantInfo {
	// 	uint16_t type_index;
	// 	FixedArray<uint8_t, 4> attributes;
	// };

	// StdVector<VariantInfo> variant_infos;

	unsigned int indexed_materials_count = 0;

	inline bool has_model(uint32_t i) const {
		return i < models.size();
	}

	inline bool get_side_pattern_occlusion(unsigned int pattern_a, unsigned int pattern_b) const {
#ifdef DEBUG_ENABLED
		CRASH_COND(pattern_a >= side_pattern_count);
		CRASH_COND(pattern_b >= side_pattern_count);
#endif
		return side_pattern_culling.get(pattern_a + pattern_b * side_pattern_count);
	}
};

} // namespace voxel::blocky

#endif // VOXEL_BLOCKY_BAKED_LIBRARY_H
