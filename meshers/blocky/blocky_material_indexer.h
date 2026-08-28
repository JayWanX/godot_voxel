#ifndef VOXEL_BLOCKY_MATERIAL_INDEXER_H
#define VOXEL_BLOCKY_MATERIAL_INDEXER_H

#include "../../util/containers/std_vector.h"
#include "../../util/godot/classes/ref_counted.h"
#include "../../util/godot/macros.h"

VOXEL_GODOT_FORWARD_DECLARE(class Material);

namespace voxel::blocky {

struct MaterialIndexer {
	StdVector<Ref<Material>> &materials;

	unsigned int get_or_create_index(const Ref<Material> &p_material);
};

} // namespace voxel::blocky

#endif // VOXEL_BLOCKY_MATERIAL_INDEXER_H
