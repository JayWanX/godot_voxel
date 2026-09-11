#include "voxel_generator.h"
#include "../constants/voxel_string_names.h"
#include "../shaders/shaders.h"
#include "../storage/voxel_buffer_gd.h"
#include <core/variant/array.h>
#include <core/object/class_db.h>
#include "../util/profiling.h"
#include "generate_block_task.h"

#ifdef VOXEL_ENABLE_GPU
#include "../engine/gpu/compute_shader.h"
#include "../engine/gpu/compute_shader_parameters.h"
#endif

namespace voxel {

VoxelGenerator::VoxelGenerator() {}

VoxelGenerator::Result VoxelGenerator::generate_block(VoxelQueryData input) {
	return Result();
}

IThreadedTask *VoxelGenerator::create_block_task(const BlockTaskParams &params) const {
	// 默认通用任务
	return VOXEL_NEW(GenerateBlockTask(params));
}

int VoxelGenerator::get_used_channels_mask() const {
	return 0;
}

VoxelSingleValue VoxelGenerator::generate_single(Vector3i pos, unsigned int channel) {
	VoxelSingleValue v;
	v.i = 0;
	VOXEL_ASSERT_RETURN_V(channel < VoxelBuffer::MAX_CHANNELS, v);
	// 默认的慢速实现
	// TODO 优化：慢的一部分是由分配器造成的。
	// 对于如此小且频繁调用的尺寸来说，使用 `VoxelMemoryPool` 并不合适。
	// 改用临时分配器或者栈上分配会更快。
	VoxelBuffer buffer(VoxelBuffer::ALLOCATOR_POOL);
	buffer.create(1, 1, 1);
	VoxelQueryData q{ buffer, pos, 0 };
	generate_block(q);
	if (channel == VoxelBuffer::CHANNEL_SDF) {
		v.f = buffer.get_voxel_f(0, 0, 0, channel);
	} else {
		v.i = buffer.get_voxel(0, 0, 0, channel);
	}
	return v;
}

void VoxelGenerator::generate_series(
		Span<const float> positions_x,
		Span<const float> positions_y,
		Span<const float> positions_z,
		unsigned int channel,
		Span<float> out_values,
		Vector3f min_pos,
		Vector3f max_pos
) {
	VOXEL_PRINT_ERROR("Not implemented");
}

void VoxelGenerator::_b_generate_block(Ref<godot::VoxelBuffer> out_buffer, Vector3 origin_in_voxels, int lod) {
	ERR_FAIL_COND(lod < 0);
	ERR_FAIL_COND(lod >= int(constants::MAX_LOD));
	ERR_FAIL_COND(out_buffer.is_null());
	VoxelQueryData q = { out_buffer->get_buffer(), origin_in_voxels, uint8_t(lod) };
	generate_block(q);
}

#ifdef VOXEL_ENABLE_GPU

bool VoxelGenerator::get_shader_source(ShaderSourceData &out_params) const {
	VOXEL_PRINT_ERROR("Not implemented");
	return false;
}

std::shared_ptr<ComputeShader> VoxelGenerator::get_detail_rendering_shader() {
	{
		MutexLock mlock(_shader_mutex);
		return _detail_rendering_shader;
	}
}

std::shared_ptr<ComputeShaderParameters> VoxelGenerator::get_detail_rendering_shader_parameters() {
	{
		MutexLock mlock(_shader_mutex);
		return _detail_rendering_shader_parameters;
	}
}

std::shared_ptr<ComputeShader> VoxelGenerator::get_block_rendering_shader() {
	{
		MutexLock mlock(_shader_mutex);
		return _block_rendering_shader;
	}
}

std::shared_ptr<ComputeShaderParameters> VoxelGenerator::get_block_rendering_shader_parameters() {
	{
		MutexLock mlock(_shader_mutex);
		return _block_rendering_shader_parameters;
	}
}

std::shared_ptr<VoxelGenerator::ShaderOutputs> VoxelGenerator::get_block_rendering_shader_outputs() {
	{
		MutexLock mlock(_shader_mutex);
		return _block_rendering_shader_outputs;
	}
}

namespace {

void append_generator_parameter_uniforms(
		String &source_text,
		ComputeShaderParameters &out_params,
		VoxelGenerator::ShaderSourceData &shader_data,
		const unsigned int bindings_start
) {
	for (unsigned int i = 0; i < shader_data.parameters.size(); ++i) {
		VoxelGenerator::ShaderParameter &p = shader_data.parameters[i];
		const unsigned int binding = bindings_start + i;
		VOXEL_ASSERT(p.resource->get_type() == ComputeShaderResourceInternal::TYPE_TEXTURE_2D);
		source_text +=
				String("layout (set = 0, binding = {0}) uniform sampler2D {1};\n").format(varray(binding, p.name));
		out_params.params.push_back(ComputeShaderParameter{ binding, p.resource });
	}
	source_text += "\n";
}

} // namespace

std::shared_ptr<ComputeShader> compile_detail_rendering_compute_shader(
		VoxelGenerator &generator,
		ComputeShaderParameters &out_params
) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND_V_MSG(
			!generator.supports_shaders(),
			ComputeShaderFactory::create_invalid(),
			String("Can't use the provided {0} with compute shaders, it does not support GLSL.")
					.format(varray(VoxelGenerator::get_class_static()))
	);

