#include "render_detail_texture_gpu_task.h"
#include "../../util/dstack.h"
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include "../../util/godot/classes/rendering_device.h"
#include "../../util/godot/core/packed_arrays.h"
#include "../../util/profiling.h"
#include "../gpu/compute_shader.h"
#include "../gpu/compute_shader_parameters.h"
#include "../voxel_engine.h"
#include "render_detail_texture_task.h"

#ifdef VOXEL_ENABLE_MODIFIERS
#include "../../modifiers/voxel_modifier.h"
#endif

// #ifdef DEBUG_ENABLED
// #include "../../util/string/format.h"
// #endif

using namespace voxel::godot;

namespace voxel {

void RenderDetailTextureGPUTask::prepare(GPUTaskContext &ctx) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_DSTACK();

	ERR_FAIL_COND(mesh_vertices.size() == 0);
	ERR_FAIL_COND(mesh_indices.size() == 0);
	ERR_FAIL_COND(cell_triangles.size() == 0);

	ERR_FAIL_COND(shader == nullptr);
	ERR_FAIL_COND(!shader->get_rid().is_valid());

	RenderingDevice &rd = ctx.rendering_device;
	GPUStorageBufferPool &storage_buffer_pool = ctx.storage_buffer_pool;

	// 尺寸每次都可能变化，因此必须重新创建格式……
	Ref<RDTextureFormat> texture_format;
	texture_format.instantiate();
	texture_format->set_width(texture_width);
	texture_format->set_height(texture_height);
	texture_format->set_format(RenderingDevice::DATA_FORMAT_R8G8B8A8_UINT);
	texture_format->set_usage_bits(
			RenderingDevice::TEXTURE_USAGE_STORAGE_BIT |
			// TODO 不确定是否真的需要 `TEXTURE_USAGE_CAN_UPDATE_BIT`，我们只是生成纹理
			RenderingDevice::TEXTURE_USAGE_CAN_UPDATE_BIT | RenderingDevice::TEXTURE_USAGE_CAN_COPY_FROM_BIT
	);
	texture_format->set_texture_type(RenderingDevice::TEXTURE_TYPE_2D);

	// TODO 可以做哪些优化？
	// - 某些存储缓冲区可以改用统一缓冲区，对于小型结构体也许更快？它们也可以池化
	// - 在运行着色器之前创建/更新缓冲区，也许并不需要所有屏障？这是默认参数
	// - 创建纹理（图像……）也需要一个池，但也许需要一个更专门的池？我无法创建
	// 一个能容纳所有情况的超大纹理，因为它之后必须被下载回来，且下载速度直接取决于
	// 数据大小。另外，我到底为什么要用图像？
	// - 每次我只修改其中一个传入的值时，真的都要新建一个 uniform 集合吗？
	// - 一个任务会顺序使用多个着色器（以及管线，尽管计算管线相比渲染管线似乎没什么
	// 可配置项）。但我猜切换着色器是有成本的，而且同类的任务很可能不止一个会执行。
	// 是否有充分理由让我重新组织调度方式以减少着色器切换？

	// 我们无法在构建计算列表的同时创建资源，所以这里会有点绕，必须先为所有着色器创建资源，
	// 然后才能创建列表。

	// 第一张输出图像

	Ref<RDTextureView> texture0_view;
	texture0_view.instantiate();

	// TODO 我是否必须使用纹理？它比存储缓冲区更好吗？
	_normalmap_texture0_rid = texture_create(rd, **texture_format, **texture0_view, TypedArray<PackedByteArray>());
	ERR_FAIL_COND(!_normalmap_texture0_rid.is_valid());

	Ref<RDUniform> image0_uniform;
	image0_uniform.instantiate();
	image0_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
	image0_uniform->add_id(_normalmap_texture0_rid);

	// 第二张临时图像

	Ref<RDTextureView> texture1_view;
	texture1_view.instantiate();

	_normalmap_texture1_rid = texture_create(rd, **texture_format, **texture1_view, TypedArray<PackedByteArray>());
	ERR_FAIL_COND(!_normalmap_texture1_rid.is_valid());

