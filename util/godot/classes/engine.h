#ifndef VOXEL_GODOT_ENGINE_H
#define VOXEL_GODOT_ENGINE_H

#if defined(VOXEL_GODOT)
#include <core/config/engine.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/classes/engine.hpp>
using namespace godot;
#endif

namespace voxel::godot {

inline void add_singleton(const char *name, Object *object) {
#if defined(VOXEL_GODOT)
	Engine::get_singleton()->add_singleton(Engine::Singleton(name, object));
#elif defined(VOXEL_GODOT_EXTENSION)
	Engine::get_singleton()->register_singleton(StringName(name), object);
#endif
}

inline void remove_singleton(const char *name) {
#if defined(VOXEL_GODOT)
	Engine::get_singleton()->remove_singleton(name);
#elif defined(VOXEL_GODOT_EXTENSION)
	Engine::get_singleton()->unregister_singleton(name);
#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_ENGINE_H
