#include "voxel_stream_script.h"
#include "../constants/voxel_string_names.h"
#include "../storage/voxel_buffer_gd.h"
#include "../util/godot/check_ref_ownership.h"
#include "../util/godot/classes/engine.h"
#include <core/object/script_language.h>
#include <core/object/script_language.h>
#include <core/object/class_db.h>


namespace voxel {

void VoxelStreamScript::load_voxel_block(VoxelStream::VoxelQueryData &query_data) {
	// 创建临时包装器，以便 Godot 将其传给脚本
	Ref<godot::VoxelBuffer> buffer_wrapper(memnew(
			godot::VoxelBuffer(static_cast<godot::VoxelBuffer::Allocator>(query_data.voxel_buffer.get_allocator()))
	));
	buffer_wrapper->get_buffer().copy_format(query_data.voxel_buffer);
	buffer_wrapper->get_buffer().create(query_data.voxel_buffer.get_size());

	query_data.result = RESULT_ERROR;

	VOXEL_GODOT_CHECK_REF_COUNT_DOES_NOT_CHANGE(buffer_wrapper);

	int res;
	if (GDVIRTUAL_CALL(_load_voxel_block, buffer_wrapper, query_data.position_in_blocks, query_data.lod_index, res)) {
		// 检查返回的枚举是否有效
		ERR_FAIL_INDEX(res, _RESULT_COUNT);
		// 若找到该数据块，则从面向脚本的对象中取出其数据存入内部缓冲区
		if (res == RESULT_BLOCK_FOUND) {
			buffer_wrapper->get_buffer().move_to(query_data.voxel_buffer);
		}
		query_data.result = ResultCode(res);
	} else {
		// 该函数未找到或失败了？
		WARN_PRINT_ONCE("VoxelStreamScript::_load_voxel_block is unimplemented!");
	}
}

void VoxelStreamScript::save_voxel_block(VoxelStream::VoxelQueryData &query_data) {
	// 目前被调用方可例外地取得此包装器的所有权，因为我们会将数据复制到其中。
	Ref<godot::VoxelBuffer> buffer_wrapper(memnew(
			godot::VoxelBuffer(static_cast<godot::VoxelBuffer::Allocator>(query_data.voxel_buffer.get_allocator()))
	));
	query_data.voxel_buffer.copy_to(buffer_wrapper->get_buffer(), true);
	if (!GDVIRTUAL_CALL(_save_voxel_block, buffer_wrapper, query_data.position_in_blocks, query_data.lod_index)) {
		WARN_PRINT_ONCE("VoxelStreamScript::_save_voxel_block is unimplemented!");
	}
}

int VoxelStreamScript::get_used_channels_mask() const {
	int mask = 0;
	if (!GDVIRTUAL_CALL(_get_used_channels_mask, mask)) {
		WARN_PRINT_ONCE("VoxelStreamScript::_get_used_channels_mask is unimplemented!");
	}
	return mask;
}

bool VoxelStreamScript::is_runnable() const {
	Ref<Script> my_script = get_script();
	if (my_script.is_null()) {
		return false;
	}
	if (Engine::get_singleton()->is_editor_hint()) {
		return my_script->is_tool();
	}
	return true;
}

void VoxelStreamScript::_bind_methods() {
	// TODO 测试 GDScript 在其它线程中失败时，GDVIRTUAL 是否能正确打印错误。
	GDVIRTUAL_BIND(_load_voxel_block, "out_buffer", "position_in_blocks", "lod");
	GDVIRTUAL_BIND(_save_voxel_block, "buffer", "position_in_blocks", "lod");
	GDVIRTUAL_BIND(_get_used_channels_mask);
}

} // namespace voxel
