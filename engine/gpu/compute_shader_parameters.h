#ifndef VOXEL_COMPUTE_SHADER_PARAMETERS_H
#define VOXEL_COMPUTE_SHADER_PARAMETERS_H

#include "../../util/containers/std_vector.h"
#include "compute_shader_resource.h"
#include <memory>

namespace voxel {

struct ComputeShaderParameter {
	unsigned int binding = 0;
	std::shared_ptr<ComputeShaderResource> resource;
};

// 相当于我们在体素引擎中使用的计算着色器的"材质"
struct ComputeShaderParameters {
	StdVector<ComputeShaderParameter> params;
};

void add_uniform_params(const StdVector<ComputeShaderParameter> &params, Array &uniforms, const RID filtering_sampler);

} // namespace voxel

#endif // VOXEL_COMPUTE_SHADER_PARAMETERS_H
