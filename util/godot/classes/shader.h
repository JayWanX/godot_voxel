#ifndef VOXEL_GODOT_SHADER_H
#define VOXEL_GODOT_SHADER_H

#if defined(VOXEL_GODOT)
#include <scene/resources/shader.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/shader.hpp>
using namespace godot;
#endif

#ifdef TOOLS_ENABLED

#include "../../containers/span.h"

namespace voxel::godot {

// TODO Cannot use `Shader.has_uniform()` because it is unreliable.
// See https://github.com/godotengine/godot/issues/64467
bool shader_has_uniform(const Shader &shader, StringName uniform_name);

String get_missing_uniform_names(Span<const StringName> expected_uniforms, const Shader &shader);

} // namespace voxel::godot

#endif

#endif // VOXEL_GODOT_SHADER_H
