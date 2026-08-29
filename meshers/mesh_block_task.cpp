#include "mesh_block_task.h"
#include "../storage/voxel_data.h"
#include "../terrain/voxel_mesh_block.h"
#include "../util/dstack.h"
#include "../util/godot/classes/mesh.h"
#include "../util/io/log.h"
#include "../util/math/conv.h"
#include "../util/profiling.h"
// #include "../util/string/format.h" // Debug
#include "../engine/voxel_engine.h"

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
#include "../engine/detail_rendering/render_detail_texture_task.h"
#include "../meshers/transvoxel/transvoxel_cell_iterator.h"
#include "../meshers/transvoxel/voxel_mesher_transvoxel.h"
#endif

#ifdef VOXEL_ENABLE_GPU
#include "../generators/generate_block_gpu_task.h"
#endif

namespace voxel {

namespace {

struct CubicAreaInfo {
	int edge_size; // 以数据块为单位
	int mesh_block_size_factor;
	unsigned int anchor_buffer_index;

	inline bool is_valid() const {
		return edge_size != 0;
	}
};

CubicAreaInfo get_cubic_area_info_from_size(unsigned int size) {
	// 确定数据块立方体的尺寸
	int edge_size;
	int mesh_block_size_factor;
	switch (size) {
		case 3 * 3 * 3:
			edge_size = 3;
			mesh_block_size_factor = 1;
			break;
		case 4 * 4 * 4:
			edge_size = 4;
			mesh_block_size_factor = 2;
			break;
		default:
			VOXEL_PRINT_ERROR("Unsupported block count");
			return CubicAreaInfo{ 0, 0, 0 };
	}

	// 选择锚点数据块，通常位于立方体的中心部分（该数据块必须是有效的）
	const unsigned int anchor_buffer_index = edge_size * edge_size + edge_size + 1;

	return { edge_size, mesh_block_size_factor, anchor_buffer_index };
}

// 接收一组数据块，并将其视为以我们想生成网格的区域为中心的立方体数据块。
// 中央数据块的体素会被复制，侧面数据块的部分体素也会被复制，
// 这样我们就能得到一个临时缓冲区，其中包含足够的邻居，使网格生成器无需进行边界检查。
void copy_block_and_neighbors(
		Span<std::shared_ptr<VoxelBuffer>> blocks,
		VoxelBuffer &dst,
		int min_padding,
		int max_padding,
		int channels_mask,
		Ref<VoxelGenerator> generator,
		const VoxelData &voxel_data,
		uint8_t lod_index,
		Vector3i mesh_block_pos,
		StdVector<Box3i> *out_boxes_to_generate,
		Vector3i *out_origin_in_voxels
) {
	VOXEL_DSTACK();
	VOXEL_PROFILE_SCOPE();

	// 提取需要的通道列表
	const SmallVector<uint8_t, VoxelBuffer::MAX_CHANNELS> channels = VoxelBuffer::mask_to_channels_list(channels_mask);

	// 确定数据块立方体的尺寸
	const CubicAreaInfo area_info = get_cubic_area_info_from_size(blocks.size());
	ERR_FAIL_COND(!area_info.is_valid());

	std::shared_ptr<VoxelBuffer> &central_buffer = blocks[area_info.anchor_buffer_index];
	ERR_FAIL_COND_MSG(central_buffer == nullptr && generator.is_null(), "Central buffer must be valid");
	if (central_buffer != nullptr) {
		ERR_FAIL_COND_MSG(
				Vector3iUtil::all_members_equal(central_buffer->get_size()) == false, "Central buffer must be cubic"
		);
	}
	const int data_block_size = voxel_data.get_block_size();
	const int mesh_block_size = data_block_size * area_info.mesh_block_size_factor;
	const int padded_mesh_block_size = mesh_block_size + min_padding + max_padding;

	const VoxelFormat voxel_format = voxel_data.get_format();
	dst.create(Vector3iUtil::create(padded_mesh_block_size), &voxel_format);

	const Box3i bounds_in_voxels_lod0 = voxel_data.get_bounds();
	const Box3i bounds_in_voxels(bounds_in_voxels_lod0.position >> lod_index, bounds_in_voxels_lod0.size >> lod_index);

	// TODO 在仅依赖缓存的 terrain 中，我们绝不应考虑从这里生成体素。
	// VoxelTerrain 就是这种情况，它现在正在做不必要的盒体减法计算……

	const Vector3i min_pos = -Vector3iUtil::create(min_padding);
	const Vector3i max_pos = Vector3iUtil::create(mesh_block_size + max_padding);

	const Vector3i origin_in_voxels_without_padding =
			mesh_block_pos * (area_info.mesh_block_size_factor * data_block_size);
	const Vector3i origin_in_voxels = origin_in_voxels_without_padding - Vector3iUtil::create(min_padding);
	const Vector3i origin_in_voxels_lod0 = origin_in_voxels << lod_index;

	// 这些盒体最初相对于最小数据块的最近角落。
	// TODO 可考虑使用临时分配器（或 SmallVector?）
	StdVector<Box3i> boxes_to_generate;
	const Box3i mesh_data_box = Box3i::from_min_max(min_pos, max_pos);
	if (contains(blocks.to_const(), std::shared_ptr<VoxelBuffer>())) {
		const Box3i bounds_local(bounds_in_voxels.position - origin_in_voxels_without_padding, bounds_in_voxels.size);
		const Box3i box = mesh_data_box.clipped(bounds_local); // 防止在固定边界之外生成
		if (!box.is_empty()) {
			boxes_to_generate.push_back(box);
		}
	}

	{
		// TODO 以下逻辑也许可以简化并移入 VoxelData。
		// 我们只是在给定区域内采样或生成数据。

		const Vector3i data_block_pos0 = mesh_block_pos * area_info.mesh_block_size_factor;
		SpatialLock3D::Read srlock(
				voxel_data.get_spatial_lock(lod_index),
				BoxBounds3i(
						data_block_pos0 - Vector3i(1, 1, 1), data_block_pos0 + Vector3iUtil::create(area_info.edge_size)
				)
		);

		// 使用 ZXY 约定重建位置，以保持线程加锁的一致性
		unsigned int block_index = 0;
		for (int z = -1; z < area_info.edge_size - 1; ++z) {
			for (int x = -1; x < area_info.edge_size - 1; ++x) {
				for (int y = -1; y < area_info.edge_size - 1; ++y) {
					const Vector3i offset = data_block_size * Vector3i(x, y, z);
					const std::shared_ptr<VoxelBuffer> &src = blocks[block_index];
					++block_index;

					if (src == nullptr) {
						continue;
					}

					const Vector3i src_min = min_pos - offset;
					const Vector3i src_max = max_pos - offset;

					for (const uint8_t channel_index : channels) {
						dst.copy_channel_from(*src, src_min, src_max, Vector3i(), channel_index);
					}

					if (boxes_to_generate.size() > 0) {
						// 从待生成区域中减去已编辑的盒体
						// TODO 这种方法允许在必要时批量处理盒体，
						// 但直接对每个裁剪后的盒体都这样做是不是更好？
						VOXEL_PROFILE_SCOPE_NAMED("Box subtract");
						const unsigned int input_count = boxes_to_generate.size();
						const Box3i block_box =
								Box3i(offset, Vector3iUtil::create(data_block_size)).clipped(mesh_data_box);

						for (unsigned int box_index = 0; box_index < input_count; ++box_index) {
							const Box3i box = boxes_to_generate[box_index];
							// 剩余盒体会追加到列表末尾
							box.difference_to_vec(block_box, boxes_to_generate);
#ifdef DEBUG_ENABLED
							// 差集操作应往向量中添加盒体，而不是移除任何盒体
							CRASH_COND(box_index >= boxes_to_generate.size());
#endif
						}

						// 移除输入盒体
						boxes_to_generate.erase(boxes_to_generate.begin(), boxes_to_generate.begin() + input_count);
					}
				}
			}
		}
	}

	// 撤销填充，回到正确的缓冲区坐标
	for (Box3i &box : boxes_to_generate) {
		box.position += Vector3iUtil::create(min_padding);
	}

	if (out_origin_in_voxels != nullptr) {
		*out_origin_in_voxels = origin_in_voxels_lod0;
	}

	if (out_boxes_to_generate != nullptr) {
		// 将生成工作委托给调用方
		append_array(*out_boxes_to_generate, boxes_to_generate);

	} else {
		// 用 CPU 上生成的体素补全数据
		VOXEL_PROFILE_SCOPE_NAMED("Generate");
		VoxelBuffer generated_voxels(VoxelBuffer::ALLOCATOR_POOL);

#ifdef VOXEL_ENABLE_MODIFIERS
		const VoxelModifierStack &modifiers = voxel_data.get_modifiers();
#endif

		for (const Box3i &box : boxes_to_generate) {
			VOXEL_PROFILE_SCOPE_NAMED("Box");
			// print_line(String("size={0}").format(varray(box.size.to_vec3())));
			generated_voxels.create(box.size, &voxel_format);
			// generated_voxels.set_voxel_f(2.0f, box.size.x / 2, box.size.y / 2, box.size.z / 2,
			// VoxelBuffer::CHANNEL_SDF);
			VoxelGenerator::VoxelQueryData q{ generated_voxels,
											  (box.position << lod_index) + origin_in_voxels_lod0,
											  lod_index };

			if (generator.is_valid()) {
				generator->generate_block(q);
			}
#ifdef VOXEL_ENABLE_MODIFIERS
			modifiers.apply(q.voxel_buffer, AABB(q.origin_in_voxels, q.voxel_buffer.get_size() << lod_index));
#endif

			for (const uint8_t channel_index : channels) {
				dst.copy_channel_from(
						generated_voxels, Vector3i(), generated_voxels.get_size(), box.position, channel_index
				);
			}
		}
	}
}

} // namespace

Ref<ArrayMesh> build_mesh(
		Span<const VoxelMesher::Output::Surface> surfaces,
		Mesh::PrimitiveType primitive,
		int flags,
		// 此向量将表面索引到它们使用的材质（如果某个表面使用了材质但为空，
		// 则不会被添加到网格中）
		StdVector<uint16_t> &mesh_material_indices
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(mesh_material_indices.size() == 0);

	Ref<ArrayMesh> mesh;

	for (unsigned int i = 0; i < surfaces.size(); ++i) {
		const VoxelMesher::Output::Surface &surface = surfaces[i];
		Array arrays = surface.arrays;

		if (arrays.is_empty()) {
			continue;
		}

		CRASH_COND(arrays.size() != Mesh::ARRAY_MAX);
		if (!voxel::godot::is_surface_triangulated(arrays)) {
			continue;
		}

		if (mesh.is_null()) {
			mesh.instantiate();
		}

		// TODO 使用 `add_surface`，在 Tracy 中测量后它大约快 20%（不过我们可以看看 Godot 4 是否
		// 表现相同）
		mesh->add_surface_from_arrays(primitive, arrays, Array(), Dictionary(), flags);

		mesh_material_indices.push_back(surface.material_index);
	}

	// 用于突出显示顶点共享的调试代码
	/*if (mesh->get_surface_count() > 0) {
		Array wireframe_surface = generate_debug_seams_wireframe_surface(mesh, 0);
		if (wireframe_surface.size() > 0) {
			const int wireframe_surface_index = mesh->get_surface_count();
			mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, wireframe_surface);
			Ref<SpatialMaterial> line_material;
			line_material.instance();
			line_material->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
			line_material->set_albedo(Color(1.0, 0.0, 1.0));
			mesh->surface_set_material(wireframe_surface_index, line_material);
		}
	}*/

	if (mesh.is_valid() && voxel::godot::is_mesh_empty(**mesh)) {
		mesh = Ref<Mesh>();
	}

	return mesh;
}

Ref<ArrayMesh> build_mesh(Array surface) {
	if (surface.is_empty()) {
		return Ref<ArrayMesh>();
	}
	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, surface);
	return mesh;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
std::atomic_int g_debug_mesh_tasks_count = { 0 };
} // namespace

