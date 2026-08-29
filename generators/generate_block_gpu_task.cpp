#include "../engine/gpu/compute_shader.h"
#include "../engine/gpu/compute_shader_parameters.h"
#include "../engine/voxel_engine.h"
#include "../meshers/mesh_block_task.h"
#include "../storage/mixel4.h"
#include "../util/dstack.h"
#include "../util/godot/classes/rendering_device.h"
#include "../util/godot/core/packed_arrays.h"
#include "../util/math/conv.h"
#include "../util/profiling.h"
#include "../util/string/format.h"

#ifdef VOXEL_ENABLE_MODIFIERS
#include "../modifiers/voxel_modifier.h"
#endif

#ifdef VOXEL_ENABLE_GPU
#include "generate_block_gpu_task.h"
#endif

namespace voxel {

GenerateBlockGPUTask::~GenerateBlockGPUTask() {
	if (consumer_task != nullptr) {
		// 如果走到这里，说明引擎在网格任务完成之前就被关闭了，
		// 所以我们仍然持有此任务的所有权，应当在这里删除它。
		VOXEL_PRINT_VERBOSE("Freeing interrupted consumer task");
		// TODO 我们不能假定此任务是如何被分配的
		VOXEL_DELETE(consumer_task);
	}
}

unsigned int GenerateBlockGPUTask::get_required_shared_output_buffer_size() const {
	unsigned int volume = 0;
	for (const Box3i &box : boxes_to_generate) {
		volume += Vector3iUtil::get_volume_u64(box.size);
	}
	// 目前所有输出都是 float 类型……
	return generator_shader_outputs->outputs.size() * volume * sizeof(float);
}

void GenerateBlockGPUTask::prepare(GPUTaskContext &ctx) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_DSTACK();

	VOXEL_ASSERT_RETURN(generator_shader != nullptr);
	VOXEL_ASSERT_RETURN(generator_shader->get_rid().is_valid());

	VOXEL_ASSERT_RETURN(generator_shader_params != nullptr);
	VOXEL_ASSERT_RETURN(generator_shader_outputs != nullptr);
	VOXEL_ASSERT_RETURN(generator_shader_outputs->outputs.size() > 0);

	VOXEL_ASSERT(consumer_task != nullptr);

	ERR_FAIL_COND(boxes_to_generate.size() == 0);

	RenderingDevice &rd = ctx.rendering_device;
	GPUStorageBufferPool &storage_buffer_pool = ctx.storage_buffer_pool;

	// 目前 modifiers 只支持 SDF。
	int sd_output_index = -1;
	{
		int i = 0;
		for (const VoxelGenerator::ShaderOutput &output : generator_shader_outputs->outputs) {
			if (output.type == VoxelGenerator::ShaderOutput::TYPE_SDF) {
				sd_output_index = i;
				break;
			}
			++i;
		}
	}

	_boxes_data.resize(boxes_to_generate.size());

	unsigned int out_offset_elements = 0;

	for (unsigned int i = 0; i < _boxes_data.size(); ++i) {
		BoxData &bd = _boxes_data[i];
		const Box3i box = boxes_to_generate[i];
		const Vector3i buffer_resolution = box.size;
		const unsigned int buffer_volume = Vector3iUtil::get_volume_u64(buffer_resolution);

		// 参数

		struct Params {
			Vector3f origin_in_voxels;
			float voxel_size;
			Vector3i block_size;
			int output_buffer_start;
		};

		Params params;
		params.origin_in_voxels = to_vec3f((box.position << lod_index) + origin_in_voxels);
		params.voxel_size = 1 << lod_index;
		params.block_size = buffer_resolution;
		params.output_buffer_start = (ctx.shared_output_buffer_begin / sizeof(float)) + out_offset_elements;

		out_offset_elements += buffer_volume * generator_shader_outputs->outputs.size();

		PackedByteArray params_pba;
		voxel::godot::copy_bytes_to(params_pba, params);

		bd.params_sb = storage_buffer_pool.allocate(params_pba);
		ERR_FAIL_COND(bd.params_sb.is_null());

		bd.params_uniform.instantiate();
		bd.params_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
		bd.params_uniform->add_id(bd.params_sb.rid);

		// 输出

		bd.output_uniform.instantiate();
		bd.output_uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
		bd.output_uniform->add_id(ctx.shared_output_buffer_rid);
	}

