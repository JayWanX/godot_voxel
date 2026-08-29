// 生成的文件

// clang-format off
const char *g_block_generator_shader_template_0 =
"#version 450\n"
"\n"
"layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;\n"
"\n"
"layout (set = 0, binding = 0, std430) restrict readonly buffer Params {\n"
"	vec3 origin_in_voxels;\n"
"	float voxel_size;\n"
"	ivec3 block_size;\n"
"	int buffer_offset;\n"
"} u_params;\n"
"\n"
"// 包含所有输出，每个输出以相同大小的连续块排列。\n"
"// 必须从 `u_params.buffer_offset` 开始索引。\n"
"layout (set = 0, binding = 1, std430) restrict writeonly buffer OutBuffer {\n"
"	float values[];\n"
"} u_out;\n"
"\n";
// clang-format on

// clang-format off
const char *g_block_generator_shader_template_1 =
"\n"
"int get_zxy_index(ivec3 pos, ivec3 size) {\n"
"	return pos.y + size.y * (pos.x + size.x * pos.z);\n"
"}\n"
"\n"
"int get_volume(ivec3 v) {\n"
"	return v.x * v.y * v.z;\n"
"}\n"
"\n"
"void main() {\n"
"	const ivec3 rpos = ivec3(gl_GlobalInvocationID.xyz);\n"
"	// 输出缓冲区可能没有与工作组大小成倍数的 3D 尺寸。\n"
"	// 一些并行执行将不会做任何事。\n"
"	if (rpos.x >= u_params.block_size.x || rpos.y >= u_params.block_size.y || rpos.z >= u_params.block_size.z) {\n"
"		return;\n"
"	}\n"
"\n"
"	const int out_index = get_zxy_index(rpos, u_params.block_size) + u_params.buffer_offset;\n"
"\n"
"	// 可能被生成代码用于有多个输出的生成器\n"
"	const int volume = get_volume(u_params.block_size);\n"
"\n"
"	const vec3 wpos = u_params.origin_in_voxels + vec3(rpos) * u_params.voxel_size;\n"
"	// float sd = get_sd(wpos);\n"
"	// u_out_sd.values[out_index] = sd;\n"
"\n";
// clang-format on

// clang-format off
const char *g_block_generator_shader_template_2 =
"}\n";
// clang-format on
