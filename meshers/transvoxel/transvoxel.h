#ifndef VOXEL_TRANSVOXEL_H
#define VOXEL_TRANSVOXEL_H

#include "../../storage/voxel_buffer.h"
#include "../../util/containers/fixed_array.h"
#include <core/math/color.h>
#include "../../util/math/vector2f.h"
#include "../../util/math/vector3f.h"
#include <core/math/vector3i.h>

#include <vector>

namespace voxel::transvoxel {

// 朝向负轴方向需要多少额外体素
static const int MIN_PADDING = 1;
// 朝向正轴方向需要多少额外体素
static const int MAX_PADDING = 2;
// Transvoxel 保证每个 2x2x2 体素单元生成的三角形数量有上限。
static const unsigned int MAX_TRIANGLES_PER_CELL = 5;

enum TexturingMode {
	TEXTURES_NONE,
	// 混合给定数据块中最具代表性的 4 种纹理，忽略其余纹理。
	// 纹理索引和混合因子具有 4 位精度（最多 16 种纹理和 16 种过渡梯度），
	// 并分别编码在 UV.x 和 UV.y 中。
	TEXTURES_MIXEL4_S4,
	// 每个体素只有一个材质索引，着色器中最多可混合 4 个
	TEXTURES_SINGLE_S4,
#ifdef VOXEL_ENABLE_TRANSVOXEL_MATERIAL_SINGLE_S2
	// 每个体素只有一个材质索引，着色器中最多可混合 2 个
	TEXTURES_SINGLE_S2,
#endif
	TEXTURES_MODE_COUNT
};

struct LodAttrib {
	Vector3f secondary_position;
	// 掩码，指示顶点所属的单元是否位于数据块的一侧。
	// 每个位对应一个侧面。
	// 0: -X
	// 1: +X
	// 2: -Y
	// 3: +Y
	// 4: -Z
	// 5: +Z
	uint8_t cell_border_mask;
	// 掩码，指示顶点是否位于数据块的一侧。约定同上。
	uint8_t vertex_border_mask;
	// 标志，指示顶点是否属于过渡网格。
	uint8_t transition;
	// 未使用。用于对齐到 4*sizeof(float)，以便复制到渲染缓冲区。
	uint8_t _pad;
};

// struct TextureAttrib {
// 	uint8_t index0;
// 	uint8_t index1;
// 	uint8_t index2;
// 	uint8_t index3;
// 	uint8_t weight0;
// 	uint8_t weight1;
// 	uint8_t weight2;
// 	uint8_t weight3;
// };

struct MeshArrays {
	StdVector<Vector3f> vertices;
	StdVector<Vector3f> normals;
	StdVector<LodAttrib> lod_data;

	// 并非真正的浮点数，只是对齐到 32 位。实际布局取决于纹理模式
	// TODO 也许可以直接使用正确的结构体，并在最后重新解释？之所以一直是 float，是因为 Godot 只允许我们以 float 传递
	StdVector<float> texturing_data_1f32;
	StdVector<Vector2f> texturing_data_2f32;

	StdVector<int32_t> indices;

	void clear() {
		vertices.clear();
		normals.clear();
		lod_data.clear();
		texturing_data_1f32.clear();
		texturing_data_2f32.clear();
		indices.clear();
	}

	int add_vertex(
			const Vector3f primary,
			const Vector3f normal,
			const uint8_t cell_border_mask,
			const uint8_t vertex_border_mask,
			const uint8_t transition,
			const Vector3f secondary
	) {
		int vi = vertices.size();
		vertices.push_back(primary);
		normals.push_back(normal);
		lod_data.push_back({ secondary, cell_border_mask, vertex_border_mask, transition, 0 });
		return vi;
	}
};

struct ReuseCell {
	FixedArray<int, 4> vertices;
	uint32_t packed_texture_indices = 0;
};

struct ReuseTransitionCell {
	FixedArray<int, 12> vertices;
	uint32_t packed_texture_indices = 0;
};

class Cache {
public:
	void reset_reuse_cells(Vector3i p_block_size) {
		_block_size = p_block_size;
		const unsigned int deck_area = _block_size.x * _block_size.y;
		for (unsigned int i = 0; i < _cache.size(); ++i) {
			StdVector<ReuseCell> &deck = _cache[i];
			deck.resize(deck_area);
			for (size_t j = 0; j < deck.size(); ++j) {
				fill(deck[j].vertices, -1);
			}
		}
	}

	void reset_reuse_cells_2d(Vector3i p_block_size) {
		for (unsigned int i = 0; i < _cache_2d.size(); ++i) {
			StdVector<ReuseTransitionCell> &row = _cache_2d[i];
			row.resize(p_block_size.x);
			for (size_t j = 0; j < row.size(); ++j) {
				fill(row[j].vertices, -1);
			}
		}
	}

	ReuseCell &get_reuse_cell(Vector3i pos) {
		unsigned int j = pos.z & 1;
		unsigned int i = pos.y * _block_size.x + pos.x;
		VOXEL_ASSERT(i < _cache[j].size());
		return _cache[j][i];
	}

	ReuseTransitionCell &get_reuse_cell_2d(int x, int y) {
		unsigned int j = y & 1;
		unsigned int i = x;
		VOXEL_ASSERT(i < _cache_2d[j].size());
		return _cache_2d[j][i];
	}

private:
	FixedArray<StdVector<ReuseCell>, 2> _cache;
	FixedArray<StdVector<ReuseTransitionCell>, 2> _cache_2d;
	Vector3i _block_size;
};

// 这只是为了将常规网格计算出的某些数据复用到过渡网格中
struct DefaultTextureIndicesData {
	FixedArray<uint8_t, 4> indices;
	uint32_t packed_indices = 0;
	// TODO 是否使用 optional？
	bool use = false;
};

struct CellInfo {
	Vector3i position;
	uint32_t triangle_count;
};

DefaultTextureIndicesData build_regular_mesh(
		const VoxelBuffer &voxels,
		const unsigned int sdf_channel,
		const uint32_t lod_index,
		const TexturingMode texturing_mode,
		Cache &cache,
		MeshArrays &output,
		StdVector<CellInfo> *cell_infos,
		const float edge_clamp_margin,
		const bool textures_ignore_air_voxels
);

void build_transition_mesh(
		const VoxelBuffer &voxels,
		const unsigned int sdf_channel,
		const int direction,
		const uint32_t lod_index,
		const TexturingMode texturing_mode,
		Cache &cache,
		MeshArrays &output,
		DefaultTextureIndicesData default_texture_indices_data,
		const float edge_clamp_margin,
		const bool textures_ignore_air_voxels
);

} // namespace voxel::transvoxel

#endif // VOXEL_TRANSVOXEL_H
