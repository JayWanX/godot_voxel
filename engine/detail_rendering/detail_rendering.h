#ifndef VOXEL_DETAIL_RENDERING_H
#define VOXEL_DETAIL_RENDERING_H

#include "../../util/containers/fixed_array.h"
#include "../../util/containers/span.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/ref_counted.h"
#include "../../util/macros.h"
#include "../../util/math/vector3f.h"

// #define VOXEL_VIRTUAL_TEXTURE_USE_TEXTURE_ARRAY
//  纹理数组用起来很方便，但层数上限往往太低（在 nVidia 1060 上为 2048），在数据块大小为 32 时
//  很容易超限。因此我们不得不继续使用带边距的 2D atlas。
#ifdef VOXEL_VIRTUAL_TEXTURE_USE_TEXTURE_ARRAY
#include "../../util/godot/classes/texture_array.h"
#endif
#include "../../util/godot/classes/image.h"
#include "../../util/godot/classes/texture_2d.h"

namespace voxel {

class VoxelGenerator;
class VoxelData;

// TODO 该系统可以扩展到不仅仅是法线
// - 纹理数据
// - 颜色
// - 某种深度（可用于从远处模拟水面）

// 对体素网格进行 UV 映射并非易事，但如果需要映射，另一种做法是将网格细分为
// 单元格网格（可以使用 Transvoxel 单元格）。在每个单元格中，根据单元格三角形法线的平均值
// 选择一个最适合的轴对齐投影。然后可以通过将像素投影到三角形上来生成一张 tile，
// 并存储在 atlas 中。之后着色器可以使用查找纹理读取 atlas 来定位 tile。

struct DetailRenderingSettings {
	// 如果启用，将为体素网格的每个单元格生成法线贴图 atlas，以便通过着色器
	// 添加更多视觉细节。
	bool enabled = false;
	// 开始生成法线贴图的 LOD 索引。
	uint8_t begin_lod_index = 2;
	// 从起始 LOD 开始使用的 tile 分辨率。在后续每个 LOD 索引处分辨率将翻倍。
	uint8_t tile_resolution_min = 4;
	uint8_t tile_resolution_max = 8;
	// 如果几何法线与计算法线之间的角度超过此角度，它们的朝向将被限制。
	uint8_t max_deviation_degrees = 60;
	// 如果启用，使用八面体压缩编码法线贴图，以少许质量为代价
	// 大幅减少内存占用（每像素使用 2 字节而非 3 字节）。
	bool octahedral_encoding_enabled = false;

	static constexpr uint8_t MIN_DEVIATION_DEGREES = 1;
	static constexpr uint8_t MAX_DEVIATION_DEGREES = 179;
};

unsigned int get_detail_texture_tile_resolution_for_lod(
		const DetailRenderingSettings &settings,
		unsigned int lod_index
);

struct DetailTextureData {
	// 已编码的法线
	StdVector<uint8_t> normals;
	struct Tile {
		uint8_t x;
		uint8_t y;
		uint8_t z;
		uint8_t axis;
	};
	StdVector<Tile> tiles;
	// 可选使用，用于仅获取已编辑 tile 时的部分 tile 数据。
	// 如果为空，表示索引是连续的，因此无需在此存储。
	StdVector<uint32_t> tile_indices;

	inline void clear() {
		normals.clear();
		tiles.clear();
	}
};

// 仅用于保存当前单元格。未针对空间进行优化。`ICellIterator` 的每种实现可能使用更高效的结构。
struct CurrentCellInfo {
	static const unsigned int MAX_TRIANGLES = 5;
	FixedArray<uint32_t, MAX_TRIANGLES> triangle_begin_indices;
	uint32_t triangle_count = 0;
	Vector3i position;
};

class ICellIterator {
public:
	virtual ~ICellIterator() {}
	virtual unsigned int get_count() const = 0;
	virtual bool next(CurrentCellInfo &info) = 0;
	virtual void rewind() = 0;
};

// 对于网格的每个非空单元格，根据单元格中的三角形法线选择轴对齐投影。
// 对单元格内的体素进行采样，从 SDF 计算一块世界空间法线 tile。
// 如果三角形与计算所得法线之间的角度大于 `max_deviation_radians`，
// 法线的朝向将被限制。
// 如果提供了 `out_edited_tiles`，则仅处理包含已编辑体素的 tile。
void compute_detail_texture_data(
		ICellIterator &cell_iterator,
		Span<const Vector3f> mesh_vertices,
		Span<const Vector3f> mesh_normals,
		Span<const int> mesh_indices,
		DetailTextureData &texture_data,
		unsigned int tile_resolution,
		VoxelGenerator &generator,
		const VoxelData *voxel_data,
		Vector3i origin_in_voxels,
		Vector3i size_in_voxels,
		unsigned int lod_index,
		bool octahedral_encoding,
		float max_deviation_radians,
		bool edited_tiles_only
);

struct DetailImages {
#ifdef VOXEL_VIRTUAL_TEXTURE_USE_TEXTURE_ARRAY
	Vector<Ref<Image>> atlas;
#else
	Ref<Image> atlas;
#endif
	Ref<Image> lookup;
};

struct DetailTextures {
#ifdef VOXEL_VIRTUAL_TEXTURE_USE_TEXTURE_ARRAY
	Ref<Texture2DArray> atlas;
#else
	Ref<Texture2D> atlas;
#endif
	Ref<Texture2D> lookup;
};

Ref<Image> store_lookup_to_image(const StdVector<DetailTextureData::Tile> &tiles, Vector3i block_size);

DetailImages store_normalmap_data_to_images(
		const DetailTextureData &data,
		unsigned int tile_resolution,
		Vector3i block_size,
		bool octahedral_encoding
);

// 将法线贴图数据转换为纹理。它们可在着色器中用于应用法线并获得额外的视觉细节。
// 如果渲染器不使用 Vulkan，此操作可能不允许在主线程之外的线程中运行。
DetailTextures store_normalmap_data_to_textures(const DetailImages &data);

struct DetailTextureOutput {
	// 用于平滑体素的法线贴图 atlas。
	// 如果无法从线程创建纹理，则改为返回图像。
	DetailImages images;
	DetailTextures textures;
	// 如果纹理是异步计算的，此值可能为 false。完成后将变为 true（之后不再改变）。
	std::atomic_bool valid = { false };
};

// 给定项目数量，计算一个 2D 正方形网格需要多大尺寸才能容纳它们
inline unsigned int get_square_grid_size_from_item_count(unsigned int item_count) {
	return int(Math::ceil(Math::sqrt(double(item_count))));
}

// 将数据从完全紧凑排列的数组复制到 2D 数组的子区域（因此每行像素在内存中可能不连续）。
void copy_2d_region_from_packed_to_atlased(
		Span<uint8_t> dst,
		const Vector2i dst_size,
		const Span<const uint8_t> src,
		const Vector2i src_size,
		const Vector2i dst_pos,
		const unsigned int item_size_in_bytes
);

} // namespace voxel

#endif // VOXEL_DETAIL_RENDERING_H