	Ref<RDUniform> image1_uniform;
	image1_uniform.instantiate();
	image1_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
	image1_uniform->add_id(_normalmap_texture1_rid);

	// 网格顶点

	PackedByteArray mesh_vertices_pba;
	copy_bytes_to<Vector4f>(mesh_vertices_pba, to_span(mesh_vertices));

	_mesh_vertices_sb = storage_buffer_pool.allocate(mesh_vertices_pba);
	ERR_FAIL_COND(_mesh_vertices_sb.is_null());

	Ref<RDUniform> mesh_vertices_uniform;
	mesh_vertices_uniform.instantiate();
	mesh_vertices_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	mesh_vertices_uniform->add_id(_mesh_vertices_sb.rid);

	// 网格索引

	PackedByteArray mesh_indices_pba;
	copy_bytes_to<int32_t>(mesh_indices_pba, to_span(mesh_indices));

	_mesh_indices_sb = storage_buffer_pool.allocate(mesh_indices_pba);
	ERR_FAIL_COND(_mesh_indices_sb.is_null());

	Ref<RDUniform> mesh_indices_uniform;
	mesh_indices_uniform.instantiate();
	mesh_indices_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	mesh_indices_uniform->add_id(_mesh_indices_sb.rid);

	// 单元格三角形

	PackedByteArray cell_triangles_pba;
	copy_bytes_to<int32_t>(cell_triangles_pba, to_span(cell_triangles));

	_cell_triangles_sb = storage_buffer_pool.allocate(cell_triangles_pba);
	ERR_FAIL_COND(_cell_triangles_sb.is_null());

	Ref<RDUniform> cell_triangles_uniform;
	cell_triangles_uniform.instantiate();
	cell_triangles_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	cell_triangles_uniform->add_id(_cell_triangles_sb.rid);

	// 瓦片数据

	PackedByteArray tile_data_pba;
	copy_bytes_to<TileData>(tile_data_pba, to_span(tile_data));

	_tile_data_sb = storage_buffer_pool.allocate(tile_data_pba);
	ERR_FAIL_COND(_tile_data_sb.is_null());

	Ref<RDUniform> tile_data_uniform;
	tile_data_uniform.instantiate();
	tile_data_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	tile_data_uniform->add_id(_tile_data_sb.rid);

	// 命中收集参数

	struct GatherHitsParams {
		Vector3f block_origin_world;
		float pixel_world_step;
		int32_t tile_size_pixels;
	};

	PackedByteArray gather_hits_params_pba;
	copy_bytes_to(
			gather_hits_params_pba,
			GatherHitsParams{ params.block_origin_world, params.pixel_world_step, params.tile_size_pixels }
	);

	// TODO 这里改用统一缓冲区可能更好。对于少量数据它们可能更快，但需要更注意对齐
	_gather_hits_params_sb = storage_buffer_pool.allocate(gather_hits_params_pba);
	ERR_FAIL_COND(_gather_hits_params_sb.is_null());

	Ref<RDUniform> gather_hits_params_uniform;
	gather_hits_params_uniform.instantiate();
	gather_hits_params_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	gather_hits_params_uniform->add_id(_gather_hits_params_sb.rid);

	// 命中缓冲区

	const unsigned int hit_positions_buffer_size_bytes =
			tile_data.size() * math::squared(params.tile_size_pixels) * sizeof(float) * 4;
	_hit_positions_buffer_sb = storage_buffer_pool.allocate(hit_positions_buffer_size_bytes);

	Ref<RDUniform> hit_positions_uniform;
	hit_positions_uniform.instantiate();
	hit_positions_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	hit_positions_uniform->add_id(_hit_positions_buffer_sb.rid);

	// 生成器参数

	struct GeneratorParams {
		int32_t tile_size_pixels;
		float pxiel_world_step;
	};

	PackedByteArray generator_params_pba;
	copy_bytes_to(generator_params_pba, GeneratorParams{ params.tile_size_pixels, params.pixel_world_step });

	_generator_params_sb = storage_buffer_pool.allocate(generator_params_pba);

	Ref<RDUniform> generator_params_uniform;
	generator_params_uniform.instantiate();
	generator_params_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	generator_params_uniform->add_id(_generator_params_sb.rid);

