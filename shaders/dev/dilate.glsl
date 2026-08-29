#[compute]
#version 450

// 通过用周围像素的平均值填充“空”像素来膨胀法线贴图。
// 假定输入图像是分块的：膨胀不会跨块交互。
// 该着色器运行一次将膨胀 1 个像素。

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// 我们必须交替使用图像，因为每次迭代都会读取相邻像素
layout (set = 0, binding = 0, rgba8ui) restrict readonly uniform uimage2D u_src_image;
layout (set = 0, binding = 1, rgba8ui) restrict writeonly uniform uimage2D u_dst_image;

layout (set = 0, binding = 2) uniform Params {
	int u_tile_size;
};

void main() {
	// 此颜色对应空法线。
	const ivec4 nocol = ivec4(127, 127, 127, 255);
	const ivec2 pixel_pos = ivec2(gl_GlobalInvocationID.xy);

	const ivec4 col11 = ivec4(imageLoad(u_src_image, pixel_pos));
	if (col11 != nocol) {
		imageStore(u_dst_image, pixel_pos, col11);
		return;
	}

	//const ivec2 im_size = imageSize(u_src_image).xy;

	const ivec2 p01 = pixel_pos + ivec2(-1, 0);
	const ivec2 p21 = pixel_pos + ivec2(1, 0);
	const ivec2 p10 = pixel_pos + ivec2(0, -1);
	const ivec2 p12 = pixel_pos + ivec2(0, 1);

	ivec4 col_sum = ivec4(0,0,0,0);
	int count = 0;

	const ivec4 col01 = ivec4(imageLoad(u_src_image, p01));
	// 不要采样与当前 tile 不同的 tile 的像素。
	// 这也能处理图像边界，但对于负向边界我们必须更明确地处理，
	// 因为除法的工作方式如此
	if (col01 != nocol && pixel_pos.x != 0 && (pixel_pos.x - 1) / u_tile_size == pixel_pos.x / u_tile_size) {
		col_sum += col01;
		++count;
	}

	const ivec4 col21 = ivec4(imageLoad(u_src_image, p21));
	if (col21 != nocol && (pixel_pos.x + 1) / u_tile_size == pixel_pos.x / u_tile_size) {
		col_sum += col21;
		++count;
	}

	const ivec4 col10 = ivec4(imageLoad(u_src_image, p10));
	if (col10 != nocol && pixel_pos.y != 0 && (pixel_pos.y - 1) / u_tile_size == pixel_pos.y / u_tile_size) {
		col_sum += col10;
		++count;
	}

	const ivec4 col12 = ivec4(imageLoad(u_src_image, p12));
	if (col12 != nocol && (pixel_pos.y + 1) / u_tile_size == pixel_pos.y / u_tile_size) {
		col_sum += col12;
		++count;
	}

	ivec4 col_avg = count == 0 ? col11 : col_sum / count;

	imageStore(u_dst_image, pixel_pos, col_avg);
}
