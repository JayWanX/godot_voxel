#include "rendering_server.h"
#include "../../errors.h"
#include "../../string/format.h"
#include "../core/string.h"
#include "../core/version.h"
#include "project_settings.h"

namespace voxel::godot {

void get_shader_parameter_list(const RID &shader_rid, StdVector<ShaderParameterInfo> &out_parameters) {
#if defined(VOXEL_GODOT)
	List<PropertyInfo> params;
	RenderingServer::get_singleton()->get_shader_parameter_list(shader_rid, &params);
	// 我本想使用 ConstIterator，因为我只读取该列表，但那是不可能的 :shrug:
	for (List<PropertyInfo>::Iterator it = params.begin(); it != params.end(); ++it) {
		const PropertyInfo property = *it;
		ShaderParameterInfo pi;
		pi.type = property.type;
		pi.name = property.name;
		out_parameters.push_back(pi);
	}

#endif
}

String get_current_rendering_method_name() {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 4
	RenderingServer *rs = RenderingServer::get_singleton();
	// `tests=yes` 时 RenderingServer 可能为 null。
	VOXEL_ASSERT_RETURN_V(rs != nullptr, "");

	const String method_name = rs->get_current_rendering_method();
	return method_name;

#else
	// 参见 https://github.com/godotengine/godot/pull/85430

#if defined(VOXEL_GODOT)
	OS *os = OS::get_singleton();
	VOXEL_ASSERT_RETURN_V(os != nullptr, "");
	return os->get_current_rendering_method();

#endif

#endif
}

RenderMethod get_current_rendering_method() {
	const String name = get_current_rendering_method_name();

	if (name == "forward_plus") {
		return RENDER_METHOD_FORWARD_PLUS;
	}
	if (name == "mobile") {
		return RENDER_METHOD_MOBILE;
	}
	if (name == "gl_compatibility") {
		return RENDER_METHOD_GL_COMPATIBILITY;
	}

	VOXEL_PRINT_WARNING(format("Rendering method {} is unknown", name));
	return RENDER_METHOD_UNKNOWN;
}

String get_current_rendering_driver_name() {
#if GODOT_VERSION_MAJOR == 4 && GODOT_VERSION_MINOR >= 4
	RenderingServer *rs = RenderingServer::get_singleton();
	// `tests=yes` 时 RenderingServer 可能为 null。
	VOXEL_ASSERT_RETURN_V(rs != nullptr, "");

	const String driver_name = rs->get_current_rendering_driver_name();
	return driver_name;

#else
	// 参见 https://github.com/godotengine/godot/pull/85430

#if defined(VOXEL_GODOT)
	OS *os = OS::get_singleton();
	VOXEL_ASSERT_RETURN_V(os != nullptr, "");
	return os->get_current_rendering_driver_name();

#endif

#endif
}

RenderDriverName get_current_rendering_driver() {
	const String name = get_current_rendering_driver_name();

	if (name == "vulkan") {
		return RENDER_DRIVER_VULKAN;
	}
	if (name == "d3d12") {
		return RENDER_DRIVER_D3D12;
	}
	if (name == "metal") {
		return RENDER_DRIVER_METAL;
	}
	if (name == "opengl3") {
		return RENDER_DRIVER_OPENGL3;
	}
	if (name == "opengl3_es") {
		return RENDER_DRIVER_OPENGL3_ES;
	}
	if (name == "opengl3_angle") {
		return RENDER_DRIVER_OPENGL3_ANGLE;
	}

	VOXEL_PRINT_WARNING(format("Rendering driver {} is unknown", name));
	return RENDER_DRIVER_UNKNOWN;
}

RenderThreadModel get_render_thread_model(const ProjectSettings &settings) {
	const int rendering_thread_model = settings.get("rendering/driver/threads/thread_model");
	return static_cast<RenderThreadModel>(rendering_thread_model);
}

bool is_render_thread_model_safe(const RenderThreadModel mode) {
	switch (mode) {
		case RENDER_SEPARATE_THREAD:
		case RENDER_THREAD_SAFE:
			return true;
		case RENDER_THREAD_UNSAFE:
			return false;
		default:
			VOXEL_PRINT_ERROR("Unhandled enum value");
			return false;
	}
}

} // namespace voxel::godot
