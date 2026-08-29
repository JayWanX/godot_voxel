#include "voxel_generator_script.h"
#include "../constants/voxel_string_names.h"
#include "../storage/voxel_buffer_gd.h"
#include "../util/godot/check_ref_ownership.h"
#include "../util/godot/classes/engine.h"
#include "../util/godot/classes/script.h"

#ifdef VOXEL_GODOT
#include "../util/godot/core/class_db.h"
#endif

namespace voxel {

VoxelGeneratorScript::VoxelGeneratorScript() {}

VoxelGenerator::Result VoxelGeneratorScript::generate_block(VoxelGenerator::VoxelQueryData input) {
	Result result;

	// 创建一个临时包装器，以便 Godot 可以把它传给脚本
	Ref<godot::VoxelBuffer> buffer_wrapper(
			memnew(godot::VoxelBuffer(static_cast<godot::VoxelBuffer::Allocator>(input.voxel_buffer.get_allocator())))
	);
	buffer_wrapper.instantiate();
	buffer_wrapper->get_buffer().copy_format(input.voxel_buffer);
	buffer_wrapper->get_buffer().create(input.voxel_buffer.get_size());

	{
		VOXEL_GODOT_CHECK_REF_COUNT_DOES_NOT_CHANGE(buffer_wrapper);
		if (!GDVIRTUAL_CALL(_generate_block, buffer_wrapper, input.origin_in_voxels, input.lod)) {
			WARN_PRINT_ONCE("VoxelGeneratorScript::_generate_block is unimplemented!");
		}
	}

	// 包装器将被丢弃
	buffer_wrapper->get_buffer().move_to(input.voxel_buffer);

	// 等到它真正被用到的那一天，我们可能会把它暴露给脚本
	// if (ret.get_type() == Variant::DICTIONARY) {
	// 	Dictionary d = ret;
	// 	result.max_lod_hint = d.get("max_lod_hint", false);
	// }

	return result;
}

int VoxelGeneratorScript::get_used_channels_mask() const {
	int mask = 0;
	if (!GDVIRTUAL_CALL(_get_used_channels_mask, mask)) {
		WARN_PRINT_ONCE("VoxelGeneratorScript::_get_used_channels_mask is unimplemented!");
	}
	return mask;
}

bool VoxelGeneratorScript::is_runnable() const {
	Ref<Script> my_script = get_script();
	if (my_script.is_null()) {
		return false;
	}
	if (Engine::get_singleton()->is_editor_hint()) {
		return my_script->is_tool();
	}
	return true;
}

void VoxelGeneratorScript::_bind_methods() {
	GDVIRTUAL_BIND(_generate_block, "out_buffer", "origin_in_voxels", "lod");
	GDVIRTUAL_BIND(_get_used_channels_mask);
}

} // namespace voxel
