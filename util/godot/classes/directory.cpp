#include "directory.h"
#include "../../containers/std_vector.h"

namespace voxel::godot {

Error erase_directory_contents_recursive(DirAccess &da) {
#ifdef VOXEL_GODOT
	return da.erase_contents_recursive();

#endif
}

} // namespace voxel::godot
