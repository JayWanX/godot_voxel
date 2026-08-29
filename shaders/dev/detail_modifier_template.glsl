#[compute]
#version 450

// 接收一组位置，并在其周围 4 个位置求值有符号距离场。
// 选择这 4 个位置，使结果可用于计算梯度。
// 随后使用指定的操作将结果叠加到先前的值上。

// 对于给定区块，此着色器可能针对我们可能组合的每个体素数据源被多次派发。

layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout (set = 0, binding = 0, std430) restrict readonly buffer PositionBuffer {
	// X、Y、Z 为命中位置
	// W 为整数三角形索引
	vec4 values[];
} u_positions;

layout (set = 0, binding = 1, std430) restrict readonly buffer DetailParams {
	int tile_size_pixels;
	float pixel_world_step;
} u_detail_params;

layout (set = 0, binding = 2, std430) restrict readonly buffer InSDBuffer {
	// 每个索引 4 个值
	float values[];
} u_in_sd;

layout (set = 0, binding = 3, std430) restrict writeonly buffer OutSDBuffer {
	// 每个索引 4 个值
	float values[];
} u_out_sd;

// 所有修改器共用的参数
layout (set = 0, binding = 4, std430) restrict readonly buffer BaseModifierParams {
	mat4 world_to_model;
	int operation;
	float smoothness;
	float sd_scale;
} u_base_modifier_params;

// <PLACEHOLDER>
float get_sd(vec3 pos) {
	return 0.0;
}
// </PLACEHOLDER>

float sd_smooth_union(float a, float b, float s) {
	const float h = clamp(0.5 + 0.5 * (b - a) / s, 0.0, 1.0);
	return mix(b, a, h) - s * h * (1.0 - h);
}

// 交换 a 和 b，因为它是从 SDF b 中减去 SDF a
float sd_smooth_subtract(float b, float a, float s) {
	const float h = clamp(0.5 - 0.5 * (b + a) / s, 0.0, 1.0);
	return mix(b, -a, h) + s * h * (1.0 - h);
}

void main() {
	const ivec2 pixel_pos_in_tile = ivec2(gl_GlobalInvocationID.xy);
	const int tile_index = int(gl_GlobalInvocationID.z);

	const int index = pixel_pos_in_tile.x + pixel_pos_in_tile.y * u_detail_params.tile_size_pixels 
		+ tile_index * u_detail_params.tile_size_pixels * u_detail_params.tile_size_pixels;

	if (u_positions.values[index].w == -1.0) {
		return;
	}

	vec3 pos0 = u_positions.values[index].xyz;
	vec3 pos1 = pos0 + vec3(u_detail_params.pixel_world_step, 0.0, 0.0);
	vec3 pos2 = pos0 + vec3(0.0, u_detail_params.pixel_world_step, 0.0);
	vec3 pos3 = pos0 + vec3(0.0, 0.0, u_detail_params.pixel_world_step);

	pos0 = (u_base_modifier_params.world_to_model * vec4(pos0, 1.0)).xyz;
	pos1 = (u_base_modifier_params.world_to_model * vec4(pos1, 1.0)).xyz;
	pos2 = (u_base_modifier_params.world_to_model * vec4(pos2, 1.0)).xyz;
	pos3 = (u_base_modifier_params.world_to_model * vec4(pos3, 1.0)).xyz;

	const int sdi = index * 4;

	float sd0 = u_in_sd.values[sdi];
	float sd1 = u_in_sd.values[sdi + 1];
	float sd2 = u_in_sd.values[sdi + 2];
	float sd3 = u_in_sd.values[sdi + 3];

	const float msd0 = get_sd(pos0) * u_base_modifier_params.sd_scale;
	const float msd1 = get_sd(pos1) * u_base_modifier_params.sd_scale;
	const float msd2 = get_sd(pos2) * u_base_modifier_params.sd_scale;
	const float msd3 = get_sd(pos3) * u_base_modifier_params.sd_scale;

	if (u_base_modifier_params.operation == 0) {
		sd0 = sd_smooth_union(sd0, msd0, u_base_modifier_params.smoothness);
		sd1 = sd_smooth_union(sd1, msd1, u_base_modifier_params.smoothness);
		sd2 = sd_smooth_union(sd2, msd2, u_base_modifier_params.smoothness);
		sd3 = sd_smooth_union(sd3, msd3, u_base_modifier_params.smoothness);

	} else if (u_base_modifier_params.operation == 1) {
		sd0 = sd_smooth_subtract(sd0, msd0, u_base_modifier_params.smoothness);
		sd1 = sd_smooth_subtract(sd1, msd1, u_base_modifier_params.smoothness);
		sd2 = sd_smooth_subtract(sd2, msd2, u_base_modifier_params.smoothness);
		sd3 = sd_smooth_subtract(sd3, msd3, u_base_modifier_params.smoothness);

	} else {
		sd0 = msd0;
		sd1 = msd1;
		sd2 = msd2;
		sd3 = msd3;
	}

	u_out_sd.values[sdi] = sd0;
	u_out_sd.values[sdi + 1] = sd1;
	u_out_sd.values[sdi + 2] = sd2;
	u_out_sd.values[sdi + 3] = sd3;
}