	// 有符号距离缓冲区

	// TODO 也许使用半精度就足够好了？
	const unsigned int sd_buffer_size_bytes =
			tile_data.size() * math::squared(params.tile_size_pixels) * 4 * sizeof(float);

	_sd_buffer0_sb = storage_buffer_pool.allocate(sd_buffer_size_bytes);

	Ref<RDUniform> sd_buffer0_uniform;
	sd_buffer0_uniform.instantiate();
	sd_buffer0_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	sd_buffer0_uniform->add_id(_sd_buffer0_sb.rid);

#ifdef VOXEL_ENABLE_MODIFIERS
	Ref<RDUniform> sd_buffer1_uniform;
	if (modifiers.size() > 0) {
		_sd_buffer1_sb = storage_buffer_pool.allocate(sd_buffer_size_bytes);

		sd_buffer1_uniform.instantiate();
		sd_buffer1_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
		sd_buffer1_uniform->add_id(_sd_buffer1_sb.rid);
	}
#endif

	// 法线贴图参数

	struct NormalmapParams {
		int32_t tile_size_pixels;
		int32_t tiles_x;
		float max_deviation_cosine;
		float max_deviation_sine;
	};

	PackedByteArray normalmap_params_pba;
	copy_bytes_to(
			normalmap_params_pba,
			NormalmapParams{
					params.tile_size_pixels, params.tiles_x, params.max_deviation_cosine, params.max_deviation_sine }
	);

	_normalmap_params_sb = storage_buffer_pool.allocate(normalmap_params_pba);

	Ref<RDUniform> normalmap_params_uniform;
	normalmap_params_uniform.instantiate();
	normalmap_params_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	normalmap_params_uniform->add_id(_normalmap_params_sb.rid);

	// 膨胀参数

	PackedByteArray dilation_params_pba;
	// 我只需要 4 字节，但显然 UBO 的最小大小是 16 字节
	dilation_params_pba.resize(16);
	*reinterpret_cast<int32_t *>(dilation_params_pba.ptrw()) = params.tile_size_pixels;

	_dilation_params_rid = rd.uniform_buffer_create(dilation_params_pba.size(), dilation_params_pba);
	ERR_FAIL_COND(!_dilation_params_rid.is_valid());

	Ref<RDUniform> dilation_params_uniform;
	dilation_params_uniform.instantiate();
	dilation_params_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_UNIFORM_BUFFER);
	dilation_params_uniform->add_id(_dilation_params_rid);

	// 管线
	// 不确定计算着色器中的管线是做什么用的，看起来只是"因为需要所以需要"

	const RID gather_hits_shader_rid = ctx.base_resources.detail_gather_hits_shader.rid;
	ERR_FAIL_COND(!gather_hits_shader_rid.is_valid());
	// TODO 也许可以缓存这个管线？
	_gather_hits_pipeline_rid = rd.compute_pipeline_create(gather_hits_shader_rid);
	ERR_FAIL_COND(!_gather_hits_pipeline_rid.is_valid());

	const RID shader_rid = shader->get_rid();
	_detail_generator_pipeline_rid = rd.compute_pipeline_create(shader_rid);
	ERR_FAIL_COND(!_detail_generator_pipeline_rid.is_valid());

#ifdef VOXEL_ENABLE_MODIFIERS
	for (const VoxelModifier::ShaderData &modifier : modifiers) {
		const RID modifier_shader_rid = VoxelModifier::get_detail_shader(ctx.base_resources, modifier.modifier_type);
		ERR_FAIL_COND(!modifier_shader_rid.is_valid());
		const RID rid = rd.compute_pipeline_create(modifier_shader_rid);
		ERR_FAIL_COND(!rid.is_valid());
		_detail_modifier_pipelines.push_back(rid);
	}