MeshBlockTask::MeshBlockTask() : _voxels(VoxelBuffer::ALLOCATOR_POOL) {
	++g_debug_mesh_tasks_count;
}

MeshBlockTask::~MeshBlockTask() {
	--g_debug_mesh_tasks_count;
}

int MeshBlockTask::debug_get_running_count() {
	return g_debug_mesh_tasks_count;
}

void MeshBlockTask::run(voxel::ThreadedTaskContext &ctx) {
	VOXEL_DSTACK();
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT(meshing_dependency != nullptr);
#ifdef DEBUG_ENABLED
	VOXEL_ASSERT_RETURN_MSG(
			meshing_dependency->mesher.is_valid(),
			"Meshing task started without a mesher. Maybe missing on the terrain node?"
	);
#endif

	// TODO 使用 Transvoxel 和固定边界 terrain 时，"边界悬崖"不会出现在负方向上。
	// 这是由于实现细节：Transvoxel 只为每个 2^3 单元的内部和正方向部分生成网格。
	// 如果期望出现悬崖，我们可以强制 terrain 在边界外多请求 1 个数据块的网格，但这
	// 有点浪费。相反，我们可以动态调整负方向的填充，以额外包含那些边界体素。
	// 不幸的是，当使用诸如细节渲染等对位置敏感的特性时，这可能会产生副作用。
	// 这还引发另一个担忧：如果高度在垂直方向上受限而水平方向上不受限，
	// 典型的 terrain 最终会在底部产生一个巨大的朝下表面，因为边界外数据块的默认值是空气。
	// 我们还必须以某种方式提供一种方法来设置这些区域默认是什么……

#ifdef VOXEL_ENABLE_GPU
	if (_stage == 0)
#endif
	{
		VOXEL_ASSERT(data != nullptr);
		const VoxelFormat format = data->get_format();
		format.configure_buffer(_voxels);
	}

#ifdef VOXEL_ENABLE_GPU
	if (block_generation_use_gpu) {
		if (_stage == 0) {
			gather_voxels_gpu(ctx);
		}
		if (_stage == 1) {
			GenerateBlockGPUTaskResult::convert_to_voxel_buffer(to_span(_gpu_generation_results), _voxels);
			_stage = 2;
		}
		if (_stage == 2) {
			build_mesh();
		}
	} else
#endif
	{
		gather_voxels_cpu();
		build_mesh();
	}
}

