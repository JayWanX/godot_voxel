#ifndef VOXEL_GODOT_ARRAY_MESH_H
#define VOXEL_GODOT_ARRAY_MESH_H

#if defined(VOXEL_GODOT)
#include <scene/resources/mesh.h>
#endif

namespace voxel::godot {

// TODO 以下函数本应能在 `Mesh` 上工作，
// 但脚本/扩展 API 只把某些方法暴露在 `ArrayMesh` 上，尽管它们内部存在于 `Mesh` 上……

// TODO 我需要在 `Mesh` 层面用廉价的方式检查这个，但看起来需要获取表面数组，
// 而那可能并不便宜……
inline bool is_mesh_empty(const ArrayMesh &mesh) {
	if (mesh.get_surface_count() == 0) {
		return true;
	}
	if (mesh.surface_get_array_len(0) == 0) {
		return true;
	}
	return false;
}

#ifdef TOOLS_ENABLED

// 生成一个线框网格，高亮三角网格中顶点未共享的边。
// 用于调试。
Array generate_debug_seams_wireframe_surface(const ArrayMesh &src_mesh, int surface_index);

#endif

} // namespace voxel::godot

#endif // VOXEL_GODOT_ARRAY_MESH_H