#endif

	const RID detail_normalmap_shader_rid = ctx.base_resources.detail_normalmap_shader.rid;
	ERR_FAIL_COND(!detail_normalmap_shader_rid.is_valid());
	// TODO 也许可以缓存这个管线？
	_detail_normalmap_pipeline_rid = rd.compute_pipeline_create(detail_normalmap_shader_rid);
	ERR_FAIL_COND(!_detail_normalmap_pipeline_rid.is_valid());

	const RID dilation_shader_rid = ctx.base_resources.dilate_normalmap_shader.rid;
	ERR_FAIL_COND(!dilation_shader_rid.is_valid());
	// TODO 也许可以缓存这个管线？
	_normalmap_dilation_pipeline_rid = rd.compute_pipeline_create(dilation_shader_rid);
	ERR_FAIL_COND(!_normalmap_dilation_pipeline_rid.is_valid());

	// 构建计算列表

#ifdef VOXEL_ENABLE_MODIFIERS
	const unsigned int modifier_count = modifiers.size();
#else
	const unsigned int modifier_count = 0;
#endif
	_uniform_sets_to_free.reserve(5 + modifier_count);

	const int compute_list_id = rd.compute_list_begin();

	// 收集命中
	{
		mesh_vertices_uniform->set_binding(0);
		mesh_indices_uniform->set_binding(1);
		cell_triangles_uniform->set_binding(2);
		tile_data_uniform->set_binding(3);
		gather_hits_params_uniform->set_binding(4);
		hit_positions_uniform->set_binding(5);

		Array gather_hits_uniforms;
		gather_hits_uniforms.resize(6);
		gather_hits_uniforms[0] = mesh_vertices_uniform;
		gather_hits_uniforms[1] = mesh_indices_uniform;
		gather_hits_uniforms[2] = cell_triangles_uniform;
		gather_hits_uniforms[3] = tile_data_uniform;
		gather_hits_uniforms[4] = gather_hits_params_uniform;
		gather_hits_uniforms[5] = hit_positions_uniform;

		const RID gather_hits_uniform_set_rid = uniform_set_create(rd, gather_hits_uniforms, gather_hits_shader_rid, 0);
		_uniform_sets_to_free.push_back(gather_hits_uniform_set_rid);

		rd.compute_list_bind_compute_pipeline(compute_list_id, _gather_hits_pipeline_rid);
		rd.compute_list_bind_uniform_set(compute_list_id, gather_hits_uniform_set_rid, 0);

		const int local_group_size_x = 4;
		const int local_group_size_y = 4;
		const int local_group_size_z = 4;
		rd.compute_list_dispatch(
				compute_list_id, //
				math::ceildiv(params.tile_size_pixels, local_group_size_x),
				math::ceildiv(params.tile_size_pixels, local_group_size_y),
				math::ceildiv(static_cast<int32_t>(tile_data.size()), local_group_size_z)
		);
	}

	// 确保在结果上运行膨胀之前依赖已就绪（我以为这是自动处理的？
	// 为什么还是需要屏障？依赖解析实际上不是自动的吗？）
	rd.compute_list_add_barrier(compute_list_id);

	// 生成有符号距离
	{
		hit_positions_uniform->set_binding(0);
		generator_params_uniform->set_binding(1);
		sd_buffer0_uniform->set_binding(2);
		// sd_buffer1_uniform->set_binding(3);

		Array detail_generator_uniforms;
		detail_generator_uniforms.resize(3);
		detail_generator_uniforms[0] = hit_positions_uniform;
		detail_generator_uniforms[1] = generator_params_uniform;
		detail_generator_uniforms[2] = sd_buffer0_uniform;

		// 额外参数
		if (shader_params != nullptr && shader_params->params.size() > 0) {
			add_uniform_params(
					shader_params->params, detail_generator_uniforms, ctx.base_resources.filtering_sampler_rid
			);
		}

		const RID detail_generator_uniform_set = uniform_set_create(rd, detail_generator_uniforms, shader_rid, 0);
		_uniform_sets_to_free.push_back(detail_generator_uniform_set);

		rd.compute_list_bind_compute_pipeline(compute_list_id, _detail_generator_pipeline_rid);
		rd.compute_list_bind_uniform_set(compute_list_id, detail_generator_uniform_set, 0);

		const int local_group_size_x = 4;
		const int local_group_size_y = 4;
		const int local_group_size_z = 4;
		rd.compute_list_dispatch(
				compute_list_id,
				math::ceildiv(params.tile_size_pixels, local_group_size_x),
				math::ceildiv(params.tile_size_pixels, local_group_size_y),
				math::ceildiv(static_cast<int32_t>(tile_data.size()), local_group_size_z)
		);
	}

	rd.compute_list_add_barrier(compute_list_id);

	// 应用修改器