	VoxelGenerator::ShaderSourceData shader_data;
	ERR_FAIL_COND_V_MSG(
			!generator.get_shader_source(shader_data),
			ComputeShaderFactory::create_invalid(),
			"Failed to get shader source code."
	);

	String source_text;
	// 只有在这里我们才能确定 binding 的值，无法更早确定
	const unsigned int generator_uniform_binding_start = 3;
	{
		// 头
		source_text += g_detail_generator_shader_template_0;

		append_generator_parameter_uniforms(source_text, out_params, shader_data, generator_uniform_binding_start);

		// 生成器代码
		source_text += shader_data.glsl;

		// 生成包装器，只使用一个输出，并适配细节渲染模板所期望的函数名
		{
			source_text += "float get_sd(vec3 pos) {\n";
			int sdf_output_index = -1;
			for (unsigned int output_index = 0; output_index < shader_data.outputs.size(); ++output_index) {
				const VoxelGenerator::ShaderOutput &output = shader_data.outputs[output_index];
				if (output.type == VoxelGenerator::ShaderOutput::TYPE_SDF) {
					sdf_output_index = output_index;
				}
				source_text += String("\tfloat v{0};\n").format(varray(output_index));
			}
			ERR_FAIL_COND_V_MSG(
					sdf_output_index == -1,
					ComputeShaderFactory::create_invalid(),
					"Can't generate detail generator shader, SDF output not found"
			);
			// 调用生成器着色器函数
			source_text += "\tgenerate(pos";
			for (unsigned int output_index = 0; output_index < shader_data.outputs.size(); ++output_index) {
				source_text += String(", v{0}").format(varray(output_index));
			}
			source_text += ");\n";
			source_text += String("\treturn v{0};\n}\n").format(varray(sdf_output_index));
		}

		// 尾
		source_text += g_detail_generator_shader_template_1;
	}

	// TODO 想办法为不同的生成器选择不同的名称
	std::shared_ptr<ComputeShader> shader =
			ComputeShaderFactory::create_from_glsl(source_text, "voxel.detail_generator.gen");

	return shader;
}

