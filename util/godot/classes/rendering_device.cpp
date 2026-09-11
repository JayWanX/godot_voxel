#include "rendering_device.h"
#include "../../dstack.h"
#include "../../profiling.h"
#include <core/version.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <core/version.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>
#include <servers/rendering/rendering_device_binds.h>

namespace voxel::godot {

void free_rendering_device_rid(RenderingDevice &rd, RID rid) {
	VOXEL_DSTACK();

	rd.free_rid(rid);

}

Ref<RDShaderSPIRV> shader_compile_spirv_from_source(RenderingDevice &rd, RDShaderSource &p_source, bool p_allow_cache) {
	// 这是 `RenderingDevice::_shader_compile_spirv_from_source` 的副本，因为它是私有的

	Ref<RDShaderSPIRV> bytecode;
	bytecode.instantiate();
	for (int i = 0; i < RenderingDevice::SHADER_STAGE_MAX; i++) {
		String error;

		RenderingDevice::ShaderStage stage = RenderingDevice::ShaderStage(i);
		String source = p_source.get_stage_source(stage);

		if (!source.is_empty()) {
			Vector<uint8_t> spirv =
					rd.shader_compile_spirv_from_source(stage, source, p_source.get_language(), &error, p_allow_cache);
			bytecode->set_stage_bytecode(stage, spirv);
			bytecode->set_stage_compile_error(stage, error);
		}
	}
	return bytecode;

}

PackedByteArray shader_compile_binary_from_spirv(RenderingDevice &rd, RDShaderSPIRV &p_spirv, String name) {
	// 这是 `RenderingDevice::_shader_compile_binary_from_spirv` 的副本，因为它是私有的。

	Vector<RenderingDevice::ShaderStageSPIRVData> stage_data;
	for (int i = 0; i < RD::SHADER_STAGE_MAX; i++) {
		RenderingDevice::ShaderStage stage = RenderingDevice::ShaderStage(i);

		String error = p_spirv.get_stage_compile_error(stage);
		ERR_FAIL_COND_V_MSG(
				!error.is_empty(),
				PackedByteArray(),
				"Can't create a shader from an errored bytecode. Check errors in source bytecode."
		);

		PackedByteArray bytecode = p_spirv.get_stage_bytecode(stage);
		if (bytecode.is_empty()) {
			continue;
		}

		RenderingDevice::ShaderStageSPIRVData sd;
		sd.shader_stage = stage;
		sd.spirv = bytecode;
		stage_data.push_back(sd);
	}

	return rd.shader_compile_binary_from_spirv(stage_data, name);

}

RID texture_create(
		RenderingDevice &rd,
		RDTextureFormat &p_format,
		RDTextureView &p_view,
		const TypedArray<PackedByteArray> &p_data
) {
	// 这是 `RenderingDevice::_texture_create` 的部分重新实现，因为它是私有的

	Vector<Vector<uint8_t>> data;
	for (int i = 0; i < p_data.size(); i++) {
		Vector<uint8_t> byte_slice = p_data[i];
		ERR_FAIL_COND_V(byte_slice.is_empty(), RID());
		data.push_back(byte_slice);
	}

	// 无法从 `RDTextureFormat` 访问 `base`，因为它是私有的，所以我只能在这里手动重建……
	RenderingDevice::TextureFormat tf;
	tf.width = p_format.get_width();
	tf.height = p_format.get_height();
	tf.depth = p_format.get_depth();
	tf.mipmaps = p_format.get_mipmaps();
	tf.samples = p_format.get_samples();
	tf.array_layers = p_format.get_array_layers();
	tf.texture_type = p_format.get_texture_type();
	tf.usage_bits = p_format.get_usage_bits();
	tf.format = p_format.get_format();

	// 无法从 `RDTextureView` 访问 `base`，因为它是私有的，所以我只能在这里手动重建……
	RenderingDevice::TextureView tv;
	tv.format_override = p_view.get_format_override();
	tv.swizzle_r = p_view.get_swizzle_r();
	tv.swizzle_g = p_view.get_swizzle_g();
	tv.swizzle_b = p_view.get_swizzle_b();
	tv.swizzle_a = p_view.get_swizzle_a();

	return rd.texture_create(tf, tv, data);

}

RID uniform_set_create(RenderingDevice &rd, Array uniforms, RID shader, int shader_set) {
	VOXEL_PROFILE_SCOPE();
	// 无法访问那个接收 `Array` 的该方法版本，因为它是私有的……
	return rd.call(SNAME("uniform_set_create"), uniforms, shader, shader_set);

}

RID sampler_create(RenderingDevice &rd, const RDSamplerState &sampler_state) {
	// 无法访问那个接收 `RDSamplerState` 对象的该方法版本，因为它是私有的……

	// return rd.call(SNAME("sampler_create"), sampler_state_ref);

	RenderingDevice::SamplerState ss;
	ss.mag_filter = sampler_state.get_mag_filter();
	ss.min_filter = sampler_state.get_min_filter();
	ss.mip_filter = sampler_state.get_mip_filter();
	ss.repeat_u = sampler_state.get_repeat_u();
	ss.repeat_v = sampler_state.get_repeat_v();
	ss.repeat_w = sampler_state.get_repeat_w();
	ss.lod_bias = sampler_state.get_lod_bias();
	ss.use_anisotropy = sampler_state.get_use_anisotropy();
	ss.anisotropy_max = sampler_state.get_anisotropy_max();
	ss.enable_compare = sampler_state.get_enable_compare();
	ss.compare_op = sampler_state.get_compare_op();
	ss.min_lod = sampler_state.get_min_lod();
	ss.max_lod = sampler_state.get_max_lod();
	ss.border_color = sampler_state.get_border_color();
	ss.unnormalized_uvw = sampler_state.get_unnormalized_uvw();

	return rd.sampler_create(ss);

}

Error update_storage_buffer(
		RenderingDevice &rd,
		RID rid,
		unsigned int offset,
		unsigned int size,
		const PackedByteArray &pba
) {

	return rd.buffer_update(rid, offset, size, pba.ptr());

}

} // namespace voxel::godot
