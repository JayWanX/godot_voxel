#ifndef VOXEL_GODOT_ENGINE_H
#define VOXEL_GODOT_ENGINE_H

#if defined(VOXEL_GODOT)
#include <core/config/engine.h>
#endif

namespace voxel::godot {

inline void add_singleton(const char *name, Object *object) {
#if defined(VOXEL_GODOT)
	Engine::get_singleton()->add_singleton(Engine::Singleton(name, object));
#endif
}

inline void remove_singleton(const char *name) {
#if defined(VOXEL_GODOT)
	Engine::get_singleton()->remove_singleton(name);
#endif
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_ENGINE_H
