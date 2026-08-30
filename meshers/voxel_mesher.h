#ifndef VOXEL_MESHER_H
#define VOXEL_MESHER_H

#include "../constants/cube_tables.h"
#include "../util/containers/fixed_array.h"
#include "../util/containers/span.h"
#include "../util/containers/std_vector.h"
#include "../util/godot/classes/image.h"
#include "../util/godot/classes/mesh.h"
#include "../util/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class ShaderMaterial)

namespace voxel {

namespace godot {
class VoxelBuffer;
}

class VoxelBuffer;
class VoxelGenerator;
class VoxelData;

// 从体素生成网格的算法的基类。
class VoxelMesher : public Resource {
	GDCLASS(VoxelMesher, Resource)
public:
	struct Input {
		// 用作主要数据源的体素。
		const VoxelBuffer &voxels;
		// 使用 LOD 时，某些网格生成器可以使用生成器和已编辑的体素来优化结果。
		// 如果未提供，网格生成器将只使用 `voxels`。
		VoxelGenerator *generator = nullptr;
		// 进行深度采样时需要数据块的原点。
		Vector3i origin_in_voxels;
		// LOD 索引。0 表示最高细节，1 表示一半细节，以此类推。
		uint8_t lod_index = 0;
		// 如果为 true，则需要碰撞信息。
		// 有时它不会改变任何东西，因为渲染网格可以用作碰撞体，
		// 但在其他设置下它可能不同，并将在 `collision_surface` 中返回。
		bool collision_hint = false;
		// 如果为 true，则告知网格生成器该网格将用于可变细节层级的上下文。
		// 例如，过渡网格将根据此信息决定是否生成（覆盖网格生成器设置）。
		bool lod_hint = false;
		// 如果为 true，网格生成器可以收集一些额外的信息，这些信息有助于加快细节纹理
		// 烘焙。取决于具体的网格生成器。
		bool detail_texture_hint = false;
	};

	struct Output {
		struct Surface {
			Array arrays;
			uint16_t material_index = 0;
		};
		StdVector<Surface> surfaces;
		FixedArray<StdVector<Surface>, Cube::SIDE_COUNT> transition_surfaces;
		Mesh::PrimitiveType primitive_type = Mesh::PRIMITIVE_TRIANGLES;
		// 用于创建 Godot 网格资源的标志
		uint32_t mesh_flags = 0;

		struct CollisionSurface {
			StdVector<Vector3f> positions;
			StdVector<int> indices;
			// 如果 >= 0，则碰撞表面实际上可能是从渲染网格第一个表面的数组子集中选取的
			//（它可能从索引 0 开始）。
			// 在过渡网格与主网格合并时使用。
			int32_t submesh_vertex_end = -1;
			int32_t submesh_index_end = -1;
		};
		CollisionSurface collision_surface;

		Array shadow_occluder;

		// 可用于存储着色器正确渲染网格所需的额外信息
		//（目前仅在 cubes 网格生成器烘焙颜色时使用）
		Ref<Image> atlas_image;
	};

	// 检查表面数组是否为空
	static bool is_mesh_empty(const StdVector<Output::Surface> &surfaces);

	// 该方法可以同时从多个线程调用。请确保成员变量受保护或是线程局部的。
	virtual void build(Output &output, const Input &voxels);

	// 根据给定的体素构建网格。此函数已简化，供脚本 API 使用。
	Ref<Mesh> build_mesh(const VoxelBuffer &voxels, TypedArray<Material> materials, Dictionary additional_data);

	// TODO 重命名 "positive" 和 "negative" 填充

	// 获取在网格区域周围需要访问的邻居体素数，朝负轴方向。
	// 如果不满足此要求，网格生成器可能在边缘产生接缝，或报错
	unsigned int get_minimum_padding() const;

	// 获取在网格区域周围需要访问的邻居体素数，朝正轴方向。
	// 如果不满足此要求，网格生成器可能在边缘产生接缝，或报错
	unsigned int get_maximum_padding() const;

	// 获取此网格生成器在当前配置下能够使用的通道。
	// 以位掩码形式返回，其中通道索引对应位位置。
	virtual int get_used_channels_mask() const {
		return 0;
	}

	// 如果此网格生成器支持在多个细节层级上生成体素数据，则返回 true。
	virtual bool supports_lod() const {
		return true;
	}

	// 某些网格生成器可以自行提供材质。索引可能来自构建输出。如果索引未分配材质则返回 null。
	// 如果此处未提供，则可能使用默认材质。如果索引越界可能会产生错误。
	virtual Ref<Material> get_material_by_index(unsigned int i) const;
	// 获取最高材质索引 + 1
	virtual unsigned int get_material_index_count() const;

#ifdef TOOLS_ENABLED
	// 如果网格生成器有问题，此方法可能返回消息，以便向用户显示。
	virtual void get_configuration_warnings(PackedStringArray &out_warnings) const {}
#endif

	// 如果网格生成器为网格碰撞生成特定数据（可在
	// `CollisionSurface` 中找到），则返回 `true`。
	// 如果返回 `false`，则渲染网格可以用作碰撞体。
	virtual bool is_generating_collision_surface() const {
		return false;
	}

	// 获取在可变细节层级下渲染由此网格生成器生成的网格时要使用的特殊默认材质。
	// 如果为 null，可以使用标准材质或默认的 Godot 着色器。
	// 这主要是为了提供一个看起来不错的默认着色器。如有需要，仍期望用户自行调整。
	// 此类材质不应被修改。
	virtual Ref<ShaderMaterial> get_default_lod_material() const;

protected:
	Ref<Mesh> _b_build_mesh(Ref<godot::VoxelBuffer> voxels, TypedArray<Material> materials, Dictionary additional_data);
	static void _bind_methods();

	void set_padding(int minimum, int maximum);

private:
	// 在构造函数中设置，此后永不更改。
	unsigned int _minimum_padding = 0;
	unsigned int _maximum_padding = 0;
};

} // namespace voxel

#endif // VOXEL_MESHER_H