	// 管线
	// 不确定计算着色器中管线具体是干什么用的，它似乎只是"必须存在"
	const RID generator_shader_rid = generator_shader->get_rid();
	_generator_pipeline_rid = rd.compute_pipeline_create(generator_shader_rid);
	ERR_FAIL_COND(!_generator_pipeline_rid.is_valid());

#ifdef VOXEL_ENABLE_MODIFIERS
	for (const VoxelModifier::ShaderData &modifier : modifiers) {
		const RID modifier_shader_rid = VoxelModifier::get_block_shader(ctx.base_resources, modifier.modifier_type);
		ERR_FAIL_COND(!modifier_shader_rid.is_valid());
		const RID rid = rd.compute_pipeline_create(modifier_shader_rid);
		ERR_FAIL_COND(!rid.is_valid());
		_modifier_pipelines.push_back(rid);
	}
#endif

	// 创建计算列表

	const int compute_list_id = rd.compute_list_begin();

	// 生成

#ifdef VOXEL_ENABLE_MODIFIERS
	_uniform_sets_to_free.reserve(_boxes_data.size() * (1 + modifiers.size()));
#endif

	for (unsigned int box_index = 0; box_index < _boxes_data.size(); ++box_index) {
		BoxData &bd = _boxes_data[box_index];

		bd.params_uniform->set_binding(0); // 基础参数
		bd.output_uniform->set_binding(1);

		Array generator_uniforms;
		generator_uniforms.resize(2);
		generator_uniforms[0] = bd.params_uniform;
		generator_uniforms[1] = bd.output_uniform;

		// 附加参数
		if (generator_shader_params != nullptr && generator_shader_params->params.size() > 0) {
			add_uniform_params(
					generator_shader_params->params, generator_uniforms, ctx.base_resources.filtering_sampler_rid
			);
		}

		// 注意，这会在内部锁定 RenderingDeviceVulkan 的类互斥锁。这意味着它或许可以在
		// 计算列表之外使用（计算列表会在其结束前一直锁定类互斥锁）。幸好它使用的是
		// 递归 Mutex（而不是 BinaryMutex）
		const RID generator_uniform_set =
				voxel::godot::uniform_set_create(rd, generator_uniforms, generator_shader_rid, 0);
		_uniform_sets_to_free.push_back(generator_uniform_set);

		{
			VOXEL_PROFILE_SCOPE_NAMED("compute_list_bind_compute_pipeline");
			rd.compute_list_bind_compute_pipeline(compute_list_id, _generator_pipeline_rid);
		}
		{
			VOXEL_PROFILE_SCOPE_NAMED("compute_list_bind_uniform_set");
			rd.compute_list_bind_uniform_set(compute_list_id, generator_uniform_set, 0);
		}

		const Box3i &box = boxes_to_generate[box_index];

		// 注意，输出缓冲区的大小可能不是工作组大小的整数倍，因此着色器应避免在某些
		// 调用中写出界。
		const Vector3i groups = math::ceildiv(box.size, Vector3i(4, 4, 4));
		{
			VOXEL_PROFILE_SCOPE_NAMED("compute_list_dispatch");
			rd.compute_list_dispatch(compute_list_id, groups.x, groups.y, groups.z);
		}
	}