#ifdef VOXEL_ENABLE_MODIFIERS
	for (unsigned int modifier_index = 0; modifier_index < modifiers.size(); ++modifier_index) {
		const VoxelModifier::ShaderData &modifier_data = modifiers[modifier_index];
		const RID modifier_shader_rid =
				VoxelModifier::get_detail_shader(ctx.base_resources, modifier_data.modifier_type);
		VOXEL_ASSERT_CONTINUE(modifier_shader_rid.is_valid());

		hit_positions_uniform->set_binding(0);
		generator_params_uniform->set_binding(1);
		sd_buffer0_uniform->set_binding(2);
		sd_buffer1_uniform->set_binding(3);

		Array detail_modifier_uniforms;
		detail_modifier_uniforms.resize(4);
		detail_modifier_uniforms[0] = hit_positions_uniform;
		detail_modifier_uniforms[1] = generator_params_uniform;
		detail_modifier_uniforms[2] = sd_buffer0_uniform;
		detail_modifier_uniforms[3] = sd_buffer1_uniform;

		// 交换缓冲区
		// TODO 是否可以读写同一个缓冲区，这样就不需要乒乓（ping-pong）了？
		Ref<RDUniform> temp = sd_buffer1_uniform;
		sd_buffer1_uniform = sd_buffer0_uniform;
		sd_buffer0_uniform = temp;

		// 额外参数
		if (modifier_data.params != nullptr) {
			add_uniform_params(
					modifier_data.params->params, detail_modifier_uniforms, ctx.base_resources.filtering_sampler_rid
			);
		}

		const RID detail_modifier_uniform_set =
				uniform_set_create(rd, detail_modifier_uniforms, modifier_shader_rid, 0);
		_uniform_sets_to_free.push_back(detail_modifier_uniform_set);

		const RID pipeline_rid = _detail_modifier_pipelines[modifier_index];
		rd.compute_list_bind_compute_pipeline(compute_list_id, pipeline_rid);
		rd.compute_list_bind_uniform_set(compute_list_id, detail_modifier_uniform_set, 0);

		const int local_group_size_x = 4;
		const int local_group_size_y = 4;
		const int local_group_size_z = 4;
		rd.compute_list_dispatch(
				compute_list_id,
				math::ceildiv(params.tile_size_pixels, local_group_size_x),
				math::ceildiv(params.tile_size_pixels, local_group_size_y),
				math::ceildiv(static_cast<int32_t>(tile_data.size()), local_group_size_z)
		);

		rd.compute_list_add_barrier(compute_list_id);
	}
