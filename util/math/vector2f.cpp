#include "vector2f.h"
#include "../io/text_writer.h"

namespace voxel {

TextWriter &operator<<(TextWriter &w, const Vector2f &v) {
	w << "(" << v.x << ", " << v.y << ")";
	return w;
}

} // namespace voxel