	// TODO 我们可以在批次中调度所有生成器（而不仅仅是本任务）以减少屏障，然后再做
	// 屏障？
	rd.compute_list_add_barrier(compute_list_id);

#ifdef VOXEL_ENABLE_MODIFIERS
	// 就地应用 modifiers
	if (sd_output_index != -1) {
		for (unsigned int box_index = 0; box_index < _boxes_data.size(); ++box_index) {
			BoxData &bd = _boxes_data[box_index];

			const Box3i &box = boxes_to_generate[box_index];
			const Vector3i groups = math::ceildiv(box.size, Vector3i(4, 4, 4));

			Ref<RDUniform> sd_buffer0_uniform = bd.output_uniform;

			for (unsigned int modifier_index = 0; modifier_index < modifiers.size(); ++modifier_index) {
				const VoxelModifier::ShaderData &modifier_data = modifiers[modifier_index];
				const RID modifier_shader_rid =
						VoxelModifier::get_block_shader(ctx.base_resources, modifier_data.modifier_type);
				VOXEL_ASSERT_CONTINUE(modifier_shader_rid.is_valid());

				bd.params_uniform->set_binding(0);
				sd_buffer0_uniform->set_binding(1);

				Array modifier_uniforms;
				modifier_uniforms.resize(2);
				modifier_uniforms[0] = bd.params_uniform;
				modifier_uniforms[1] = sd_buffer0_uniform;

				// 附加参数
				if (modifier_data.params != nullptr) {
					add_uniform_params(
							modifier_data.params->params, modifier_uniforms, ctx.base_resources.filtering_sampler_rid
					);
				}

				const RID modifier_uniform_set =
						voxel::godot::uniform_set_create(rd, modifier_uniforms, modifier_shader_rid, 0);
				_uniform_sets_to_free.push_back(modifier_uniform_set);

				const RID pipeline_rid = _modifier_pipelines[modifier_index];
				rd.compute_list_bind_compute_pipeline(compute_list_id, pipeline_rid);
				rd.compute_list_bind_uniform_set(compute_list_id, modifier_uniform_set, 0);

				rd.compute_list_dispatch(compute_list_id, groups.x, groups.y, groups.z);

				rd.compute_list_add_barrier(compute_list_id);
			}
		}
	}
#endif

	rd.compute_list_end();
}