#ifdef VOXEL_ENABLE_GPU

void MeshBlockTask::gather_voxels_gpu(voxel::ThreadedTaskContext &ctx) {
	VOXEL_ASSERT(meshing_dependency != nullptr);
	VOXEL_ASSERT(data != nullptr);

	Ref<VoxelMesher> mesher = meshing_dependency->mesher;
	const unsigned int min_padding = mesher->get_minimum_padding();
	const unsigned int max_padding = mesher->get_maximum_padding();

	StdVector<Box3i> boxes_to_generate;
	Vector3i origin_in_voxels;

	copy_block_and_neighbors(
			to_span(blocks, blocks_count),
			_voxels,
			min_padding,
			max_padding,
			mesher->get_used_channels_mask(),
			meshing_dependency->generator,
			*data,
			lod_index,
			mesh_block_position,
			&boxes_to_generate,
			&origin_in_voxels
	);

	if (boxes_to_generate.size() == 0) {
		_stage = 2;
		return;
	}

	Ref<VoxelGenerator> generator = meshing_dependency->generator;
	ERR_FAIL_COND(generator.is_null());

	VoxelGenerator::VoxelQueryData generator_query{ _voxels, origin_in_voxels, lod_index };
	if (generator->generate_broad_block(generator_query)) {
		_stage = 2;
		return;
	}

	std::shared_ptr<ComputeShader> generator_shader = generator->get_block_rendering_shader();
	ERR_FAIL_COND(generator_shader == nullptr);

	GenerateBlockGPUTask *gpu_task = VOXEL_NEW(GenerateBlockGPUTask);
	gpu_task->boxes_to_generate = std::move(boxes_to_generate);
	gpu_task->generator_shader = generator_shader;
	gpu_task->generator_shader_params = generator->get_block_rendering_shader_parameters();
	gpu_task->generator_shader_outputs = generator->get_block_rendering_shader_outputs();
	gpu_task->lod_index = lod_index;
	gpu_task->origin_in_voxels = origin_in_voxels;
	gpu_task->consumer_task = this;

#ifdef VOXEL_ENABLE_MODIFIERS
	const AABB aabb_voxels(to_vec3(origin_in_voxels), to_vec3(_voxels.get_size() << lod_index));
	StdVector<VoxelModifier::ShaderData> modifiers_shader_data;
	const VoxelModifierStack &modifiers = data->get_modifiers();
	modifiers.apply_for_gpu_rendering(modifiers_shader_data, aabb_voxels);
	gpu_task->modifiers = std::move(modifiers_shader_data);
#endif

	ctx.status = ThreadedTaskContext::STATUS_TAKEN_OUT;

	// 启动 GPU 任务，之后我们会继续网格生成
	VoxelEngine::get_singleton().push_gpu_task(gpu_task);
}

