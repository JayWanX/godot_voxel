#[compute]
#version 450

// <SNIPPET>

layout (set = 0, binding = 5) restrict readonly buffer ShapeParams {
	// 不需要中心点，变换在公共着色器代码中应用
	//vec3 center;
	float radius;
} u_shape_params;

float get_sd(vec3 pos) {
	return length(pos) - u_shape_params.radius;
}

// </SNIPPET>

void main() {}

