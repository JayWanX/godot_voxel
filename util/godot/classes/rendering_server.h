#include <core/version.h>
#ifndef VOXEL_GODOT_RENDERING_SERVER_H
#define VOXEL_GODOT_RENDERING_SERVER_H

#include <core/version.h>

#include <servers/rendering/rendering_server.h>

// RenderingServer 的枚举现在位于一个名为 RenderingServerEnums 的独立命名空间中
#include <servers/rendering/rendering_server_enums.h>


#include "../../containers/std_vector.h"
#include "../macros.h"

class ProjectSettings;

namespace voxel::godot {

inline void free_rendering_server_rid(RenderingServer &rs, const RID &rid) {
	rs.free_rid(rid);

}

#ifdef VOXEL_ENABLE_GPU
inline bool supports_rendering_device() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs == nullptr) {
		return false;
	}

	return rs->get_rendering_device() != nullptr;
}
#endif

struct ShaderParameterInfo {
	String name;
	Variant::Type type;
};

void get_shader_parameter_list(const RID &shader_rid, StdVector<ShaderParameterInfo> &out_parameters);

String get_current_rendering_method_name();

// 与 ProjectSettings 和 RenderingServer 中使用的字符串等价的枚举。
enum RenderMethod {
	RENDER_METHOD_FORWARD_PLUS,
	RENDER_METHOD_MOBILE,
	RENDER_METHOD_GL_COMPATIBILITY,
	RENDER_METHOD_UNKNOWN,
};

RenderMethod get_current_rendering_method();

String get_current_rendering_driver_name();

enum RenderDriverName {
	RENDER_DRIVER_VULKAN,
	RENDER_DRIVER_D3D12,
	RENDER_DRIVER_METAL,
	RENDER_DRIVER_OPENGL3,
	RENDER_DRIVER_OPENGL3_ES,
	RENDER_DRIVER_OPENGL3_ANGLE,
	RENDER_DRIVER_UNKNOWN,
};

RenderDriverName get_current_rendering_driver();

// `ProjectSettings.rendering/driver/threads/thread_model` 的枚举等价物。
// 遗憾的是它没有暴露出来。
enum RenderThreadModel {
	RENDER_THREAD_UNSAFE,
	RENDER_THREAD_SAFE,
	RENDER_SEPARATE_THREAD,
};

RenderThreadModel get_render_thread_model(const ProjectSettings &settings);

// 说明从主线程以外的线程调用 RenderingServer 的函数是否安全。
bool is_render_thread_model_safe(const RenderThreadModel mode);

} // namespace voxel::godot

#endif // VOXEL_GODOT_RENDERING_SERVER_H