namespace {

StdVector<uint8_t> &get_temporary_conversion_memory_tls() {
	static thread_local StdVector<uint8_t> mem;
	return mem;
}

template <typename T>
inline Span<T> get_temporary_conversion_memory_tls(unsigned int count) {
	StdVector<uint8_t> &mem = get_temporary_conversion_memory_tls();
	mem.resize(count * sizeof(T));
	return to_span(mem).reinterpret_cast_to<T>();
}

void convert_gpu_output_sdf(VoxelBuffer &dst, Span<const float> src_data_f, const Box3i &box) {
	VOXEL_PROFILE_SCOPE();

	const VoxelBuffer::Depth depth = dst.get_channel_depth(VoxelBuffer::CHANNEL_SDF);
	const float sd_scale = VoxelBuffer::get_sdf_quantization_scale(depth);

	switch (depth) {
		case VoxelBuffer::DEPTH_8_BIT: {
			Span<int8_t> sd_data = get_temporary_conversion_memory_tls<int8_t>(src_data_f.size());
			for (unsigned int i = 0; i < src_data_f.size(); ++i) {
				sd_data[i] = snorm_to_s8(sd_scale * src_data_f[i]);
			}
			dst.copy_channel_from(
					sd_data.to_const(), box.size, Vector3i(), box.size, box.position, VoxelBuffer::CHANNEL_SDF
			);
		} break;

		case VoxelBuffer::DEPTH_16_BIT: {
			Span<int16_t> sd_data = get_temporary_conversion_memory_tls<int16_t>(src_data_f.size());
			for (unsigned int i = 0; i < src_data_f.size(); ++i) {
				sd_data[i] = snorm_to_s16(sd_scale * src_data_f[i]);
			}

			dst.copy_channel_from(
					sd_data.to_const(), box.size, Vector3i(), box.size, box.position, VoxelBuffer::CHANNEL_SDF
			);
		} break;

		case VoxelBuffer::DEPTH_32_BIT: {
			dst.copy_channel_from(
					src_data_f.to_const(), box.size, Vector3i(), box.size, box.position, VoxelBuffer::CHANNEL_SDF
			);
		} break;

		case VoxelBuffer::DEPTH_64_BIT: {
			VOXEL_PRINT_ERROR("64-bit SDF is not supported");
		} break;

		default:
			VOXEL_PRINT_ERROR("Unhandled depth");
			break;
	}
}

void convert_gpu_output_single_texture(VoxelBuffer &dst, Span<const float> src_data_f, const Box3i &box) {
	VOXEL_PROFILE_SCOPE();
	const uint16_t encoded_weights = mixel4::make_encoded_weights_for_single_texture();
	dst.fill_area(encoded_weights, box.position, box.position + box.size, VoxelBuffer::CHANNEL_WEIGHTS);

	// 小技巧：目标类型比 float 小，因此可以就地转换。
	Span<uint16_t> src_data_u16 = get_temporary_conversion_memory_tls<uint16_t>(src_data_f.size());

	for (unsigned int value_index = 0; value_index < src_data_f.size(); ++value_index) {
		const uint8_t index = math::clamp(int(Math::round(src_data_f[value_index])), 0, 15);
		const uint16_t encoded_indices = mixel4::make_encoded_indices_for_single_texture(index);
		src_data_u16[value_index] = encoded_indices;
	}

	dst.copy_channel_from(
			src_data_u16.to_const(), box.size, Vector3i(), box.size, box.position, VoxelBuffer::CHANNEL_INDICES
	);
}

template <typename T>
Span<const T> cast_floats(Span<const float> src_data_f, StdVector<uint8_t> &memory) {
	memory.resize(src_data_f.size() * sizeof(T));
	Span<T> dst = to_span(memory).reinterpret_cast_to<T>();
	unsigned int dst_i = 0;
	for (const float src_value : src_data_f) {
		dst[dst_i] = src_value;
		++dst_i;
	}
	return dst;
}

void convert_gpu_output_uint(
		VoxelBuffer &dst,
		Span<const float> src_data_f,
		const Box3i &box,
		VoxelBuffer::ChannelId channel_index
) {
	VOXEL_PROFILE_SCOPE();

	const VoxelBuffer::Depth depth = dst.get_channel_depth(VoxelBuffer::CHANNEL_SDF);
	StdVector<uint8_t> &tls_temp = get_temporary_conversion_memory_tls();

	switch (depth) {
		case VoxelBuffer::DEPTH_8_BIT: {
			dst.copy_channel_from(
					cast_floats<uint8_t>(src_data_f, tls_temp),
					box.size,
					Vector3i(),
					box.size,
					box.position,
					channel_index
			);
		} break;

		case VoxelBuffer::DEPTH_16_BIT: {
			dst.copy_channel_from(
					cast_floats<uint16_t>(src_data_f, tls_temp),
					box.size,
					Vector3i(),
					box.size,
					box.position,
					channel_index
			);
		} break;

		case VoxelBuffer::DEPTH_32_BIT: {
			dst.copy_channel_from(
					cast_floats<uint32_t>(src_data_f, tls_temp),
					box.size,
					Vector3i(),
					box.size,
					box.position,
					channel_index
			);
		} break;

		case VoxelBuffer::DEPTH_64_BIT: {
			dst.copy_channel_from(
					cast_floats<uint64_t>(src_data_f, tls_temp),
					box.size,
					Vector3i(),
					box.size,
					box.position,
					channel_index
			);
		} break;

		default:
			VOXEL_PRINT_ERROR("Unhandled depth");
			break;
	}
}

} // namespace