void MeshBlockTask::set_gpu_results(StdVector<GenerateBlockGPUTaskResult> &&results) {
	_gpu_generation_results = std::move(results);
	_stage = 1;
}

#endif

void MeshBlockTask::gather_voxels_cpu() {
	VOXEL_ASSERT(meshing_dependency != nullptr);
	VOXEL_ASSERT(data != nullptr);

	Ref<VoxelMesher> mesher = meshing_dependency->mesher;
	const unsigned int min_padding = mesher->get_minimum_padding();
	const unsigned int max_padding = mesher->get_maximum_padding();

	copy_block_and_neighbors(
			to_span(blocks, blocks_count),
			_voxels,
			min_padding,
			max_padding,
			mesher->get_used_channels_mask(),
			meshing_dependency->generator,
			*data,
			lod_index,
			mesh_block_position,
			nullptr,
			nullptr
	);

	// 如果从这里写入 map 是安全的，本可以把生成器数据缓存起来
	/*if (data != nullptr && cache_generated_blocks) {
		const CubicAreaInfo area_info = get_cubic_area_info_from_size(blocks.size());
		ERR_FAIL_COND(!area_info.is_valid());

		VoxelDataLodMap::Lod &lod = data->lods[lod_index];

		// 注意，这个包围盒不包含相邻的块！
		const Vector3i min_bpos = position * area_info.mesh_block_size_factor;
		const Vector3i max_bpos = min_bpos + Vector3iUtil::create(area_info.edge_size - 2);

		Vector3i bpos;
		for (bpos.z = min_bpos.z; bpos.z < max_bpos.z; ++bpos.z) {
			for (bpos.x = min_bpos.x; bpos.x < max_bpos.x; ++bpos.x) {
				for (bpos.y = min_bpos.y; bpos.y < max_bpos.y; ++bpos.y) {
					// {
					// 	RWLockRead rlock(lod.map_lock);
					// 	VoxelDataBlock *block = lod.map.get_block(bpos);
					// 	if (block != nullptr && (block->is_edited() || block->is_modified())) {
					// 		continue;
					// 	}
					// }
					std::shared_ptr<VoxelBuffer> &cache_buffer = make_shared_instance<VoxelBuffer>();
					cache_buffer->copy_format(voxels);
					const Vector3i min_src_pos =
							(bpos - min_bpos) * data_block_size + Vector3iUtil::create(min_padding);
					cache_buffer->copy_from(voxels, min_src_pos, min_src_pos + cache_buffer->get_size(), Vector3i());
					// TODO 体素该放在哪里？目前无法安全地写入数据。
				}
			}
		}
	}*/
}

