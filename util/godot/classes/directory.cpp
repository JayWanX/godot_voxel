#include "directory.h"
#include "../../containers/std_vector.h"

namespace voxel::godot {

Error erase_directory_contents_recursive(DirAccess &da) {
	return da.erase_contents_recursive();

}

} // namespace voxel::godot
