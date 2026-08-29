#ifndef VOXEL_TOOL_H
#define VOXEL_TOOL_H

#include "../storage/funcs.h"
#include "../storage/voxel_buffer_gd.h"
#include "../storage/voxel_format.h"
#include "../util/math/box3i.h"
#include "../util/math/sdf.h"
#include "funcs.h"
#include "voxel_raycast_result.h"

// TODO 需要审查 VoxelTool 以考虑变换后的体积

namespace voxel {

#ifdef VOXEL_ENABLE_MESH_SDF
class VoxelMeshSDF;
#endif

// 高级体素编辑接口。
// 这不是一个需要单独实例化的类，请从你想要操作的体素对象中获取它。
// 可能会有些开销，因此如果某个特定场景需要优化，可以直接用底层数据结构实现，
// 或最终添加到 VoxelTool 的对应实现中。如果大多数实现都提供相同的功能，
// 则可以添加到基类中。
class VoxelTool : public RefCounted {
	GDCLASS(VoxelTool, RefCounted)
public:
	enum Mode {
		MODE_ADD = ops::MODE_ADD,
		MODE_REMOVE = ops::MODE_REMOVE,
		MODE_SET = ops::MODE_SET,
		MODE_TEXTURE_PAINT = ops::MODE_TEXTURE_PAINT
	};

	VoxelTool();

	void set_value(uint64_t val);
	uint64_t get_value() const;

	void set_channel(VoxelBuffer::ChannelId p_channel);
	VoxelBuffer::ChannelId get_channel() const;

	void set_mode(Mode mode);
	Mode get_mode() const;

	void set_eraser_value(uint64_t value);
	uint64_t get_eraser_value() const;

	uint64_t get_voxel(Vector3i pos) const;
	float get_voxel_f(Vector3i pos) const;

	virtual float get_voxel_f_interpolated(const Vector3 pos) const;

	float get_sdf_scale() const;
	void set_sdf_scale(float s);

	void set_texture_index(int ti);
	int get_texture_index() const;

	void set_texture_opacity(float opacity);
	float get_texture_opacity() const;

	void set_texture_falloff(float falloff);
	float get_texture_falloff() const;

	void set_sdf_strength(float strength);
	float get_sdf_strength() const;

	// TODO 对整个区域操作的方法必须使用尽量减少锁的实现！

	// 以下每个方法代表一次编辑。请为工作选择合适的那个。
	// 例如，使用 `do_box` 比多次调用 `do_point` 更高效。
	virtual void set_voxel(Vector3i pos, uint64_t v);
	virtual void set_voxel_f(Vector3i pos, float v);
	virtual void do_point(Vector3i pos);
	virtual void do_sphere(Vector3 p_center, float radius);
	virtual void do_box(Vector3i begin, Vector3i end);
	virtual void do_path(Span<const Vector3> positions, Span<const float> radii);
#ifdef VOXEL_ENABLE_MESH_SDF
	virtual void do_mesh(const VoxelMeshSDF &mesh_sdf, const Transform3D &transform, const float isolevel);
#endif

	void sdf_stamp_erase(Ref<godot::VoxelBuffer> stamp, Vector3i pos);
	void sdf_stamp_erase(const VoxelBuffer &stamp, Vector3i pos);

	virtual void copy(
			const Vector3i pos,
			VoxelBuffer &dst,
			const uint8_t channels_mask,
			const bool with_metadata
	) const;
	void copy(
			const Vector3i pos,
			Ref<godot::VoxelBuffer> dst,
			const uint8_t channels_mask,
			const bool with_metadata
	) const;

	virtual void paste(Vector3i pos, const VoxelBuffer &src, uint8_t channels_mask);
	void paste(Vector3i pos, Ref<godot::VoxelBuffer> p_voxels, uint8_t channels_mask);

	virtual void paste_masked(
			Vector3i pos,
			Ref<godot::VoxelBuffer> p_voxels,
			uint8_t channels_mask,
			uint8_t mask_channel,
			uint64_t mask_value
	);

	virtual void paste_masked_writable_list(
			Vector3i pos,
			Ref<godot::VoxelBuffer> p_voxels,
			uint8_t channels_mask,
			uint8_t src_mask_channel,
			uint64_t src_mask_value,
			uint8_t dst_mask_channel,
			PackedInt32Array dst_writable_list
	);

