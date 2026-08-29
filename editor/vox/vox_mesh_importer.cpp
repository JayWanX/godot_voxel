#include "vox_mesh_importer.h"
#include "../../constants/voxel_string_names.h"
#include "../../meshers/cubes/voxel_mesher_cubes.h"
#include "../../storage/voxel_buffer.h"
#include "../../storage/voxel_memory_pool.h"
#include "../../streams/vox/vox_data.h"
#include "../../util/dstack.h"
#include "../../util/godot/classes/image_texture.h"
#include "../../util/godot/classes/resource_saver.h"
#include "../../util/godot/classes/standard_material_3d.h"
#include "../../util/godot/core/array.h"
#include "../../util/macros.h"
#include "../../util/math/conv.h"
#include "../../util/memory/memory.h"
#include "../../util/profiling.h"
#include "vox_import_funcs.h"

using namespace voxel::godot;

namespace voxel::magica {

String VoxelVoxMeshImporter::_voxel_get_importer_name() const {
	return "VoxelVoxMeshImporter";
}

String VoxelVoxMeshImporter::_voxel_get_visible_name() const {
	return "VoxelVoxMeshImporter";
}

PackedStringArray VoxelVoxMeshImporter::_voxel_get_recognized_extensions() const {
	PackedStringArray extensions;
	extensions.append("vox");
	return extensions;
}

String VoxelVoxMeshImporter::_voxel_get_preset_name(int p_idx) const {
	return "Default";
}

int VoxelVoxMeshImporter::_voxel_get_preset_count() const {
	return 1;
}

String VoxelVoxMeshImporter::_voxel_get_save_extension() const {
	return "mesh";
}

String VoxelVoxMeshImporter::_voxel_get_resource_type() const {
	return "ArrayMesh";
}

float VoxelVoxMeshImporter::_voxel_get_priority() const {
	// 导入优先级越高，说明该导入器优先于另一个被选用。
	return 0.0;
}

void VoxelVoxMeshImporter::_voxel_get_import_options(
		StdVector<ImportOptionWrapper> &out_options,
		const String &path,
		int preset_index
) const {
	// const VoxelStringNames &sn = VoxelStringNames::get_singleton();
	out_options.push_back(ImportOptionWrapper(PropertyInfo(Variant::BOOL, "store_colors_in_texture"), false));
	out_options.push_back(ImportOptionWrapper(PropertyInfo(Variant::FLOAT, "scale"), 1.f));
	out_options.push_back(ImportOptionWrapper(
			PropertyInfo(Variant::INT, "pivot_mode", PROPERTY_HINT_ENUM, "LowerCorner,SceneOrigin,Center"), 1
	));
}

bool VoxelVoxMeshImporter::_voxel_get_option_visibility(
		const String &path,
		const StringName &option_name,
		const KeyValueWrapper options
) const {
	return true;
}

struct ForEachModelInstanceArgs {
	const Model *model;
	// 枢轴位置，在 MagicaVoxel 中恰好位于中心
	Vector3i position;
	Basis basis;
};

template <typename F>
Error for_each_model_instance_in_scene_graph(const Data &data, int node_id, Transform3D transform, int depth, F f) {
	//
	ERR_FAIL_COND_V(depth > 10, ERR_INVALID_DATA);
	const Node *vox_node = data.get_node(node_id);

	switch (vox_node->type) {
		case Node::TYPE_TRANSFORM: {
			const TransformNode *vox_transform_node = reinterpret_cast<const TransformNode *>(vox_node);
			// 计算子节点的全局变换
			const Transform3D child_trans(
					transform.basis * vox_transform_node->rotation.basis, transform.xform(vox_transform_node->position)
			);
			for_each_model_instance_in_scene_graph(data, vox_transform_node->child_node_id, child_trans, depth + 1, f);
		} break;

		case Node::TYPE_GROUP: {
			const GroupNode *vox_group_node = reinterpret_cast<const GroupNode *>(vox_node);
			for (unsigned int i = 0; i < vox_group_node->child_node_ids.size(); ++i) {
				const int child_node_id = vox_group_node->child_node_ids[i];
				for_each_model_instance_in_scene_graph(data, child_node_id, transform, depth + 1, f);
			}
		} break;

		case Node::TYPE_SHAPE: {
			const ShapeNode *vox_shape_node = reinterpret_cast<const ShapeNode *>(vox_node);
			ForEachModelInstanceArgs args;
			args.model = &data.get_model(vox_shape_node->model_id);
			args.position = math::round_to_int(transform.origin);
			args.basis = transform.basis;
			f(args);
		} break;

		default:
			ERR_FAIL_V(ERR_INVALID_DATA);
			break;
	}

	return OK;
}

template <typename F>
void for_each_model_instance(const Data &vox_data, F f) {
	if (vox_data.get_model_count() == 0) {
		return;
	}
	if (vox_data.get_root_node_id() == -1) {
		// 没有场景图
		ForEachModelInstanceArgs args;
		args.model = &vox_data.get_model(0);
		// 放到中心以匹配 MagicaVoxel 的做法
		args.position = args.model->size / 2;
		args.basis = Basis();
		f(args);
		return;
	}
	for_each_model_instance_in_scene_graph(vox_data, vox_data.get_root_node_id(), Transform3D(), 0, f);
}

// 找到相交或接触的模型，将它们的体素合并到同一个网格中，对结果做网格化，然后合并网格。

struct ModelInstance {
	// 已烘焙旋转的模型
	UniquePtr<voxel::VoxelBuffer> voxels;
	// 最低角位置
	Vector3i position;
};

void extract_model_instances(const Data &vox_data, StdVector<ModelInstance> &out_instances) {
	VOXEL_DSTACK();
	// 收集所有模型并烘焙它们的旋转
	for_each_model_instance(vox_data, [&out_instances](ForEachModelInstanceArgs args) {
		ERR_FAIL_COND(args.model == nullptr);
		const Model &model = *args.model;

		Span<const uint8_t> src_color_indices;
		Vector3i dst_size = model.size;

		// 使用临时副本旋转数据
		StdVector<uint8_t> temp_voxels;

		if (args.basis == Basis()) {
			// 没有变换
			src_color_indices = to_span_const(model.color_indexes);
		} else {
			math::OrthoBasis basis;
			basis.x = to_vec3i(args.basis.get_column(Vector3::AXIS_X));
			basis.y = to_vec3i(args.basis.get_column(Vector3::AXIS_Y));
			basis.z = to_vec3i(args.basis.get_column(Vector3::AXIS_Z));
			temp_voxels.resize(model.color_indexes.size());
			dst_size =
					transform_3d_array_zxy(to_span_const(model.color_indexes), to_span(temp_voxels), model.size, basis);
			src_color_indices = to_span_const(temp_voxels);
		}

		// TODO 优化：为 VoxelBuffer 实现变换，这样就能避免使用临时副本。
		// 还没这么做，因为 VoxelBuffer 也有元数据，而且 `transform_3d_array_zxy` 函数只对
		// 数组有效。
		UniquePtr<voxel::VoxelBuffer> voxels = make_unique_instance<voxel::VoxelBuffer>(voxel::VoxelBuffer::ALLOCATOR_DEFAULT);
		voxels->create(dst_size);
		voxels->decompress_channel(voxel::VoxelBuffer::CHANNEL_COLOR);

		Span<uint8_t> dst_color_indices;
		ERR_FAIL_COND(!voxels->get_channel_as_bytes(voxel::VoxelBuffer::CHANNEL_COLOR, dst_color_indices));

		CRASH_COND(src_color_indices.size() != dst_color_indices.size());
		memcpy(dst_color_indices.data(), src_color_indices.data(), dst_color_indices.size() * sizeof(uint8_t));

		ModelInstance mi;
		mi.voxels = std::move(voxels);
		mi.position = args.position - mi.voxels->get_size() / 2;
		out_instances.push_back(std::move(mi));
	});
}

bool make_single_voxel_grid(Span<const ModelInstance> instances, Vector3i &out_origin, voxel::VoxelBuffer &out_voxels) {
	// 确定总大小
	const ModelInstance &first_instance = instances[0];
	Box3i bounding_box(first_instance.position, first_instance.voxels->get_size());
	for (unsigned int instance_index = 1; instance_index < instances.size(); ++instance_index) {
		const ModelInstance &mi = instances[instance_index];
		bounding_box.merge_with(Box3i(mi.position, mi.voxels->get_size()));
	}

	// 额外的健全性检查
	// 3 GB
	const size_t limit = 3'000'000'000ull;
	const size_t volume = Vector3iUtil::get_volume_u64(bounding_box.size);
	ERR_FAIL_COND_V_MSG(
			volume > limit,
			false,
			String("Vox data is too big to be meshed as a single mesh ({0}: {0} bytes)")
					.format(varray(bounding_box.size, VOXEL_SIZE_T_TO_VARIANT(volume)))
	);

	out_voxels.create(bounding_box.size + Vector3iUtil::create(VoxelMesherCubes::PADDING * 2));
	out_voxels.set_channel_depth(voxel::VoxelBuffer::CHANNEL_COLOR, voxel::VoxelBuffer::DEPTH_8_BIT);
	out_voxels.decompress_channel(voxel::VoxelBuffer::CHANNEL_COLOR);

	for (unsigned int instance_index = 0; instance_index < instances.size(); ++instance_index) {
		const ModelInstance &mi = instances[instance_index];
		ERR_FAIL_COND_V(mi.voxels == nullptr, false);
		out_voxels.copy_channel_from(
				*mi.voxels,
				Vector3i(),
				mi.voxels->get_size(),
				mi.position - bounding_box.position + Vector3iUtil::create(VoxelMesherCubes::PADDING),
				voxel::VoxelBuffer::CHANNEL_COLOR
		);
	}

	out_origin = bounding_box.position;
	return true;
}

Error VoxelVoxMeshImporter::_voxel_import(
		const String &p_source_file,
		const String &p_save_path,
		const KeyValueWrapper p_options,
		StringListWrapper out_platform_variants,
		StringListWrapper out_gen_files
) const {
	//
	const bool p_store_colors_in_textures = p_options.get("store_colors_in_texture");
	const float p_scale = p_options.get("scale");
	const int p_pivot_mode = p_options.get("pivot_mode");

	ERR_FAIL_INDEX_V(p_pivot_mode, PIVOT_MODES_COUNT, ERR_INVALID_PARAMETER);

	Data vox_data;
	const Error load_err = vox_data.load_from_file(p_source_file);
	ERR_FAIL_COND_V(load_err != OK, load_err);

	// 获取颜色调色板
	Ref<VoxelColorPalette> palette;
	palette.instantiate();
	for (unsigned int i = 0; i < vox_data.get_palette().size(); ++i) {
		const Color8 color = vox_data.get_palette()[i];
		palette->set_color8(i, color);
	}

	if (vox_data.get_model_count() == 0) {
		// wut
		return ERR_CANT_CREATE;
	}

	Ref<Image> atlas;
	Ref<Mesh> mesh;
	StdVector<unsigned int> surface_index_to_material;
	{
		StdVector<ModelInstance> model_instances;
		extract_model_instances(vox_data, model_instances);

		// 从这里开始我们不再需要 vox 数据，因此可以释放一些内存
		vox_data.clear();

		// TODO 优化：这种方法占用大量内存，在边界框较大的场景上可能会失败。
		// 一个变通方法是以块为单位增量地对场景进行网格化，超过 256 左右就放弃贪心网格化。
		Vector3i bounding_box_origin;
		voxel::VoxelBuffer voxels(voxel::VoxelBuffer::ALLOCATOR_DEFAULT);
		const bool single_grid_succeeded =
				make_single_voxel_grid(to_span_const(model_instances), bounding_box_origin, voxels);
		ERR_FAIL_COND_V(!single_grid_succeeded, ERR_CANT_CREATE);

		// 我们不再需要这些
		model_instances.clear();

		Ref<VoxelMesherCubes> mesher;
		mesher.instantiate();
		mesher->set_color_mode(VoxelMesherCubes::COLOR_MESHER_PALETTE);
		mesher->set_palette(palette);
		mesher->set_greedy_meshing_enabled(true);
		mesher->set_store_colors_in_texture(p_store_colors_in_textures);

		Vector3 offset;
		switch (p_pivot_mode) {
			case PIVOT_LOWER_CORNER:
				break;
			case PIVOT_SCENE_ORIGIN:
				offset = bounding_box_origin;
				break;
			case PIVOT_CENTER:
				offset = -((voxels.get_size() - Vector3iUtil::create(1)) / 2);
				break;
			default:
				ERR_FAIL_V(ERR_BUG);
				break;
		};

		mesh = build_mesh(voxels, **mesher, surface_index_to_material, atlas, p_scale, offset);
		// 释放大型临时内存以腾出空间。
		// 这是一个变通方法，因为 VoxelBuffer 默认使用这个池，但它不适合当前的用例。
		// 最终我们应该避免在这里使用这个池。
		VoxelMemoryPool::get_singleton().clear_unused_blocks();
	}

	if (mesh.is_null()) {
		// wut
		return ERR_CANT_CREATE;
	}

	// 保存图集
	// TODO 由于 https://github.com/godotengine/godot/issues/51163，无法单独保存图集
	// 相反，我像 ResourceImporterScene 那样做：我把它们以未压缩的形式放在材质内部……
	/*String atlas_path;
		if (atlas.is_valid()) {
			atlas_path = String("{0}.atlas{1}.stex").format(varray(p_save_path, model_index));
			const Error save_stex_err = save_stex(atlas, atlas_path, false, 0, true, true, true);
			ERR_FAIL_COND_V_MSG(save_stex_err != OK, save_stex_err,
					String("Failed to save {0}").format(varray(atlas_path)));
		}*/

	// DEBUG
	// if (atlas.is_valid()) {
	// 	atlas->save_png(String("debug_atlas{0}.png").format(varray(model_index)));
	// }

	FixedArray<Ref<StandardMaterial3D>, 2> materials;
	for (unsigned int i = 0; i < materials.size(); ++i) {
		Ref<StandardMaterial3D> &mat = materials[i];
		mat.instantiate();
		mat->set_roughness(1.f);
		if (!p_store_colors_in_textures) {
			// 这种情况下我们把颜色存储在顶点中
			mat->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		}
	}
	materials[1]->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);

