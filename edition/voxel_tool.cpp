#include "voxel_tool.h"

#include "../storage/voxel_buffer_gd.h"
#include "../storage/voxel_data.h"
#include "../util/godot/core/packed_arrays.h"
#include "../util/io/log.h"
#include "../util/math/color8.h"
#include "../util/math/conv.h"
#include "../util/profiling.h"
#include "funcs.h"

#ifdef VOXEL_GODOT
#include "../util/godot/core/class_db.h"
#endif

#ifdef VOXEL_ENABLE_MESH_SDF
#include "voxel_mesh_sdf_gd.h"
#endif

#ifdef DEBUG_ENABLED
#include "../util/godot/core/aabb.h"
#include "../util/string/format.h"
#endif

namespace voxel {

VoxelTool::VoxelTool() {}

void VoxelTool::set_value(uint64_t val) {
	_value = val;
}

uint64_t VoxelTool::get_value() const {
	return _value;
}

void VoxelTool::set_eraser_value(uint64_t value) {
	_eraser_value = value;
}

uint64_t VoxelTool::get_eraser_value() const {
	return _eraser_value;
}

void VoxelTool::set_channel(VoxelBuffer::ChannelId p_channel) {
	ERR_FAIL_INDEX(p_channel, VoxelBuffer::MAX_CHANNELS);
	_channel = p_channel;
}

VoxelBuffer::ChannelId VoxelTool::get_channel() const {
	return _channel;
}

void VoxelTool::set_mode(Mode mode) {
	_mode = mode;
}

VoxelTool::Mode VoxelTool::get_mode() const {
	return _mode;
}

float VoxelTool::get_sdf_scale() const {
	return _sdf_scale;
}

void VoxelTool::set_sdf_scale(float s) {
	_sdf_scale = math::max(s, 0.00001f);
}

void VoxelTool::set_texture_index(int ti) {
	ERR_FAIL_INDEX(ti, 16);
	_texture_params.index = ti;
}

int VoxelTool::get_texture_index() const {
	return _texture_params.index;
}

void VoxelTool::set_texture_opacity(float opacity) {
	_texture_params.opacity = math::clamp(opacity, 0.f, 1.f);
}

float VoxelTool::get_texture_opacity() const {
	return _texture_params.opacity;
}

void VoxelTool::set_texture_falloff(float falloff) {
	_texture_params.sharpness = 1.f / math::clamp(falloff, 0.001f, 1.f);
}

void VoxelTool::set_sdf_strength(float strength) {
	_sdf_strength = math::clamp(strength, 0.f, 1.f);
}

float VoxelTool::get_sdf_strength() const {
	return _sdf_strength;
}

float VoxelTool::get_texture_falloff() const {
	ERR_FAIL_COND_V(_texture_params.sharpness < 1.f, 1.f);
	return 1.f / _texture_params.sharpness;
}

Ref<VoxelRaycastResult> VoxelTool::raycast(Vector3 pos, Vector3 dir, float max_distance, uint32_t collision_mask) {
	ERR_PRINT("Not implemented");
	return Ref<VoxelRaycastResult>();
	// 具体实现请参见派生类
}

void VoxelTool::set_raycast_normal_enabled(bool enabled) {
	_raycast_normal_enabled = enabled;
}

uint64_t VoxelTool::get_voxel(Vector3i pos) const {
	return _get_voxel(pos);
}

float VoxelTool::get_voxel_f(Vector3i pos) const {
	return _get_voxel_f(pos);
}

float VoxelTool::get_voxel_f_interpolated(const Vector3 pos) const {
	// 默认的、较慢的实现
	return get_sdf_interpolated([this](Vector3i ipos) { return _get_voxel_f(ipos); }, pos);
}

void VoxelTool::set_voxel(Vector3i pos, uint64_t v) {
	Box3i box(pos, Vector3i(1, 1, 1));
	if (!is_area_editable(box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}
	_set_voxel(pos, v);
	_post_edit(box);
}

void VoxelTool::set_voxel_f(Vector3i pos, float v) {
	Box3i box(pos, Vector3i(1, 1, 1));
	if (!is_area_editable(box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}
	_set_voxel_f(pos, v);
	_post_edit(box);
}

void VoxelTool::do_point(Vector3i pos) {
	Box3i box(pos, Vector3i(1, 1, 1));
	if (!is_area_editable(box)) {
		return;
	}
	if (_channel == VoxelBuffer::CHANNEL_SDF) {
		// 不是一致的 SDF，但应该能用
		_set_voxel_f(pos, _mode == MODE_REMOVE ? constants::SDF_FAR_OUTSIDE : constants::SDF_FAR_INSIDE);
	} else {
		_set_voxel(pos, _mode == MODE_REMOVE ? _eraser_value : _value);
	}
	_post_edit(box);
}

uint64_t VoxelTool::_get_voxel(Vector3i pos) const {
	ERR_PRINT("Not implemented");
	return 0;
}

float VoxelTool::_get_voxel_f(Vector3i pos) const {
	ERR_PRINT("Not implemented");
	return 0;
}

void VoxelTool::_set_voxel(Vector3i pos, uint64_t v) {
	ERR_PRINT("Not implemented");
}

void VoxelTool::_set_voxel_f(Vector3i pos, float v) {
	ERR_PRINT("Not implemented");
}

// 以下是默认的旧式实现。它们可能比专门化的实现更慢，因此通常会
// 在 VoxelTool 的子类中定义。理想情况下，只有当某个函数在所有专门化子类中都有最优
// 实现时，才应在基类中暴露该函数。

void VoxelTool::do_sphere(Vector3 p_center, float radius) {
	VOXEL_PROFILE_SCOPE();
	// 默认的、次优的实现

	const Box3i box(
			math::floor_to_int(p_center) - Vector3iUtil::create(Math::floor(radius)),
			Vector3iUtil::create(Math::ceil(radius) * 2)
	);

	if (_allow_out_of_bounds == false && !is_area_editable(box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	if (_channel == VoxelBuffer::CHANNEL_SDF) {
		const Vector3f center = to_vec3f(p_center);
		box.for_each_cell([this, center, radius](Vector3i pos) {
			float d = _sdf_scale * voxel::math::sdf_sphere(to_vec3f(pos), center, radius);
			_set_voxel_f(pos, ops::sdf_blend(d, get_voxel_f(pos), static_cast<ops::Mode>(_mode)));
		});

	} else {
		int value = _mode == MODE_REMOVE ? _eraser_value : _value;

		box.for_each_cell([this, p_center, radius, value](Vector3i pos) {
			float d = Vector3(pos).distance_to(p_center);
			if (d <= radius) {
				_set_voxel(pos, value);
			}
		});
	}

	_post_edit(box);
}

// 在提供的缓冲区中存在物质的每个体素处擦除物质。
void VoxelTool::sdf_stamp_erase(Ref<godot::VoxelBuffer> stamp, Vector3i pos) {
	VOXEL_ASSERT_RETURN(stamp.is_valid());
	sdf_stamp_erase(stamp->get_buffer(), pos);
}

void VoxelTool::sdf_stamp_erase(const VoxelBuffer &stamp, Vector3i pos) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND_MSG(get_channel() != VoxelBuffer::CHANNEL_SDF, "This function only works when channel is set to SDF");

	const Box3i box(pos, stamp.get_size());
	if (!is_area_editable(box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	box.for_each_cell_zxy([this, &stamp, pos](Vector3i pos_in_volume) {
		const Vector3i pos_in_stamp = pos_in_volume - pos;
		const float dst_sdf =
				stamp.get_voxel_f(pos_in_stamp.x, pos_in_stamp.y, pos_in_stamp.z, VoxelBuffer::CHANNEL_SDF);
		if (dst_sdf <= 0.f) {
			// 不是一致的 SDF，但应该能正常工作
			_set_voxel_f(pos_in_volume, constants::SDF_FAR_OUTSIDE);
		}
	});

	_post_edit(box);
}

void VoxelTool::do_box(Vector3i begin, Vector3i end) {
	VOXEL_PROFILE_SCOPE();
	// 默认的、次优的实现

	Vector3iUtil::sort_min_max(begin, end);
	const Box3i box = Box3i::from_min_max(begin, end + Vector3i(1, 1, 1));

	if (_allow_out_of_bounds == false && !is_area_editable(box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	if (_channel == VoxelBuffer::CHANNEL_SDF) {
		// TODO 提高质量
		// 不是一致的 SDF，但应该能正常工作
		box.for_each_cell([this](Vector3i pos) {
			_set_voxel_f(pos, sdf_blend(constants::SDF_FAR_INSIDE, get_voxel_f(pos), static_cast<ops::Mode>(_mode)));
		});

	} else {
		const int value = _mode == MODE_REMOVE ? _eraser_value : _value;
		box.for_each_cell([this, value](Vector3i pos) { _set_voxel(pos, value); });
	}

	_post_edit(box);
}

void VoxelTool::do_path(Span<const Vector3> positions, Span<const float> radii) {
	ERR_PRINT("Not implemented");
	// 在派生类中实现
}

#ifdef VOXEL_ENABLE_MESH_SDF
void VoxelTool::do_mesh(const VoxelMeshSDF &mesh_sdf, const Transform3D &transform, const float isolevel) {
	ERR_PRINT("Not implemented");
	// 在派生类中实现
}
#endif

void VoxelTool::copy(Vector3i pos, VoxelBuffer &dst, uint8_t channels_mask, const bool with_metadata) const {
	ERR_PRINT("Not implemented");
	// 在派生类中实现
}

void VoxelTool::copy(Vector3i pos, Ref<godot::VoxelBuffer> dst, uint8_t channel_mask, const bool with_metadata) const {
	ERR_FAIL_COND(dst.is_null());
#ifdef TOOLS_ENABLED
	if (Vector3iUtil::is_empty_size(dst->get_size())) {
		VOXEL_PRINT_WARNING("The passed buffer has an empty size, nothing will be copied.");
	}
#endif
	copy(pos, dst->get_buffer(), channel_mask, with_metadata);
}

void VoxelTool::paste(Vector3i p_pos, const VoxelBuffer &src, uint8_t channels_mask) {
	ERR_PRINT("Not implemented");
	// 在派生类中实现
}

void VoxelTool::paste(Vector3i p_pos, Ref<godot::VoxelBuffer> p_voxels, uint8_t channels_mask) {
	ERR_FAIL_COND(p_voxels.is_null());
	if (Vector3iUtil::is_empty_size(p_voxels->get_size())) {
		VOXEL_PRINT_WARNING("The passed buffer has an empty size, nothing will be pasted.");
	}
	paste(p_pos, p_voxels->get_buffer(), channels_mask);
}

void VoxelTool::paste_masked(
		Vector3i p_pos,
		Ref<godot::VoxelBuffer> p_voxels,
		uint8_t channels_mask,
		uint8_t mask_channel,
		uint64_t mask_value
) {
	ERR_FAIL_COND(p_voxels.is_null());
	ERR_PRINT("Not implemented");
	// 在派生类中实现
}

void VoxelTool::paste_masked_writable_list(
		Vector3i pos,
		Ref<godot::VoxelBuffer> p_voxels,
		uint8_t channels_mask,
		uint8_t src_mask_channel,
		uint64_t src_mask_value,
		uint8_t dst_mask_channel,
		PackedInt32Array dst_writable_list
) {
	VOXEL_PRINT_ERROR("Not implemented");
	// 在派生类中实现
}

void VoxelTool::smooth_sphere(Vector3 sphere_center, float sphere_radius, int blur_radius) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(blur_radius >= 1 && blur_radius <= 64);
	VOXEL_ASSERT_RETURN(sphere_radius >= 0.01f);

	const Box3i voxel_box = Box3i::from_min_max(
			math::floor_to_int(sphere_center - Vector3(sphere_radius, sphere_radius, sphere_radius)),
			math::ceil_to_int(sphere_center + Vector3(sphere_radius, sphere_radius, sphere_radius))
	);

	const Box3i padded_voxel_box = voxel_box.padded(blur_radius);

	if (_allow_out_of_bounds == false && !is_area_editable(padded_voxel_box)) {
		VOXEL_PRINT_VERBOSE("Area not editable");
		return;
	}

	VoxelBuffer buffer(VoxelBuffer::ALLOCATOR_POOL);
	buffer.create(padded_voxel_box.size);

	if (_channel == VoxelBuffer::CHANNEL_SDF) {
		// 注意，这仅适用于 SDF，不会模糊体素纹理数据。

		copy(padded_voxel_box.position, buffer, (1 << VoxelBuffer::CHANNEL_SDF), false);

		VoxelBuffer smooth_buffer(VoxelBuffer::ALLOCATOR_POOL);
		smooth_buffer.copy_format(buffer);
		const Vector3f relative_sphere_center = to_vec3f(sphere_center - to_vec3(voxel_box.position));
		ops::box_blur(buffer, smooth_buffer, blur_radius, relative_sphere_center, sphere_radius);

		paste(voxel_box.position, smooth_buffer, (1 << VoxelBuffer::CHANNEL_SDF));

	} else {
		ERR_PRINT("Not implemented");
	}
}

void VoxelTool::grow_sphere(Vector3 sphere_center, float sphere_radius, float strength) {
	// TODO：未来可能更倾向于使用额外的 “GROW”/“SHRINK” 体素工具模式来替代。
	// 参见：https://github.com/Voxel/godot_voxel/pull/594
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(sphere_radius >= 0.01f);

	const Box3i voxel_box = Box3i::from_min_max(
			math::floor_to_int(sphere_center - Vector3(sphere_radius, sphere_radius, sphere_radius)),
			math::ceil_to_int(sphere_center + Vector3(sphere_radius, sphere_radius, sphere_radius))
	);

	if (_allow_out_of_bounds == false && !is_area_editable(voxel_box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelBuffer buffer(VoxelBuffer::ALLOCATOR_POOL);
	buffer.create(voxel_box.size);

	if (_channel == VoxelBuffer::CHANNEL_SDF) {
		// 注意，这仅适用于 SDF，不会影响体素纹理数据。

		copy(voxel_box.position, buffer, (1 << VoxelBuffer::CHANNEL_SDF), false);

		const Vector3f relative_sphere_center = to_vec3f(sphere_center - to_vec3(voxel_box.position));
		const float signed_strength = _mode == VoxelTool::MODE_REMOVE ? -strength : strength;

		ops::grow_sphere(buffer, _sdf_scale * signed_strength, relative_sphere_center, sphere_radius);

		paste(voxel_box.position, buffer, (1 << VoxelBuffer::CHANNEL_SDF));

	} else {
		ERR_PRINT("Not implemented");
	}
}

bool VoxelTool::is_area_editable(const Box3i &box) const {
	ERR_PRINT("Not implemented");
	return false;
}

void VoxelTool::_post_edit(const Box3i &box) {
	ERR_PRINT("Not implemented");
}

void VoxelTool::set_voxel_metadata(const Vector3i pos, const Variant &meta) {
	ERR_PRINT("Not implemented");
}

Variant VoxelTool::get_voxel_metadata(const Vector3i pos) const {
	ERR_PRINT("Not implemented");
	return Variant();
}

VoxelFormat VoxelTool::get_format() const {
	ERR_PRINT("Not implemented");
	return VoxelFormat();
}

void VoxelTool::do_path_chunked(
		VoxelData &vdata,
		Span<const Vector3> positions,
		Span<const float> radii,
		const bool with_pre_generate
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(positions.size() >= 2);
	VOXEL_ASSERT_RETURN(positions.size() == radii.size());

	// TODO 对平滑体素是否稍微增大边距？
	const int margin = 1;

	// 计算总包围盒

	const AABB total_aabb = get_path_aabb(positions, radii).grow(margin);
	const Box3i total_voxel_box = Box3i::from_min_max(
			to_vec3i(math::floor(total_aabb.position)), to_vec3i(math::ceil(total_aabb.position + total_aabb.size))
	);

	if (!is_area_editable(total_voxel_box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	if (with_pre_generate) {
		vdata.pre_generate_box(total_voxel_box);
	}

	VoxelDataGrid grid;

	vdata.get_blocks_grid(grid, total_voxel_box, 0);

	{
		VoxelDataGrid::LockWrite wlock(grid);

		// 光栅化

		for (unsigned int point_index = 1; point_index < positions.size(); ++point_index) {
			// TODO 可以在局部空间中运行，从而无需使用双精度
			// TODO 应用地形缩放
			const Vector3f p0 = to_vec3f(positions[point_index - 1]);
			const Vector3f p1 = to_vec3f(positions[point_index]);

			const float r0 = radii[point_index - 1];
			const float r1 = radii[point_index];

			ops::DoShapeChunked<ops::SdfRoundCone, ops::VoxelDataGridAccess> op;
			op.block_access.grid = &grid;
			op.shape.cone.a = p0;
			op.shape.cone.b = p1;
			op.shape.cone.r1 = r0;
			op.shape.cone.r2 = r1;
			op.shape.cone.update();
			op.shape.sdf_scale = get_sdf_scale();
			op.box = op.shape.get_box().padded(margin);
			op.mode = ops::Mode(get_mode());
			op.texture_params = _texture_params;
			op.blocky_value = _value;
			op.channel = get_channel();
			op.strength = get_sdf_strength();

			op();

			// 曾试验绘制一条 100 点的路径，锥体计算之外所有操作的开销为：
			// - 非模板版本：2.55 ms
			// - 模板版本：1.00 ms
			// 包含锥体计算时：
			// - 非模板版本：5.7 ms
			// - 模板版本：4.5 ms
			// 因此模板版本更快，但快得不多。
			//
			// math::SdfRoundConePrecalc cone;
			// cone.a = p0;
			// cone.b = p1;
			// cone.r1 = r0;
			// cone.r2 = r1;
			// cone.update();
			// uint64_t value = _value;
			// segment_box.for_each_cell_zxy([&grid, &cone, value](Vector3i pos) {
			// 	if (cone(pos) < 0.f) {
			// 		grid.set_voxel_no_lock(pos, value, VoxelBuffer::CHANNEL_TYPE);
			// 	}
			// });
		}
	}

	_post_edit(total_voxel_box);
}

#ifdef VOXEL_ENABLE_MESH_SDF
void VoxelTool::do_mesh_chunked(
		const VoxelMeshSDF &mesh_sdf,
		VoxelData &vdata,
		const Transform3D &transform,
		const float isolevel,
		const bool with_pre_generate
) {
	VOXEL_PROFILE_SCOPE();

	VOXEL_ASSERT_RETURN(mesh_sdf.is_baked());
	Ref<godot::VoxelBuffer> buffer_ref = mesh_sdf.get_voxel_buffer();
	VOXEL_ASSERT_RETURN(buffer_ref.is_valid());
	const VoxelBuffer &buffer = buffer_ref->get_buffer();
	const VoxelBuffer::ChannelId buffer_channel = VoxelBuffer::CHANNEL_SDF;
	VOXEL_ASSERT_RETURN(buffer.get_channel_compression(buffer_channel) != VoxelBuffer::COMPRESSION_UNIFORM);
	VOXEL_ASSERT_RETURN(buffer.get_channel_depth(buffer_channel) == VoxelBuffer::DEPTH_32_BIT);

	const Transform3D &box_to_world = transform;
	const AABB local_aabb = mesh_sdf.get_aabb();

	// 注意，变换是相对于地形局部的
	const AABB aabb = box_to_world.xform(local_aabb);
	const Box3i voxel_box =
			Box3i::from_min_max(aabb.position.floor(), (aabb.position + aabb.size).ceil()).clipped(vdata.get_bounds());

	// TODO 有时在尚未加载的区块附近会失败，即使变换后的包围盒并未与它们相交。
	// 这可以通过盒体/变换后盒体的相交判定算法来避免。如果确实出现这种使用场景，可以考虑研究一下
	// 的情况。在完全加载模式下不会发生，其他形状也会受此影响。
	if (!is_area_editable(voxel_box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	if (with_pre_generate) {
		vdata.pre_generate_box(voxel_box);
	}

	// TODO 也许将盒体“光栅化”会更高效？我们目前会遍历盒体并未相交的体素。
	// TODO 也许我们也应该根据变换的缩放来缩放 SDF 值
	// TODO 支持其他位深，格式应可从体积中访问

	const Transform3D buffer_to_box =
			Transform3D(Basis().scaled(Vector3(local_aabb.size / buffer.get_size())), local_aabb.position);
	const Transform3D buffer_to_world = box_to_world * buffer_to_box;

	// 让模型变大，也会使有符号距离变大。
	// 不过，非均匀缩放支持得并不好。
	const float size_scale = math::get_largest_coord(transform.get_basis().get_scale());

	ops::DoShapeChunked<ops::SdfBufferShape, ops::VoxelDataGridAccess> op;
	op.shape.world_to_buffer = buffer_to_world.affine_inverse();
	op.shape.buffer_size = buffer.get_size();
	op.shape.isolevel = isolevel;
	op.shape.sdf_scale = get_sdf_scale() * size_scale;
	// 注意，传入的缓冲区不能与另一个线程共享。
	// buffer.decompress_channel(channel);
	VOXEL_ASSERT_RETURN(buffer.get_channel_data_read_only(buffer_channel, op.shape.buffer));
	op.mode = static_cast<ops::Mode>(get_mode());
	op.texture_params = _texture_params;
	op.blocky_value = _value;
	op.channel = get_channel();
	op.strength = get_sdf_strength();
	op.box = voxel_box;

	VoxelDataGrid grid;
	vdata.get_blocks_grid(grid, voxel_box, 0);
	op.block_access.grid = &grid;

	{
		VoxelDataGrid::LockWrite wlock(grid);
		op();
	}

	_post_edit(voxel_box);
}
#endif

// 绑定相关代码

uint64_t VoxelTool::_b_get_voxel(Vector3i pos) {
	return get_voxel(pos);
}

float VoxelTool::_b_get_voxel_f(Vector3i pos) {
	return get_voxel_f(pos);
}

void VoxelTool::_b_set_voxel(Vector3i pos, uint64_t v) {
	set_voxel(pos, v);
}

void VoxelTool::_b_set_voxel_f(Vector3i pos, float v) {
	set_voxel_f(pos, v);
}

Ref<VoxelRaycastResult> VoxelTool::_b_raycast(Vector3 pos, Vector3 dir, float max_distance, uint32_t collision_mask) {
	return raycast(pos, dir, max_distance, collision_mask);
}

void VoxelTool::_b_do_point(Vector3i pos) {
	do_point(pos);
}

void VoxelTool::_b_do_sphere(Vector3 pos, float radius) {
	do_sphere(pos, radius);
}

void VoxelTool::_b_do_box(Vector3i begin, Vector3i end) {
	do_box(begin, end);
}

void VoxelTool::_b_do_path(PackedVector3Array positions, PackedFloat32Array radii) {
	do_path(to_span(positions), to_span(radii));
}

#ifdef VOXEL_ENABLE_MESH_SDF
void VoxelTool::_b_do_mesh(Ref<VoxelMeshSDF> mesh_sdf, Transform3D transform, float isolevel) {
	VOXEL_ASSERT_RETURN(mesh_sdf.is_valid());
	do_mesh(**mesh_sdf, transform, isolevel);
}
#endif

void VoxelTool::_b_copy(Vector3i pos, Ref<godot::VoxelBuffer> voxels, int channel_mask, bool with_metadata) {
	copy(pos, voxels, channel_mask, with_metadata);
}

void VoxelTool::_b_paste(Vector3i pos, Ref<godot::VoxelBuffer> voxels, int channels_mask) {
	paste(pos, voxels, channels_mask);
}

void VoxelTool::_b_paste_masked(
		Vector3i pos,
		Ref<godot::VoxelBuffer> voxels,
		int channels_mask,
		int mask_channel,
		int64_t mask_value
) {
	paste_masked(pos, voxels, channels_mask, mask_channel, mask_value);
}

Variant VoxelTool::_b_get_voxel_metadata(Vector3i pos) const {
	return get_voxel_metadata(pos);
}

void VoxelTool::_b_set_voxel_metadata(Vector3i pos, Variant meta) {
	return set_voxel_metadata(pos, meta);
}

bool VoxelTool::_b_is_area_editable(AABB box) const {
	const Vector3i minp = math::floor_to_int(box.position);
	const Vector3i maxp = math::ceil_to_int(box.position + box.size);
	const Box3i ibox = Box3i::from_min_max(minp, maxp);
#ifdef DEBUG_ENABLED
	if (Vector3iUtil::is_empty_size(ibox.size)) {
		VOXEL_PRINT_WARNING_ONCE(format("Box passed to `is_area_editable` is null-sized: {} => {}", box, ibox));
	}
	VOXEL_ASSERT_RETURN_V(Vector3iUtil::is_valid_size(ibox.size), false);
#endif
	return is_area_editable(ibox);
}

namespace {
int _b_color_to_u16(Color col) {
	return Color8(col).to_u16();
}

uint32_t _b_color_to_u32(Color col) {
	return Color8(col).to_u32();
}

int _b_vec4i_to_u16_indices(Vector4i v) {
	return mixel4::encode_indices_to_packed_u16(v.x, v.y, v.z, v.w);
}

int _b_color_to_u16_weights(Color cf) {
	const Color8 c(cf);
	return mixel4::encode_weights_to_packed_u16_lossy(c.r, c.g, c.b, c.a);
}

Vector4i _b_u16_indices_to_vec4i(int e) {
	FixedArray<uint8_t, 4> indices = mixel4::decode_indices_from_packed_u16(e);
	return Vector4i(indices[0], indices[1], indices[2], indices[3]);
}

Color _b_u16_weights_to_color(int e) {
	FixedArray<uint8_t, 4> indices = mixel4::decode_weights_from_packed_u16(e);
	return Color(indices[0] / 255.f, indices[1] / 255.f, indices[2] / 255.f, indices[3] / 255.f);
}

Color _b_normalize_color(Color c) {
	const float sum = c.r + c.g + c.b + c.a;
	if (sum < 0.00001f) {
		return Color();
	}
	return c / sum;
}
} // namespace

void VoxelTool::_b_set_channel(godot::VoxelBuffer::ChannelId p_channel) {
	set_channel(VoxelBuffer::ChannelId(p_channel));
}

godot::VoxelBuffer::ChannelId VoxelTool::_b_get_channel() const {
	return godot::VoxelBuffer::ChannelId(get_channel());
}

void VoxelTool::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_value", "v"), &VoxelTool::set_value);
	ClassDB::bind_method(D_METHOD("get_value"), &VoxelTool::get_value);

	ClassDB::bind_method(D_METHOD("set_channel", "v"), &VoxelTool::_b_set_channel);
	ClassDB::bind_method(D_METHOD("get_channel"), &VoxelTool::_b_get_channel);

	ClassDB::bind_method(D_METHOD("set_mode", "m"), &VoxelTool::set_mode);
	ClassDB::bind_method(D_METHOD("get_mode"), &VoxelTool::get_mode);

	ClassDB::bind_method(D_METHOD("set_eraser_value", "v"), &VoxelTool::set_eraser_value);
	ClassDB::bind_method(D_METHOD("get_eraser_value"), &VoxelTool::get_eraser_value);

	ClassDB::bind_method(D_METHOD("set_sdf_scale", "scale"), &VoxelTool::set_sdf_scale);
	ClassDB::bind_method(D_METHOD("get_sdf_scale"), &VoxelTool::get_sdf_scale);

	ClassDB::bind_method(D_METHOD("set_sdf_strength", "strength"), &VoxelTool::set_sdf_strength);
	ClassDB::bind_method(D_METHOD("get_sdf_strength"), &VoxelTool::get_sdf_strength);

	ClassDB::bind_method(D_METHOD("set_texture_index", "index"), &VoxelTool::set_texture_index);
	ClassDB::bind_method(D_METHOD("get_texture_index"), &VoxelTool::get_texture_index);

	ClassDB::bind_method(D_METHOD("set_texture_opacity", "opacity"), &VoxelTool::set_texture_opacity);
	ClassDB::bind_method(D_METHOD("get_texture_opacity"), &VoxelTool::get_texture_opacity);

	ClassDB::bind_method(D_METHOD("set_texture_falloff", "falloff"), &VoxelTool::set_texture_falloff);
	ClassDB::bind_method(D_METHOD("get_texture_falloff"), &VoxelTool::get_texture_falloff);

	ClassDB::bind_method(D_METHOD("get_voxel", "pos"), &VoxelTool::_b_get_voxel);
	ClassDB::bind_method(D_METHOD("get_voxel_f", "pos"), &VoxelTool::_b_get_voxel_f);
	ClassDB::bind_method(D_METHOD("set_voxel", "pos", "v"), &VoxelTool::_b_set_voxel);
	ClassDB::bind_method(D_METHOD("set_voxel_f", "pos", "v"), &VoxelTool::_b_set_voxel_f);
	ClassDB::bind_method(D_METHOD("do_point", "pos"), &VoxelTool::_b_do_point);
	ClassDB::bind_method(D_METHOD("do_sphere", "center", "radius"), &VoxelTool::_b_do_sphere);
	ClassDB::bind_method(D_METHOD("do_box", "begin", "end"), &VoxelTool::_b_do_box);
	ClassDB::bind_method(D_METHOD("do_path", "points", "radii"), &VoxelTool::_b_do_path);
#ifdef VOXEL_ENABLE_MESH_SDF
	ClassDB::bind_method(D_METHOD("do_mesh", "mesh_sdf", "transform", "isolevel"), &VoxelTool::_b_do_mesh, DEFVAL(0.0));
#endif

	ClassDB::bind_method(
			D_METHOD("smooth_sphere", "sphere_center", "sphere_radius", "blur_radius"), &VoxelTool::smooth_sphere
	);
	ClassDB::bind_method(
			D_METHOD("grow_sphere", "sphere_center", "sphere_radius", "strength"), &VoxelTool::grow_sphere
	);

	ClassDB::bind_method(D_METHOD("set_voxel_metadata", "pos", "meta"), &VoxelTool::_b_set_voxel_metadata);
	ClassDB::bind_method(D_METHOD("get_voxel_metadata", "pos"), &VoxelTool::_b_get_voxel_metadata);

	ClassDB::bind_method(
			D_METHOD("copy", "src_pos", "dst_buffer", "channels_mask", "with_metadata"),
			&VoxelTool::_b_copy,
			DEFVAL(VoxelBuffer::ALL_CHANNELS_MASK),
			DEFVAL(true)
	);
	ClassDB::bind_method(
			D_METHOD("paste", "dst_pos", "src_buffer", "channels_mask"),
			&VoxelTool::_b_paste,
			DEFVAL(VoxelBuffer::ALL_CHANNELS_MASK)
	);
	ClassDB::bind_method(
			D_METHOD("paste_masked", "dst_pos", "src_buffer", "channels_mask", "mask_channel", "mask_value"),
			&VoxelTool::_b_paste_masked
	);
	ClassDB::bind_method(
			D_METHOD(
					"paste_masked_writable_list",
					"position",
					"voxels",
					"channels_mask",
					"src_mask_channel",
					"src_mask_value",
					"dst_mask_channel",
					"dst_writable_list"
			),
			&VoxelTool::paste_masked_writable_list
	);

	ClassDB::bind_method(
			D_METHOD("raycast", "origin", "direction", "max_distance", "collision_mask"),
			&VoxelTool::_b_raycast,
			DEFVAL(10.0),
			DEFVAL(0xffffffff)
	);

	ClassDB::bind_method(D_METHOD("set_raycast_normal_enabled", "enabled"), &VoxelTool::set_raycast_normal_enabled);

	ClassDB::bind_method(D_METHOD("is_area_editable", "box"), &VoxelTool::_b_is_area_editable);

	// 编码辅助函数
	ClassDB::bind_static_method(VoxelTool::get_class_static(), D_METHOD("color_to_u16", "color"), &_b_color_to_u16);
	ClassDB::bind_static_method(VoxelTool::get_class_static(), D_METHOD("color_to_u32", "color"), &_b_color_to_u32);
	ClassDB::bind_static_method(
			VoxelTool::get_class_static(), D_METHOD("vec4i_to_u16_indices"), &_b_vec4i_to_u16_indices
	);
	ClassDB::bind_static_method(
			VoxelTool::get_class_static(), D_METHOD("color_to_u16_weights"), &_b_color_to_u16_weights
	);
	ClassDB::bind_static_method(
			VoxelTool::get_class_static(), D_METHOD("u16_indices_to_vec4i"), &_b_u16_indices_to_vec4i
	);
	ClassDB::bind_static_method(
			VoxelTool::get_class_static(), D_METHOD("u16_weights_to_color"), &_b_u16_weights_to_color
	);
	ClassDB::bind_static_method(VoxelTool::get_class_static(), D_METHOD("normalize_color"), &_b_normalize_color);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "value"), "set_value", "get_value");
	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "channel", PROPERTY_HINT_ENUM, godot::VoxelBuffer::CHANNEL_ID_HINT_STRING),
			"set_channel",
			"get_channel"
	);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "eraser_value"), "set_eraser_value", "get_eraser_value");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Add,Remove,Set"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sdf_scale"), "set_sdf_scale", "get_sdf_scale");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sdf_strength"), "set_sdf_strength", "get_sdf_strength");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "texture_index"), "set_texture_index", "get_texture_index");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "texture_opacity"), "set_texture_opacity", "get_texture_opacity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "texture_falloff"), "set_texture_falloff", "get_texture_falloff");

	BIND_ENUM_CONSTANT(MODE_ADD);
	BIND_ENUM_CONSTANT(MODE_REMOVE);
	BIND_ENUM_CONSTANT(MODE_SET);
	BIND_ENUM_CONSTANT(MODE_TEXTURE_PAINT);
}

} // namespace voxel
