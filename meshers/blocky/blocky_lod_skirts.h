// 只在网格生成器中包含一次以定义它。
// 之所以单独放在一个头文件中，是因为我想拆分内容。

#include "../../util/math/vector3f.h"
#include "blocky_tint_sampler.h"
#include "voxel_blocky_library_base.h"
#include "voxel_blocky_model.h"
#include "voxel_mesher_blocky.h"

namespace voxel::blocky {

Vector3f side_to_block_coordinates(const Vector3f v, const VoxelBlockyModel::Side side) {
	switch (side) {
		case VoxelBlockyModel::SIDE_NEGATIVE_X:
		case VoxelBlockyModel::SIDE_POSITIVE_X:
			return v.zyx();
		case VoxelBlockyModel::SIDE_NEGATIVE_Y:
		case VoxelBlockyModel::SIDE_POSITIVE_Y:
			// return v.zxy();
			return v.yzx();
		case VoxelBlockyModel::SIDE_NEGATIVE_Z:
		case VoxelBlockyModel::SIDE_POSITIVE_Z:
			return v;
		default:
			VOXEL_CRASH();
			return v;
	}
}

int get_side_sign(const VoxelBlockyModel::Side side) {
	switch (side) {
		case VoxelBlockyModel::SIDE_NEGATIVE_X:
		case VoxelBlockyModel::SIDE_NEGATIVE_Y:
		case VoxelBlockyModel::SIDE_NEGATIVE_Z:
			return -1;
		case VoxelBlockyModel::SIDE_POSITIVE_X:
		case VoxelBlockyModel::SIDE_POSITIVE_Y:
		case VoxelBlockyModel::SIDE_POSITIVE_Z:
			return 1;
		default:
			VOXEL_CRASH();
			return 1;
	}
}

// 为数据块侧面每个暴露于空气的体素添加额外的体素侧面几何体。这会在不同 LOD 的网格并排放置时
// 形成"接缝"，用以隐藏 LOD 裂缝。
// 该方法无需访问子 LOD 的体素。缺点是它不能总是隐藏所有裂缝，
// 但假设是大多数情况下都能做到。
// 未处理 AO，可能也不需要处理
template <typename TModelID>
void append_side_skirts(
		Span<const TModelID> buffer,
		const TintSampler tint_sampler,
		const Vector3T<int> jump,
		const int z, // 第一个或最后一个体素的坐标（不在填充区域内）
		const int size_x,
		const int size_y,
		const VoxelBlockyModel::Side side,
		const BakedLibrary &library,
		StdVector<VoxelMesherBlocky::Arrays> &out_arrays_per_material
) {
	constexpr TModelID AIR = 0;
	constexpr int pad = 1;

	const int z_base = z * jump.z;
	const int side_sign = get_side_sign(side);

	// 发送到数据块网格化的缓冲区包含外层和内层体素。
	// 内层体素是实际被网格化的体素。
	// 外层体素不构成最终网格的一部分，但它们的存在是为了知道如何剔除与之接触的内层体素的侧面。

	// 对于数据块侧面的每个外层体素（使用侧面相对坐标）
	for (int x = pad; x < size_x - pad; ++x) {
		for (int y = pad; y < size_y - pad; ++y) {
			const int buffer_index = x * jump.x + y * jump.y + z_base;
			const int v = buffer[buffer_index];

			if (v == AIR) {
				continue;
			}

			// 检查体素是否暴露于空气

			const int nv0 = buffer[buffer_index - jump.x];
			const int nv1 = buffer[buffer_index + jump.x];
			const int nv2 = buffer[buffer_index - jump.y];
			const int nv3 = buffer[buffer_index + jump.y];

			if (nv0 != AIR && nv1 != AIR && nv2 != AIR && nv3 != AIR) {
				continue;
			}

			// 检查外层体素是否遮挡了内层体素
			// （这个检查实际上并不精确，也许必须使用库做完整的遮挡检查？）

			const TModelID nv4 = buffer[buffer_index - side_sign * jump.z];
			if (nv4 == AIR) {
				continue;
			}

			// 如果是这样，为该内层体素的侧面添加几何体

			const Vector3f pos = side_to_block_coordinates(Vector3f(x - pad, y - pad, z - (side_sign + 1)), side);

			if (nv4 >= library.models.size()) {
				// 无效 ID，跳过
				continue;
			}
			const BakedModel &voxel_baked_data = library.models[nv4];

			if (!voxel_baked_data.lod_skirts) {
				// 一个典型问题是制作海洋：
				// - 裙边会出现在水面之后，因此在这种情况下不是好方案。
				// - 如果海平面在不同 LOD 下无法对齐，那么无论如何都会出现 LOD "裂缝"。对此我没有
				// 好的解决方案。一种变通办法是选择在每个 LOD 下都能对齐的海平面（如 Y=0），
				// 让接缝出现在通常少得多的情况下。
				// - 另一种方法是在水平方向而非垂直方向降低 LOD 分辨率，但这在远距离下内存成本很高，
				// 所以不是万灵药。
				// - 在远距离时让水变得不透明？如果可以接受，这是一个不错的修复方式（Distant Horizons
				// 模组曾这样做过），但要么需要自定义着色器，要么需要在库中为不同 LOD 指定
				// 不同的模型
				continue;
			}

			const BakedModel::Model &model = voxel_baked_data.model;
			const Color tint = voxel_baked_data.color * tint_sampler.evaluate(Vector3i(x, y, z));

			const FixedArray<BakedModel::SideSurface, MAX_SURFACES> &side_surfaces = model.sides_surfaces[side];

			for (unsigned int surface_index = 0; surface_index < model.surface_count; ++surface_index) {
				const BakedModel::Surface &surface = model.surfaces[surface_index];
				VoxelMesherBlocky::Arrays &arrays = out_arrays_per_material[surface.material_id];

				const BakedModel::SideSurface &side_surface = side_surfaces[surface_index];
				const unsigned int vertex_count = side_surface.positions.size();

				// TODO 以下代码与主网格化函数几乎相同。
				// 一旦 blocky 网格生成器的功能合并（blocky 流体、阴影遮挡体），我们应该将其抽到公共代码。
				// 烘焙遮挡部分应分离出来，在颜色调制之上运行。
				// 索引偏移量最终可能并不需要向量。

				const unsigned int index_offset = arrays.positions.size();

				{
					const unsigned int append_index = arrays.positions.size();
					arrays.positions.resize(arrays.positions.size() + vertex_count);
					Vector3f *w = arrays.positions.data() + append_index;
					for (unsigned int i = 0; i < vertex_count; ++i) {
						w[i] = side_surface.positions[i] + pos;
					}
				}

				{
					const unsigned int append_index = arrays.uvs.size();
					arrays.uvs.resize(arrays.uvs.size() + vertex_count);
					memcpy(arrays.uvs.data() + append_index, side_surface.uvs.data(), vertex_count * sizeof(Vector2f));
				}

				if (side_surface.tangents.size() > 0) {
					const unsigned int append_index = arrays.tangents.size();
					arrays.tangents.resize(arrays.tangents.size() + vertex_count * 4);
					memcpy(arrays.tangents.data() + append_index,
						   side_surface.tangents.data(),
						   (vertex_count * 4) * sizeof(float));
				}

				{
					const int append_index = arrays.normals.size();
					arrays.normals.resize(arrays.normals.size() + vertex_count);
					Vector3f *w = arrays.normals.data() + append_index;
					for (unsigned int i = 0; i < vertex_count; ++i) {
						w[i] = to_vec3f(Cube::g_side_normals[side]);
					}
				}

				{
					const int append_index = arrays.colors.size();
					arrays.colors.resize(arrays.colors.size() + vertex_count);
					Color *w = arrays.colors.data() + append_index;
					for (unsigned int i = 0; i < vertex_count; ++i) {
						w[i] = tint;
					}
				}

				{
					const unsigned int index_count = side_surface.indices.size();
					unsigned int i = arrays.indices.size();
					arrays.indices.resize(arrays.indices.size() + index_count);
					int *w = arrays.indices.data();
					for (unsigned int j = 0; j < index_count; ++j) {
						w[i++] = index_offset + side_surface.indices[j];
					}
				}
			}
		}
	}
}

template <typename TModelID>
void append_skirts(
		Span<const TModelID> buffer,
		const Vector3i size,
		StdVector<VoxelMesherBlocky::Arrays> &out_arrays_per_material,
		const BakedLibrary &library,
		const TintSampler tint_sampler
) {
	VOXEL_PROFILE_SCOPE();

	const Vector3T<int> jump(size.y, 1, size.x * size.y);

	// 快捷方式
	StdVector<VoxelMesherBlocky::Arrays> &out = out_arrays_per_material;
	constexpr VoxelBlockyModel::Side NEGATIVE_X = VoxelBlockyModel::SIDE_NEGATIVE_X;
	constexpr VoxelBlockyModel::Side POSITIVE_X = VoxelBlockyModel::SIDE_POSITIVE_X;
	constexpr VoxelBlockyModel::Side NEGATIVE_Y = VoxelBlockyModel::SIDE_NEGATIVE_Y;
	constexpr VoxelBlockyModel::Side POSITIVE_Y = VoxelBlockyModel::SIDE_POSITIVE_Y;
	constexpr VoxelBlockyModel::Side NEGATIVE_Z = VoxelBlockyModel::SIDE_NEGATIVE_Z;
	constexpr VoxelBlockyModel::Side POSITIVE_Z = VoxelBlockyModel::SIDE_POSITIVE_Z;

	append_side_skirts(buffer, tint_sampler, jump.xyz(), 0, size.x, size.y, NEGATIVE_Z, library, out);
	append_side_skirts(buffer, tint_sampler, jump.xyz(), (size.z - 1), size.x, size.y, POSITIVE_Z, library, out);
	append_side_skirts(buffer, tint_sampler, jump.zyx(), 0, size.z, size.y, NEGATIVE_X, library, out);
	append_side_skirts(buffer, tint_sampler, jump.zyx(), (size.x - 1), size.z, size.y, POSITIVE_X, library, out);
	append_side_skirts(buffer, tint_sampler, jump.zxy(), 0, size.z, size.x, NEGATIVE_Y, library, out);
	append_side_skirts(buffer, tint_sampler, jump.zxy(), (size.y - 1), size.z, size.x, POSITIVE_Y, library, out);
}

} // namespace voxel::blocky
