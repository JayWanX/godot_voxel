#include <core/math/vector3.h>
#include "../io/text_writer.h"

namespace voxel {

TextWriter &operator<<(TextWriter &w, const Vector3 &v) {
	w << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	return w;
}

} // namespace voxel