std::shared_ptr<ComputeShader> compile_block_rendering_compute_shader(
		VoxelGenerator &generator,
		ComputeShaderParameters &out_params,
		VoxelGenerator::ShaderOutputs &outputs
) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND_V_MSG(
			!generator.supports_shaders(),
			ComputeShaderFactory::create_invalid(),
			String("Can't use the provided {0} with compute shaders, it does not support GLSL.")
					.format(varray(VoxelGenerator::get_class_static()))
	);

	VoxelGenerator::ShaderSourceData shader_data;
	ERR_FAIL_COND_V_MSG(
			!generator.get_shader_source(shader_data),
			ComputeShaderFactory::create_invalid(),
			"Failed to get shader source code."
	);

	String source_text;
	const unsigned int generator_uniform_binding_start = 2;

	source_text += g_block_generator_shader_template_0;

	append_generator_parameter_uniforms(source_text, out_params, shader_data, generator_uniform_binding_start);

	for (unsigned int output_index = 0; output_index < shader_data.outputs.size(); ++output_index) {
		const VoxelGenerator::ShaderOutput &output = shader_data.outputs[output_index];
		outputs.outputs.push_back(output);
	}

	// 生成器代码
	source_text += shader_data.glsl;

	// main() 的头
	source_text += g_block_generator_shader_template_1;

	// 调用生成器函数
	{
		source_text += "\tgenerate(wpos";
		for (unsigned int output_index = 0; output_index < shader_data.outputs.size(); ++output_index) {
			// TODO 也许我们应该能够打包输出，而不是总是使用 float？
			// TODO 也许交错输出会因为数据局部性而更高效？
			source_text += String(", u_out.values[out_index + volume * {0}]").format(varray(output_index));
		}
		source_text += ");\n";
	}

	// main() 的尾
	source_text += g_block_generator_shader_template_2;

	// TODO 想办法为不同的生成器选择不同的名称
	std::shared_ptr<ComputeShader> shader =
			ComputeShaderFactory::create_from_glsl(source_text, "voxel.block_generator.gen");

	return shader;
}

void VoxelGenerator::compile_shaders() {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(!supports_shaders());
	VOXEL_PRINT_VERBOSE("Compiling compute shaders for virtual rendering");

	std::shared_ptr<ComputeShaderParameters> detail_params = make_shared_instance<ComputeShaderParameters>();
	std::shared_ptr<ComputeShader> detail_render_shader =
			compile_detail_rendering_compute_shader(*this, *detail_params);

	std::shared_ptr<ComputeShaderParameters> block_params = make_shared_instance<ComputeShaderParameters>();
	std::shared_ptr<ShaderOutputs> block_outputs = make_shared_instance<ShaderOutputs>();
	std::shared_ptr<ComputeShader> block_render_shader =
			compile_block_rendering_compute_shader(*this, *block_params, *block_outputs);

	{
		MutexLock mlock(_shader_mutex);

		_detail_rendering_shader = detail_render_shader;
		_detail_rendering_shader_parameters = detail_params;

		_block_rendering_shader = block_render_shader;
		_block_rendering_shader_parameters = block_params;
		_block_rendering_shader_outputs = block_outputs;
	}
}

void VoxelGenerator::invalidate_shaders() {
	{
		MutexLock mlock(_shader_mutex);

		_detail_rendering_shader.reset();
		_detail_rendering_shader_parameters.reset();

		_block_rendering_shader.reset();
		_block_rendering_shader_parameters.reset();
		_block_rendering_shader_outputs.reset();
	}
}

#endif

bool VoxelGenerator::generate_broad_block(VoxelQueryData input) {
	// 默认情况下，生成器不单独支持这一点，而是在 `generate_block` 内部完成。
	// 但如果生成器支持 GPU，则建议实现它。
	return false;
}

void VoxelGenerator::process_viewer_diff(ViewerID viewer_id, Box3i p_requested_box, Box3i p_prev_requested_box) {
	// 可选地在子类中实现
}

void VoxelGenerator::clear_cache() {
	// 可选地在子类中实现
}

bool VoxelGenerator::is_runnable() const {
	return true;
}

void VoxelGenerator::_bind_methods() {
	ClassDB::bind_method(
			D_METHOD("generate_block", "out_buffer", "origin_in_voxels", "lod"), &VoxelGenerator::_b_generate_block
	);
}

} // namespace voxel
