#if !defined(VOXEL_MESHER_CUBES_H)
#define VOXEL_MESHER_CUBES_H

#include "../../util/math/vector2f.h"
#include "../../util/math/vector3f.h"
#include "../../util/thread/rw_lock.h"
#include "../voxel_mesher.h"
#include "voxel_color_palette.h"

#include <vector>

namespace voxel {

// 一个只生成彩色立方体的超级简单的网格生成器
class VoxelMesherCubes : public VoxelMesher {
	GDCLASS(VoxelMesherCubes, VoxelMesher)
public:
	static const unsigned int PADDING = 1;

	enum Materials { //
		MATERIAL_OPAQUE = 0,
		MATERIAL_TRANSPARENT,
		MATERIAL_COUNT
	};

	// 说明如何解释体素颜色数据
	enum ColorMode {
		// 体素值将被视为 RGBA 颜色，各分量位深相等
		COLOR_RAW = 0,
		// 体素值将映射到此网格生成器上指定的调色板中的 32 位颜色
		COLOR_MESHER_PALETTE,
		// 体素值将直接复制到顶点数组，
		// 以便着色器可以选择正确的颜色。
		// 限制：目前此模式只能使用一种材质。
		COLOR_SHADER_PALETTE,

		COLOR_MODE_COUNT
	};

	VoxelMesherCubes();
	~VoxelMesherCubes();

	// 将体素数据构建成网格
	void build(VoxelMesher::Output &output, const VoxelMesher::Input &input) override;

	// 是否启用贪心网格化
	void set_greedy_meshing_enabled(bool enable);
	bool is_greedy_meshing_enabled() const;

	// 体素颜色解释方式
	void set_color_mode(ColorMode mode);
	ColorMode get_color_mode() const;

	// 使用的调色板
	void set_palette(Ref<VoxelColorPalette> palette);
	Ref<VoxelColorPalette> get_palette() const;

	// TODO: Resource::duplicate() 无法被覆写。
	// 这会导致性能下降，甚至可能出现意外行为
	// 	Ref<Resource> duplicate(bool p_subresources = false) const override;

	// 网格生成器使用的通道掩码
	int get_used_channels_mask() const override;

	// 是否将颜色存入纹理
	void set_store_colors_in_texture(bool enable);
	bool get_store_colors_in_texture() const;

	bool supports_lod() const override {
		return true;
	}

	// 设置或获取指定索引的材质
	void set_material_by_index(Materials id, Ref<Material> material);
	Ref<Material> get_material_by_index(unsigned int i) const override;
	unsigned int get_material_index_count() const override;

	// 从图像生成体素网格
	static Ref<Mesh> generate_mesh_from_image(Ref<Image> image, float voxel_size);

	// 结构体

	// 使用 std::vector，因为它们使此网格生成器比 Godot 的 Vector 快一倍。
	// 原因见：https://github.com/godotengine/godot/issues/24731
	struct Arrays {
		StdVector<Vector3f> positions;
		StdVector<Vector3f> normals;
		StdVector<Color> colors;
		StdVector<Vector2f> uvs;
		StdVector<int> indices;

		void clear() {
			positions.clear();
			normals.clear();
			colors.clear();
			uvs.clear();
			indices.clear();
		}
	};

	struct GreedyAtlasData {
		struct ImageInfo {
			unsigned int first_color_index;
			unsigned int first_vertex_index; // 来自一个四边形
			unsigned int size_x;
			unsigned int size_y;
			unsigned int surface_index;
		};
		StdVector<Color8> colors;
		StdVector<ImageInfo> images;

		void clear() {
			colors.clear();
			images.clear();
		}
	};

private:
	void _b_set_opaque_material(Ref<Material> material);
	Ref<Material> _b_get_opaque_material() const;

	void _b_set_transparent_material(Ref<Material> material);
	Ref<Material> _b_get_transparent_material() const;

	static void _bind_methods();

	struct Parameters {
		ColorMode color_mode = COLOR_RAW;
		Ref<VoxelColorPalette> palette;
		bool greedy_meshing = true;
		bool store_colors_in_texture = false;
	};

	struct Cache {
		FixedArray<Arrays, MATERIAL_COUNT> arrays_per_material;
		StdVector<uint8_t> mask_memory_pool;
		GreedyAtlasData greedy_atlas_data;
	};

	// 参数
	Parameters _parameters;
	RWLock _parameters_lock;

	FixedArray<Ref<Material>, MATERIAL_COUNT> _materials;

	// 工作缓存
	static Cache &get_tls_cache();
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelMesherCubes::ColorMode);
VARIANT_ENUM_CAST(voxel::VoxelMesherCubes::Materials);

#endif // VOXEL_MESHER_CUBES_H