void GenerateBlockGPUTaskResult::convert_to_voxel_buffer(VoxelBuffer &dst) {
	// 目前着色器只能输出 float 数组。而且看起来 GLSL 没有 8 位或 16 位的数据类型？
	// TODO 我们是否应该在计算着色器中转换以降低带宽和 CPU 开销？
	Span<const float> src_data_f = _bytes.reinterpret_cast_to<const float>();

	switch (_type) {
		case VoxelGenerator::ShaderOutput::TYPE_SDF:
			convert_gpu_output_sdf(dst, src_data_f, _box);
			break;

		case VoxelGenerator::ShaderOutput::TYPE_SINGLE_TEXTURE:
			convert_gpu_output_single_texture(dst, src_data_f, _box);
			break;

		case VoxelGenerator::ShaderOutput::TYPE_TYPE:
			convert_gpu_output_uint(dst, src_data_f, _box, VoxelBuffer::CHANNEL_TYPE);
			break;

		default:
			VOXEL_PRINT_ERROR("Unhandled output");
			break;
	}
}

void GenerateBlockGPUTaskResult::convert_to_voxel_buffer(
		Span<GenerateBlockGPUTaskResult> boxes_data,
		VoxelBuffer &dst
) {
	VOXEL_PROFILE_SCOPE();

	for (GenerateBlockGPUTaskResult &box_data : boxes_data) {
		box_data.convert_to_voxel_buffer(dst);
	}

	dst.compress_uniform_channels();
}

void GenerateBlockGPUTask::collect(GPUTaskContext &ctx) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_DSTACK();

	RenderingDevice &rd = ctx.rendering_device;
	GPUStorageBufferPool &storage_buffer_pool = ctx.storage_buffer_pool;

	StdVector<GenerateBlockGPUTaskResult> results;
	results.reserve(_boxes_data.size());

	// 获取该特定任务的 span
	Span<const uint8_t> outputs_bytes = to_span(ctx.downloaded_shared_output_data)
												.sub(ctx.shared_output_buffer_begin, ctx.shared_output_buffer_size);

	unsigned int box_offset = 0;

	for (unsigned int box_index = 0; box_index < _boxes_data.size(); ++box_index) {
		BoxData &bd = _boxes_data[box_index];
		const Box3i box = boxes_to_generate[box_index];

		// 目前每个输出的大小都相同
		const unsigned int size_per_output = Vector3iUtil::get_volume_u64(box.size) * sizeof(float);

		for (unsigned int output_index = 0; output_index < generator_shader_outputs->outputs.size(); ++output_index) {
			const VoxelGenerator::ShaderOutput &output_info = generator_shader_outputs->outputs[output_index];

			GenerateBlockGPUTaskResult result(
					box,
					output_info.type,
					// 获取该特定输出的 span
					outputs_bytes.sub(box_offset + size_per_output * output_index, size_per_output),
					// 传递对后备缓冲区的引用，使其在所有消费者用完之前保持有效
					ctx.downloaded_shared_output_data
			);

			results.push_back(result);
		}

		box_offset += size_per_output * generator_shader_outputs->outputs.size();

		storage_buffer_pool.recycle(bd.params_sb);
	}

	voxel::godot::free_rendering_device_rid(rd, _generator_pipeline_rid);

	for (const RID &rid : _modifier_pipelines) {
		voxel::godot::free_rendering_device_rid(rd, rid);
	}

	for (const RID &rid : _uniform_sets_to_free) {
		voxel::godot::free_rendering_device_rid(rd, rid);
	}

	// 我们把转换留给 CPU 任务，因为 GPU 工作只有一个线程，它只用于等待
	// 阻塞函数，而不是用来做实际工作
	consumer_task->set_gpu_results(std::move(results));

	// 恢复网格生成任务，把所有权交还给任务运行器。
	VoxelEngine::get_singleton().push_async_task(consumer_task);
	consumer_task = nullptr;
}

} // namespace voxel
