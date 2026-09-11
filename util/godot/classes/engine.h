#ifndef VOXEL_GODOT_ENGINE_H
#define VOXEL_GODOT_ENGINE_H

#include <core/config/engine.h>

namespace voxel::godot {

inline void add_singleton(const char *name, Object *object) {
	Engine::get_singleton()->add_singleton(Engine::Singleton(name, object));
}

inline void remove_singleton(const char *name) {
	Engine::get_singleton()->remove_singleton(name);
}

} // namespace voxel::godot

#endif // VOXEL_GODOT_ENGINE_H