	void smooth_sphere(Vector3 sphere_center, float sphere_radius, int blur_radius);
	void grow_sphere(Vector3 sphere_center, float sphere_radius, float strength);

	virtual Ref<VoxelRaycastResult> raycast(Vector3 pos, Vector3 dir, float max_distance, uint32_t collision_mask);

	void set_raycast_normal_enabled(bool enabled);

	// 检查影响给定盒的编辑是否可以应用，无论是完全还是部分
	virtual bool is_area_editable(const Box3i &box) const;

	virtual void set_voxel_metadata(const Vector3i pos, const Variant &meta);
	virtual Variant get_voxel_metadata(const Vector3i pos) const;

	virtual VoxelFormat get_format() const;

protected:
	static void _bind_methods();

	// 这些方法从不单独使用，但可以在其它方法中使用。
	// 它们不代表一次编辑，只是对底层 API 的抽象
	virtual uint64_t _get_voxel(Vector3i pos) const;
	virtual float _get_voxel_f(Vector3i pos) const;
	virtual void _set_voxel(Vector3i pos, uint64_t v);
	virtual void _set_voxel_f(Vector3i pos, float v);
	virtual void _post_edit(const Box3i &box);

	void do_path_chunked(
			VoxelData &vdata,
			Span<const Vector3> positions,
			Span<const float> radii,
			const bool with_pre_generate
	);

#ifdef VOXEL_ENABLE_MESH_SDF
	void do_mesh_chunked(
			const VoxelMeshSDF &mesh_sdf,
			VoxelData &vdata,
			const Transform3D &transform,
			const float isolevel,
			const bool with_pre_generate
	);
#endif

private:
	// 用于转换为更具体的 C++ 类型并处理虚特性的绑定，
	// 因为我不确定直接绑定是否可行

	uint64_t _b_get_voxel(Vector3i pos);
	float _b_get_voxel_f(Vector3i pos);
	void _b_set_voxel(Vector3i pos, uint64_t v);
	void _b_set_voxel_f(Vector3i pos, float v);
	Ref<VoxelRaycastResult> _b_raycast(Vector3 pos, Vector3 dir, float max_distance, uint32_t collision_mask);
	void _b_do_point(Vector3i pos);
	void _b_do_sphere(Vector3 pos, float radius);
	void _b_do_box(Vector3i begin, Vector3i end);
	void _b_do_path(PackedVector3Array positions, PackedFloat32Array radii);
#ifdef VOXEL_ENABLE_MESH_SDF
	void _b_do_mesh(Ref<VoxelMeshSDF> mesh_sdf, Transform3D transform, float isolevel);
#endif
	void _b_copy(Vector3i pos, Ref<godot::VoxelBuffer> voxels, int channel_mask, bool with_metadata);
	void _b_paste(Vector3i pos, Ref<godot::VoxelBuffer> voxels, int channels_mask);
	void _b_paste_masked(
			Vector3i pos,
			Ref<godot::VoxelBuffer> voxels,
			int channels_mask,
			int mask_channel,
			int64_t mask_value
	);
	Variant _b_get_voxel_metadata(Vector3i pos) const;
	void _b_set_voxel_metadata(Vector3i pos, Variant meta);
	bool _b_is_area_editable(AABB box) const;
	void _b_set_channel(godot::VoxelBuffer::ChannelId p_channel);

	godot::VoxelBuffer::ChannelId _b_get_channel() const;

protected:
	uint64_t _value = 0;
	uint64_t _eraser_value = 0; // 空气
	VoxelBuffer::ChannelId _channel = VoxelBuffer::CHANNEL_TYPE;
	float _sdf_scale = 1.f;
	float _sdf_strength = 1.f;
	Mode _mode = MODE_ADD;
	// 如果为 true，即使受影响区域部分超出可编辑体素的边界，也允许进行操作。
	// 视上下文而定，这可能有用，也可能导致不一致的结果。
	bool _allow_out_of_bounds = false;
	bool _raycast_normal_enabled = true;

	// 用于平滑地形
	ops::TextureParams _texture_params;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::VoxelTool::Mode)

#endif // VOXEL_TOOL_H
