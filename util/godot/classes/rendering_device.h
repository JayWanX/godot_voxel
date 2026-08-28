#ifndef VOXEL_GODOT_RENDERING_DEVICE_H
#define VOXEL_GODOT_RENDERING_DEVICE_H

#if defined(VOXEL_GODOT)
#include <servers/rendering/rendering_device.h>
#endif

#include "../macros.h"
#include "rd_shader_spirv.h"

VOXEL_GODOT_FORWARD_DECLARE(class RDShaderSource)

namespace voxel::godot {

void free_rendering_device_rid(RenderingDevice &rd, RID rid);

// The script-facing RenderingDevice API differs from the module one, so reimplement here.

Ref<RDShaderSPIRV> shader_compile_spirv_from_source(RenderingDevice &rd, RDShaderSource &p_source, bool p_allow_cache);
PackedByteArray shader_compile_binary_from_spirv(RenderingDevice &rd, RDShaderSPIRV &p_spirv, String name = "");
RID texture_create(
		RenderingDevice &rd,
		RDTextureFormat &p_format,
		RDTextureView &p_view,
		const TypedArray<PackedByteArray> &p_data
);
RID uniform_set_create(RenderingDevice &rd, Array uniforms, RID shader, int shader_set);
RID sampler_create(RenderingDevice &rd, const RDSamplerState &sampler_state);
Error update_storage_buffer(
		RenderingDevice &rd,
		RID rid,
		unsigned int offset,
		unsigned int size,
		const PackedByteArray &pba
);

} // namespace voxel::godot

#endif // VOXEL_GODOT_RENDERING_DEVICE_H