	// 分配材质
	if (p_store_colors_in_textures) {
		// 目前无法共享材质，因为每个图集都专属于其网格
		for (unsigned int surface_index = 0; surface_index < surface_index_to_material.size(); ++surface_index) {
			const unsigned int material_index = surface_index_to_material[surface_index];
			CRASH_COND(material_index >= materials.size());
			Ref<StandardMaterial3D> material = materials[material_index]->duplicate();
			if (atlas.is_valid()) {
				// TODO 我真的必须为了导入而把这个纹理重新加载回内存和渲染器吗？？
				// Ref<Texture> texture = ResourceLoader::load(atlas_path);
				// TODO 这是一个变通方法，它不应该是 ImageTexture……
				// 参见前面的代码，我找不到任何方法引用单独的 StreamTexture。
				Ref<ImageTexture> texture = ImageTexture::create_from_image(atlas);
				material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, texture);
				material->set_texture_filter(StandardMaterial3D::TEXTURE_FILTER_NEAREST);
			}
			mesh->surface_set_material(surface_index, material);
		}
	} else {
		for (unsigned int surface_index = 0; surface_index < surface_index_to_material.size(); ++surface_index) {
			const unsigned int material_index = surface_index_to_material[surface_index];
			CRASH_COND(material_index >= materials.size());
			mesh->surface_set_material(surface_index, materials[material_index]);
		}
	}

	// 保存网格
	{
		VOXEL_PROFILE_SCOPE();
		String mesh_save_path = String("{0}.mesh").format(varray(p_save_path));
		const Error mesh_save_err = save_resource(mesh, mesh_save_path, ResourceSaver::FLAG_NONE);
		ERR_FAIL_COND_V_MSG(
				mesh_save_err != OK, mesh_save_err, String("Failed to save {0}").format(varray(mesh_save_path))
		);
	}

	return OK;
}

} // namespace voxel::magica