#endif

	// 法线贴图渲染
	{
		sd_buffer0_uniform->set_binding(0);
		mesh_vertices_uniform->set_binding(1);
		mesh_indices_uniform->set_binding(2);
		hit_positions_uniform->set_binding(3);
		normalmap_params_uniform->set_binding(4);
		image0_uniform->set_binding(5);

		Array detail_normalmap_uniforms;
		detail_normalmap_uniforms.resize(6);
		detail_normalmap_uniforms[0] = sd_buffer0_uniform;
		detail_normalmap_uniforms[1] = mesh_vertices_uniform;
		detail_normalmap_uniforms[2] = mesh_indices_uniform;
		detail_normalmap_uniforms[3] = hit_positions_uniform;
		detail_normalmap_uniforms[4] = normalmap_params_uniform;
		detail_normalmap_uniforms[5] = image0_uniform;

		const RID detail_normalmap_uniform_set_rid =
				uniform_set_create(rd, detail_normalmap_uniforms, detail_normalmap_shader_rid, 0);
		_uniform_sets_to_free.push_back(detail_normalmap_uniform_set_rid);
		// #ifdef DEV_ENABLED
		// 		_uniform_sets_expected_to_be_freed.push_back(detail_normalmap_uniform_set_rid);
		// #endif

		rd.compute_list_bind_compute_pipeline(compute_list_id, _detail_normalmap_pipeline_rid);
		rd.compute_list_bind_uniform_set(compute_list_id, detail_normalmap_uniform_set_rid, 0);

		const int local_group_size_x = 4;
		const int local_group_size_y = 4;
		const int local_group_size_z = 4;
		rd.compute_list_dispatch(
				compute_list_id,
				math::ceildiv(params.tile_size_pixels, local_group_size_x),
				math::ceildiv(params.tile_size_pixels, local_group_size_y),
				math::ceildiv(static_cast<int32_t>(tile_data.size()), local_group_size_z)
		);
	}

	rd.compute_list_add_barrier(compute_list_id);

	// 膨胀步骤 1
	{
		image0_uniform->set_binding(0);
		image1_uniform->set_binding(1);
		dilation_params_uniform->set_binding(2);

		Array dilation_uniforms;
		dilation_uniforms.resize(3);
		// 此时绑定应分别为 0 和 1
		dilation_uniforms[0] = image0_uniform;
		dilation_uniforms[1] = image1_uniform;
		dilation_uniforms[2] = dilation_params_uniform;
		const RID dilation_uniform_set_rid = uniform_set_create(rd, dilation_uniforms, dilation_shader_rid, 0);
		_uniform_sets_to_free.push_back(dilation_uniform_set_rid);
		// #ifdef DEV_ENABLED
		// 		_uniform_sets_expected_to_be_freed.push_back(dilation_uniform_set_rid);
		// #endif

		rd.compute_list_bind_compute_pipeline(compute_list_id, _normalmap_dilation_pipeline_rid);
		rd.compute_list_bind_uniform_set(compute_list_id, dilation_uniform_set_rid, 0);

		const unsigned int local_group_size_x = 8;
		const unsigned int local_group_size_y = 8;
		const unsigned int local_group_size_z = 1;
		rd.compute_list_dispatch(
				compute_list_id,
				// 不得不显式转换，因为即使两个参数都是无符号的，MSVC 也笨到无法
				// 意识到可以直接使用该函数的无符号版本。另外，如果两者都是 uint16_t，它
				// 居然会选择有符号版本。
				math::ceildiv(static_cast<unsigned int>(texture_width), local_group_size_x),
				math::ceildiv(static_cast<unsigned int>(texture_height), local_group_size_y),
				local_group_size_z
		);
	}

	rd.compute_list_add_barrier(compute_list_id);

	// 膨胀步骤 2
	{
		// 交换图像

		image1_uniform->set_binding(0);
		image0_uniform->set_binding(1);

		// Uniform 集合

		Array dilation_uniforms;
		dilation_uniforms.resize(3);
		dilation_uniforms[0] = image1_uniform;
		dilation_uniforms[1] = image0_uniform;
		dilation_uniforms[2] = dilation_params_uniform;
		// TODO 每次只修改其中一个传入的值时，真的都要新建一个 uniform 集合吗？
		const RID dilation_uniform_set_rid = uniform_set_create(rd, dilation_uniforms, dilation_shader_rid, 0);
		_uniform_sets_to_free.push_back(dilation_uniform_set_rid);
		// #ifdef DEV_ENABLED
		// 		_uniform_sets_expected_to_be_freed.push_back(dilation_uniform_set_rid);
		// #endif

		rd.compute_list_bind_uniform_set(compute_list_id, dilation_uniform_set_rid, 0);

		const unsigned int local_group_size_x = 8;
		const unsigned int local_group_size_y = 8;
		const unsigned int local_group_size_z = 1;
		rd.compute_list_dispatch(
				compute_list_id,
				math::ceildiv(static_cast<unsigned int>(texture_width), local_group_size_x),
				math::ceildiv(static_cast<unsigned int>(texture_height), local_group_size_y),
				local_group_size_z
		);
	}

	// 最终结果应在 image0 中。

	rd.compute_list_end();
}

