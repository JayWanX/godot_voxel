#include "voxel_tool_terrain.h"
#include "../meshers/blocky/voxel_mesher_blocky.h"
#include "../meshers/cubes/voxel_mesher_cubes.h"
#include "../storage/metadata/voxel_metadata_variant.h"
#include "../storage/voxel_buffer_gd.h"
#include "../storage/voxel_data.h"
#include "../terrain/fixed_lod/voxel_terrain.h"
#include "../util/godot/classes/ref_counted.h"
#include <core/variant/array.h>
#include "../util/godot/core/packed_arrays.h"
#include "../util/math/conv.h"
#include "raycast.h"
#include <core/variant/array.h>

using namespace voxel::godot;

namespace voxel {

VoxelToolTerrain::VoxelToolTerrain() {
	_random.randomize();
}

VoxelToolTerrain::VoxelToolTerrain(VoxelTerrain *terrain) {
	ERR_FAIL_COND(terrain == nullptr);
	_terrain = terrain;
	// 当体素工具仍引用地形时，不要销毁地形
}

bool VoxelToolTerrain::is_area_editable(const Box3i &box) const {
	ERR_FAIL_COND_V(_terrain == nullptr, false);
	// TODO 需要考虑体积边界
	return _terrain->get_storage().is_area_loaded(box);
}

Ref<VoxelRaycastResult> VoxelToolTerrain::raycast(
		Vector3 p_pos,
		Vector3 p_dir,
		float p_max_distance,
		uint32_t p_collision_mask
) {
	return raycast_generic_world(
			_terrain->get_storage(),
			_terrain->get_mesher(),
			_terrain->get_global_transform(),
			p_pos,
			p_dir,
			p_max_distance,
			p_collision_mask,
			0,
			_raycast_normal_enabled
	);
}

void VoxelToolTerrain::copy(
		const Vector3i pos,
		VoxelBuffer &dst,
		const uint8_t p_channels_mask,
		const bool with_metadata
) const {
	ERR_FAIL_COND(_terrain == nullptr);
	const uint8_t channels_mask = (p_channels_mask == 0) ? (1 << _channel) : p_channels_mask;
	_terrain->get_storage().copy(pos, dst, channels_mask, with_metadata);
}

void VoxelToolTerrain::paste(Vector3i pos, const VoxelBuffer &src, uint8_t channels_mask) {
	ERR_FAIL_COND(_terrain == nullptr);
	if (channels_mask == 0) {
		channels_mask = (1 << _channel);
	}
	_terrain->get_storage().paste(pos, src, channels_mask, false, true);
	_post_edit(Box3i(pos, src.get_size()));
}

void VoxelToolTerrain::paste_masked(
		Vector3i pos,
		Ref<godot::VoxelBuffer> p_voxels,
		uint8_t channels_mask,
		uint8_t mask_channel,
		uint64_t mask_value
) {
	ERR_FAIL_COND(_terrain == nullptr);
	ERR_FAIL_COND(p_voxels.is_null());
	if (channels_mask == 0) {
		channels_mask = (1 << _channel);
	}
	_terrain->get_storage().paste_masked(pos, p_voxels->get_buffer(), channels_mask, mask_channel, mask_value, false);
	_post_edit(Box3i(pos, p_voxels->get_buffer().get_size()));
}

void VoxelToolTerrain::paste_masked_writable_list(
		Vector3i pos,
		Ref<godot::VoxelBuffer> p_voxels,
		uint8_t channels_mask,
		uint8_t src_mask_channel,
		uint64_t src_mask_value,
		uint8_t dst_mask_channel,
		PackedInt32Array dst_writable_list
) {
	ERR_FAIL_COND(_terrain == nullptr);
	ERR_FAIL_COND(p_voxels.is_null());
	if (channels_mask == 0) {
		channels_mask = (1 << _channel);
	}
	_terrain->get_storage().paste_masked_writable_list(
			pos,
			p_voxels->get_buffer(),
			channels_mask,
			src_mask_channel,
			src_mask_value,
			dst_mask_channel,
			to_span(dst_writable_list),
			false
	);
	_post_edit(Box3i(pos, p_voxels->get_buffer().get_size()));
}

void VoxelToolTerrain::do_box(Vector3i begin, Vector3i end) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_terrain == nullptr);

	if (get_channel() != VoxelBuffer::CHANNEL_SDF) {
		// 回退到通用的 do_box，它基本上是在精确边界内进行朴素填充，不过它仍然
		// 比所需的更慢，因为它使用了随机访问。
		// TODO 让通用操作也能做到这一点，且不需要额外的边距和多余的计算
		VoxelTool::do_box(begin, end);
		return;
	}

	ops::DoShapeChunked<ops::SdfAxisAlignedBox, ops::VoxelDataGridAccess> op;
	op.shape.center = to_vec3f(begin + end) * 0.5f;
	op.shape.half_size = to_vec3f(end - begin) * 0.5f;
	op.shape.sdf_scale = get_sdf_scale();
	op.box = op.shape.get_box().clipped(_terrain->get_bounds());
	op.mode = ops::Mode(get_mode());
	op.texture_params = _texture_params;
	op.blocky_value = _value;
	op.channel = get_channel();
	op.strength = get_sdf_strength();

	if (!is_area_editable(op.box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelData &data = _terrain->get_storage();

	VoxelDataGrid grid;
	data.get_blocks_grid(grid, op.box, 0);
	op.block_access.grid = &grid;

	{
		VoxelDataGrid::LockWrite wlock(grid);
		op();
	}

	_post_edit(op.box);
}

void VoxelToolTerrain::do_sphere(Vector3 center, float radius) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_terrain == nullptr);

	ops::DoSphere op;
	op.shape.center = to_vec3f(center);
	op.shape.radius = radius;
	op.shape.sdf_scale = get_sdf_scale();
	op.box = op.shape.get_box().clipped(_terrain->get_bounds());
	op.mode = ops::Mode(get_mode());
	op.texture_params = _texture_params;
	op.blocky_value = _value;
	op.channel = get_channel();
	op.strength = get_sdf_strength();

	if (!is_area_editable(op.box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelData &data = _terrain->get_storage();

	data.get_blocks_grid(op.blocks, op.box, 0);
	op();

	_post_edit(op.box);
}

void VoxelToolTerrain::do_hemisphere(Vector3 center, float radius, Vector3 flat_direction, float smoothness) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_terrain == nullptr);

	ops::DoShapeChunked<ops::SdfHemisphere, ops::VoxelDataGridAccess> op;
	op.shape.center = to_vec3f(center);
	op.shape.radius = radius;
	op.shape.flat_direction = to_vec3f(flat_direction);
	op.shape.plane_d = flat_direction.dot(center);
	op.shape.smoothness = smoothness;
	op.shape.sdf_scale = get_sdf_scale();
	op.box = op.shape.get_box().clipped(_terrain->get_bounds());
	op.mode = ops::Mode(get_mode());
	op.texture_params = _texture_params;
	op.blocky_value = _value;
	op.channel = get_channel();
	op.strength = get_sdf_strength();

	if (!is_area_editable(op.box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelData &data = _terrain->get_storage();

	VoxelDataGrid grid;
	data.get_blocks_grid(grid, op.box, 0);
	op.block_access.grid = &grid;

	{
		VoxelDataGrid::LockWrite wlock(grid);
		op();
	}

	_post_edit(op.box);
}

uint64_t VoxelToolTerrain::_get_voxel(Vector3i pos) const {
	ERR_FAIL_COND_V(_terrain == nullptr, 0);
	VoxelSingleValue defval;
	defval.i = 0;
	return _terrain->get_storage().get_voxel(pos, _channel, defval).i;
}

float VoxelToolTerrain::_get_voxel_f(Vector3i pos) const {
	ERR_FAIL_COND_V(_terrain == nullptr, 0);
	return _terrain->get_storage().get_voxel_f(pos, _channel);
}

void VoxelToolTerrain::_set_voxel(Vector3i pos, uint64_t v) {
	ERR_FAIL_COND(_terrain == nullptr);
	_terrain->get_storage().try_set_voxel(v, pos, _channel);
}

void VoxelToolTerrain::_set_voxel_f(Vector3i pos, float v) {
	ERR_FAIL_COND(_terrain == nullptr);
	_terrain->get_storage().try_set_voxel_f(v, pos, _channel);
}

void VoxelToolTerrain::_post_edit(const Box3i &box) {
	ERR_FAIL_COND(_terrain == nullptr);
	_terrain->post_edit_area(box, true);
}

void VoxelToolTerrain::set_voxel_metadata(const Vector3i pos, const Variant &meta) {
	ERR_FAIL_COND(_terrain == nullptr);
	VoxelData &data = _terrain->get_storage();
	data.set_voxel_metadata(pos, meta);
	_terrain->post_edit_area(Box3i(pos, Vector3i(1, 1, 1)), false);
}

Variant VoxelToolTerrain::get_voxel_metadata(const Vector3i pos) const {
	ERR_FAIL_COND_V(_terrain == nullptr, Variant());
	VoxelData &data = _terrain->get_storage();
	return data.get_voxel_metadata(pos);
}

namespace {
Ref<VoxelBlockyLibraryBase> get_voxel_library(const VoxelTerrain &terrain) {
	Ref<VoxelMesherBlocky> blocky_mesher = terrain.get_mesher();
	if (blocky_mesher.is_valid()) {
		return blocky_mesher->get_library();
	}
	return Ref<VoxelBlockyLibraryBase>();
}
} // namespace

// TODO 这个函数将给定 AABB 对齐到区块，这不够直观。应该想办法尊重
// 区域。它对所提供的区域内随机体素执行函数（使用 type 通道），从而可以实现
// 如 Minecraft 中那样缓慢的“自然”元胞自动机行为。
void VoxelToolTerrain::run_blocky_random_tick(
		const AABB voxel_area,
		const int voxel_count,
		const Callable &callback,
		const int batch_count,
		const uint32_t tags_mask
) {
	VOXEL_PROFILE_SCOPE();

	ERR_FAIL_COND(_terrain == nullptr);
	ERR_FAIL_COND_MSG(
			get_voxel_library(*_terrain).is_null(),
			String("This function requires a volume using {0} with a valid library")
					.format(varray(VoxelMesherBlocky::get_class_static()))
	);
	ERR_FAIL_COND(callback.is_null());
	ERR_FAIL_COND(batch_count <= 0);
	ERR_FAIL_COND(voxel_count < 0);
	ERR_FAIL_COND(!math::is_valid_size(voxel_area.size));

	if (voxel_count == 0) {
		return;
	}

	const VoxelBlockyLibraryBase &lib = **get_voxel_library(*_terrain);
	VoxelData &data = _terrain->get_storage();

	voxel::run_blocky_random_tick(
			data, voxel_area, lib, _random, voxel_count, batch_count, tags_mask, callback
	);
}

void VoxelToolTerrain::for_each_voxel_metadata_in_area(AABB voxel_area, const Callable &callback) {
	ERR_FAIL_COND(_terrain == nullptr);
	ERR_FAIL_COND(callback.is_null());
	ERR_FAIL_COND(!math::is_valid_size(voxel_area.size));

	const Box3i voxel_box = Box3i(math::floor_to_int(voxel_area.position), math::floor_to_int(voxel_area.size));
	ERR_FAIL_COND(!is_area_editable(voxel_box));

	const Box3i data_block_box = voxel_box.downscaled(_terrain->get_data_block_size());

	VoxelData &data = _terrain->get_storage();

	data_block_box.for_each_cell([&data, &callback, voxel_box](Vector3i block_pos) {
		std::shared_ptr<VoxelBuffer> voxels_ptr = data.try_get_block_voxels(block_pos);

		if (voxels_ptr == nullptr) {
			return;
		}

		const Vector3i block_origin = block_pos * data.get_block_size();
		const Box3i rel_voxel_box(voxel_box.position - block_origin, voxel_box.size);
		// TODO 为元数据锁定区块是否值得？
		// 用于读取还是写入？我们必须将其作为参数指定并信任用户……因为元数据可能包含
		// 引用类型。

		voxels_ptr->for_each_voxel_metadata_in_area(
				rel_voxel_box, [&callback, block_origin](Vector3i rel_pos, const VoxelMetadata &meta) {
					const Variant v = godot::get_as_variant(meta);
					const Vector3i key = rel_pos + block_origin;
					const Variant key_v = key;
					const Variant *args[2] = { &key_v, &v };
					Callable::CallError err;
					Variant retval; // 我们并不关心返回值，但 Callable API 要求提供它
					callback.callp(args, 2, retval, err);

					ERR_FAIL_COND_MSG(
							err.error != Callable::CallError::CALL_OK,
							String("Callable failed at {0}").format(varray(key))
					);
				}
		);
	});
}

void VoxelToolTerrain::do_path(Span<const Vector3> positions, Span<const float> radii) {
	VOXEL_ASSERT_RETURN(_terrain != nullptr);
	do_path_chunked(_terrain->get_storage(), positions, radii, false);
}

#ifdef VOXEL_ENABLE_MESH_SDF
void VoxelToolTerrain::do_mesh(const VoxelMeshSDF &mesh_sdf, const Transform3D &transform, const float isolevel) {
	VOXEL_ASSERT_RETURN(_terrain != nullptr);
	do_mesh_chunked(mesh_sdf, _terrain->get_storage(), transform, isolevel, false);
}
#endif

VoxelFormat VoxelToolTerrain::get_format() const {
	VOXEL_ASSERT(_terrain != nullptr);
	return _terrain->get_storage().get_format();
}

void VoxelToolTerrain::_bind_methods() {
	ClassDB::bind_method(
			D_METHOD("run_blocky_random_tick", "area", "voxel_count", "callback", "batch_count", "tags_mask"),
			&VoxelToolTerrain::run_blocky_random_tick,
			DEFVAL(16),
			DEFVAL(0xffffffff)
	);
	ClassDB::bind_method(
			D_METHOD("for_each_voxel_metadata_in_area", "voxel_area", "callback"),
			&VoxelToolTerrain::for_each_voxel_metadata_in_area
	);
	ClassDB::bind_method(
			D_METHOD("do_hemisphere", "center", "radius", "flat_direction", "smoothness"),
			&VoxelToolTerrain::do_hemisphere,
			DEFVAL(0.0)
	);
}

} // namespace voxel