void MeshBlockTask::build_mesh() {
	Ref<VoxelMesher> mesher = meshing_dependency->mesher;
	const Vector3i mesh_block_size =
			_voxels.get_size() - Vector3iUtil::create(mesher->get_minimum_padding() + mesher->get_maximum_padding());

	const Vector3i origin_in_voxels = mesh_block_position * (mesh_block_size << lod_index);

	const VoxelMesher::Input input{
		_voxels,
		meshing_dependency->generator.ptr(),
		origin_in_voxels,
		lod_index,
		collision_hint,
		lod_hint,
		// TODO 收集细节纹理信息并非总是必要
		true // detail_texture_hint
	};
	mesher->build(_surfaces_output, input);

#ifdef VOXEL_ENABLE_SMOOTH_MESHING
	const bool mesh_is_empty = VoxelMesher::is_mesh_empty(_surfaces_output.surfaces);

	// 目前，Transvoxel 仅与细节法线贴图纹理结合使用才受支持，因为该算法
	// 为细分网格的单元提供了廉价的来源。从任何网格中获取单元应该都是可能的，
	// 但从头开始查找它们的成本更高，目前 Transvoxel 是平滑地形最可行的算法。
	Ref<VoxelMesherTransvoxel> transvoxel_mesher;

	if (
			require_visual //
			&& voxel::godot::try_get_as(mesher, transvoxel_mesher) //
			&& detail_texture_settings.enabled //
			&& !mesh_is_empty //
			&& lod_index >= detail_texture_settings.begin_lod_index //
			&& require_detail_texture //
	) {
		VOXEL_PROFILE_SCOPE_NAMED("Schedule detail render");

		const transvoxel::MeshArrays &mesh_arrays = VoxelMesherTransvoxel::get_mesh_cache_from_current_thread();
		Span<const transvoxel::CellInfo> cell_infos = VoxelMesherTransvoxel::get_cell_info_from_current_thread();
		VOXEL_ASSERT(cell_infos.size() > 0 && mesh_arrays.vertices.size() > 0);

		UniquePtr<TransvoxelCellIterator> cell_iterator = make_unique_instance<TransvoxelCellIterator>(cell_infos);

		std::shared_ptr<DetailTextureOutput> detail_textures = make_shared_instance<DetailTextureOutput>();
		detail_textures->valid = false;
		// 这里保存一份副本，以防细节纹理渲染在当前任务输出被主线程
		// 取出之前完成，因为它是在一个独立的异步任务中运行的
		_detail_textures = detail_textures;

		RenderDetailTextureTask *nm_task = VOXEL_NEW(RenderDetailTextureTask);
		nm_task->cell_iterator = std::move(cell_iterator);
		// 复制网格数据
		append_array(nm_task->mesh_vertices, mesh_arrays.vertices);
		append_array(nm_task->mesh_normals, mesh_arrays.normals);
		append_array(nm_task->mesh_indices, mesh_arrays.indices);
		if (detail_texture_generator_override.is_valid()) {
			nm_task->generator = lod_index >= detail_texture_generator_override_begin_lod_index
					? detail_texture_generator_override
					: meshing_dependency->generator;
		} else {
			nm_task->generator = meshing_dependency->generator;
		}
		nm_task->voxel_data = data;
		nm_task->mesh_block_size = mesh_block_size;
		nm_task->lod_index = lod_index;
		nm_task->mesh_block_position = mesh_block_position;
		nm_task->volume_id = volume_id;
		nm_task->output_textures = detail_textures;
		nm_task->detail_texture_settings = detail_texture_settings;
		nm_task->priority_dependency = priority_dependency;
#ifdef VOXEL_ENABLE_GPU
		nm_task->use_gpu =
				(detail_texture_use_gpu && nm_task->generator.is_valid() && nm_task->generator->supports_shaders());
#endif

		VoxelEngine::get_singleton().push_async_task(nm_task);
	}
#endif

	if (require_visual && VoxelEngine::get_singleton().is_threaded_graphics_resource_building_enabled()) {
		// 这只有在引擎支持多线程构建网格时才可运行

		_mesh = voxel::build_mesh(
				to_span(_surfaces_output.surfaces),
				_surfaces_output.primitive_type,
				_surfaces_output.mesh_flags,
				_mesh_material_indices
		);

		if (_surfaces_output.shadow_occluder.size() > 0) {
			_shadow_occluder_mesh = voxel::build_mesh(_surfaces_output.shadow_occluder);
		}

		_has_mesh_resource = true;

	} else {
		_has_mesh_resource = false;
	}

	_has_run = true;
}

