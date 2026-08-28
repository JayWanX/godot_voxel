#ifndef VOXEL_GODOT_RECT2I_H
#define VOXEL_GODOT_RECT2I_H

#if defined(VOXEL_GODOT)
#include <core/math/rect2i.h>
#elif defined(VOXEL_GODOT_EXTENSION)
#include <godot_cpp/variant/rect2i.hpp>
using namespace godot;
#endif

namespace voxel {

class TextWriter;
TextWriter &operator<<(TextWriter &w, const Rect2i &box);

} // namespace voxel

#endif // VOXEL_GODOT_RECT2I_H
