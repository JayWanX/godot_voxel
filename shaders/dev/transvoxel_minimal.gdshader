shader_type spatial;

// 来自 Voxel Tools API
uniform int u_transition_mask;

float get_transvoxel_secondary_factor(int idata) {
	int transition_mask = u_transition_mask & 0xff;

	int cell_border_mask = idata & 63; // 单元格接触的边
	int vertex_border_mask = (idata >> 8) & 63; // 顶点接触的边
	// 如果顶点靠近存在低分辨率邻居的边，
	// 将其移动到次位置
	int m = transition_mask & cell_border_mask;
	float t = float(m != 0);
	// 如果顶点位于一条或多条边上，且至少有一条边没有低分辨率邻居，
	// 则不移动顶点。
	t *= float((vertex_border_mask & ~transition_mask) == 0);
	
	// 调试
	//t *= 0.5 + 0.5 * sin(TIME * 4.0);
	//t *= 2.0;

	return t;
}

vec3 get_transvoxel_position(vec3 vertex_pos, vec4 fdata) {
	int idata = floatBitsToInt(fdata.a);

	// 移动顶点以实现平滑过渡
	float secondary_factor = get_transvoxel_secondary_factor(idata);
	vec3 secondary_position = fdata.xyz;
	vec3 pos = mix(vertex_pos, secondary_position, secondary_factor);

	// 如果网格合并了过渡且顶点属于某个过渡，
	// 当该过渡未激活时，我们改变顶点的位置，使
	// 所有三角形退化且不可见。
	// 这是单独渲染它们的替代方案，
	// 可减少绘制调用，也减少需要在 Godot 中创建的网格资源。
	// 理想情况下，我会像 LOD 那样调整索引缓冲区，但 Godot 并未
	// 提供任何以这种方式使用它的接口。
	int itransition = (idata >> 16) & 0xff; // 顶点是否在过渡网格上？
	float transition_cull = float(itransition == 0 || (itransition & u_transition_mask) != 0);
	pos *= transition_cull;

	return pos;
}

void vertex() {
	VERTEX = get_transvoxel_position(VERTEX, CUSTOM0);
}