TaskPriority MeshBlockTask::get_priority() {
	float closest_viewer_distance_sq;
	const TaskPriority p =
			priority_dependency.evaluate(lod_index, constants::TASK_PRIORITY_MESH_BAND2, &closest_viewer_distance_sq);
	_too_far = closest_viewer_distance_sq > priority_dependency.drop_distance_squared;
	return p;
}

bool MeshBlockTask::is_cancelled() {
	if (cancellation_token.is_valid()) {
		return cancellation_token.is_cancelled();
	}
	return !meshing_dependency->valid || _too_far;
}

void MeshBlockTask::apply_result() {
	if (VoxelEngine::get_singleton().is_volume_valid(volume_id)) {
		// 请求响应必须与请求时对应的依赖相匹配。
		// 如果不匹配，说明我们不再关心该结果。
		// 可以假定：当依赖发生变化时，会创建一份新副本，旧副本被标记为无效。
		if (meshing_dependency->valid) {
			VoxelEngine::BlockMeshOutput o;
			// TODO 检查因属性变化导致的失效

			if (_has_run) {
				o.type = VoxelEngine::BlockMeshOutput::TYPE_MESHED;
			} else {
				o.type = VoxelEngine::BlockMeshOutput::TYPE_DROPPED;
			}

			o.position = mesh_block_position;
			o.lod = lod_index;
			o.surfaces = std::move(_surfaces_output);
			o.mesh = _mesh;
			o.shadow_occluder_mesh = _shadow_occluder_mesh;
			o.mesh_material_indices = std::move(_mesh_material_indices);
			o.has_mesh_resource = _has_mesh_resource;
			o.visual_was_required = require_visual;
#ifdef VOXEL_ENABLE_SMOOTH_MESHING
			o.detail_textures = _detail_textures;
#endif

			VoxelEngine::VolumeCallbacks callbacks = VoxelEngine::get_singleton().get_volume_callbacks(volume_id);
			ERR_FAIL_COND(callbacks.mesh_output_callback == nullptr);
			ERR_FAIL_COND(callbacks.data == nullptr);
			callbacks.mesh_output_callback(callbacks.data, o);
		}

	} else {
		// 用户可能在请求尚未返回时移除了体积
		VOXEL_PRINT_VERBOSE("Mesh request response came back but volume wasn't found");
	}
}

} // namespace voxel
