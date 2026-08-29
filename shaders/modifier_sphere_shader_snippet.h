// 生成的文件

// clang-format off
const char *g_modifier_sphere_shader_snippet =
"\n"
"layout (set = 0, binding = 5) restrict readonly buffer ShapeParams {\n"
"	// 不需要中心点，变换在公共着色器代码中应用\n"
"	//vec3 center;\n"
"	float radius;\n"
"} u_shape_params;\n"
"\n"
"float get_sd(vec3 pos) {\n"
"	return length(pos) - u_shape_params.radius;\n"
"}\n"
"\n";
// clang-format on
