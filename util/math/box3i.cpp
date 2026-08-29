#include "box3i.h"
#include "../io/text_writer.h"

namespace voxel {

TextWriter &operator<<(TextWriter &w, const Box3i &box) {
	// TODO 不知为何单行版本无法编译？
	w << "(o:";
	w << box.position;
	w << ", s:";
	w << box.size;
	w << ")";
	return w;
}

} // namespace voxel