PackedByteArray RenderDetailTextureGPUTask::collect_texture_and_cleanup(
		RenderingDevice &rd,
		GPUStorageBufferPool &storage_buffer_pool
) {
	VOXEL_PROFILE_SCOPE();

	// TODO 这极其缓慢，本就不该发生。
	// 但由于 Godot 当前的设计，无法直接从计算着色器的输出创建纹理，
	// 只能先把它下载回内存……
	PackedByteArray texture_data = rd.texture_get_data(_normalmap_texture0_rid, 0);

	{
		VOXEL_PROFILE_SCOPE_NAMED("Cleanup");

		// Godot 会在其依赖被释放时"自动释放" uniform 集合。
		// 但有时它不会，也无法猜到应该释放（例如复用资源时）。
		// 因此我们必须手动检查哪些该释放、哪些不该释放。
		// 参见 https://github.com/godotengine/godot/issues/103073
		// 与其添加更多调试检查，不如先手动释放 uniform 集合，
		// 这样比之后等 Godot 自动释放更简单。
		for (RID rid : _uniform_sets_to_free) {
			free_rendering_device_rid(rd, rid);
		}

		free_rendering_device_rid(rd, _normalmap_texture0_rid);
		free_rendering_device_rid(rd, _normalmap_texture1_rid);

		free_rendering_device_rid(rd, _gather_hits_pipeline_rid);
		free_rendering_device_rid(rd, _detail_generator_pipeline_rid);
		free_rendering_device_rid(rd, _detail_normalmap_pipeline_rid);
		free_rendering_device_rid(rd, _normalmap_dilation_pipeline_rid);
		for (RID rid : _detail_modifier_pipelines) {
			free_rendering_device_rid(rd, rid);
		}

		// #ifdef DEV_ENABLED
		// 		for (unsigned int i = 0; i < _uniform_sets_expected_to_be_freed.size(); ++i) {
		// 			const RID rid = _uniform_sets_expected_to_be_freed[i];
		// 			if(rd.uniform_set_is_valid(rid)) {
		// 				VOXEL_PRINT_ERROR(format("Uniform Set #{} wasn't freed by Godot", i));
		// 			}
		// 		}
		// #endif

		storage_buffer_pool.recycle(_mesh_vertices_sb);
		storage_buffer_pool.recycle(_mesh_indices_sb);
		storage_buffer_pool.recycle(_cell_triangles_sb);
		storage_buffer_pool.recycle(_tile_data_sb);
		storage_buffer_pool.recycle(_gather_hits_params_sb);

		free_rendering_device_rid(rd, _dilation_params_rid);

		storage_buffer_pool.recycle(_hit_positions_buffer_sb);
		storage_buffer_pool.recycle(_generator_params_sb);
		storage_buffer_pool.recycle(_sd_buffer0_sb);

		if (_sd_buffer1_sb.is_valid()) {
			storage_buffer_pool.recycle(_sd_buffer1_sb);
		}

		storage_buffer_pool.recycle(_normalmap_params_sb);
	}

	// Uniform 集合会在其内容被释放后自动释放。
	// rd.free(_uniform_set_rid);
	return texture_data;
}

void RenderDetailTextureGPUTask::collect(GPUTaskContext &ctx) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_DSTACK();

	PackedByteArray texture_data = collect_texture_and_cleanup(ctx.rendering_device, ctx.storage_buffer_pool);

#ifdef VOXEL_TESTS
	if (testing_output != nullptr) {
		*testing_output = texture_data;
		return;
	}
#endif

	{
		StdVector<DetailTextureData::Tile> tile_data2;
		tile_data2.reserve(tile_data.size());
		for (const TileData &td : tile_data) {
			tile_data2.push_back(DetailTextureData::Tile{ td.cell_x, td.cell_y, td.cell_z, uint8_t(td.data & 0x3) });
		}

		RenderDetailTexturePass2Task *task = VOXEL_NEW(RenderDetailTexturePass2Task);
		task->atlas_data = texture_data;
		task->tile_data = std::move(tile_data2);
		task->edited_tiles_texture_data = std::move(edited_tiles_texture_data);
		task->output_textures = output;
		task->volume_id = volume_id;
		task->mesh_block_position = block_position;
		task->mesh_block_size = block_size;
		task->atlas_width = texture_width;
		task->atlas_height = texture_height;
		task->lod_index = lod_index;
		task->tile_size_pixels = params.tile_size_pixels;

		VoxelEngine::get_singleton().push_async_task(task);
	}
}

} // namespace voxel
