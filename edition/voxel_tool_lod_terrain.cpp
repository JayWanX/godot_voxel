#include "voxel_tool_lod_terrain.h"
#include "../constants/voxel_string_names.h"
#include "../generators/graph/voxel_generator_graph.h"
#include "../meshers/blocky/voxel_mesher_blocky.h"
#include "../storage/voxel_buffer_gd.h"
#include "../storage/voxel_data_grid.h"
#include "../terrain/variable_lod/voxel_lod_terrain.h"
#include "../util/containers/std_vector.h"
#include "../util/dstack.h"
#include "../util/island_finder.h"
#include "../util/math/conv.h"
#include "../util/string/format.h"
#include "../util/tasks/async_dependency_tracker.h"
#include "../util/voxel_raycast.h"
#include "floating_chunks.h"
#include "funcs.h"
#include "raycast.h"

#ifdef VOXEL_ENABLE_MESH_SDF
#include "voxel_mesh_sdf_gd.h"
#endif

namespace voxel {

VoxelToolLodTerrain::VoxelToolLodTerrain(VoxelLodTerrain *terrain) : _terrain(terrain) {
	ERR_FAIL_COND(terrain == nullptr);
	// 目前仅支持 LOD0。
	// 当体素工具仍引用地形时，不要销毁地形
}

bool VoxelToolLodTerrain::is_area_editable(const Box3i &box) const {
	ERR_FAIL_COND_V(_terrain == nullptr, false);
	return _terrain->get_storage().is_area_loaded(box);
}

Ref<VoxelRaycastResult> VoxelToolLodTerrain::raycast(
		Vector3 pos,
		Vector3 dir,
		float max_distance,
		uint32_t collision_mask
) {
	return raycast_generic_world(
			_terrain->get_storage(),
			_terrain->get_mesher(),
			_terrain->get_global_transform(),
			pos,
			dir,
			max_distance,
			collision_mask,
			_raycast_binary_search_iterations,
			_raycast_normal_enabled
	);
}

void VoxelToolLodTerrain::do_box(Vector3i begin, Vector3i end) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_terrain == nullptr);

	ops::DoShapeChunked<ops::SdfAxisAlignedBox, ops::VoxelDataGridAccess> op;
	op.shape.center = to_vec3f(begin + end) * 0.5f;
	op.shape.half_size = to_vec3f(end - begin) * 0.5f;
	op.shape.sdf_scale = get_sdf_scale();
	op.box = op.shape.get_box().clipped(_terrain->get_voxel_bounds());
	op.mode = static_cast<ops::Mode>(get_mode());
	op.texture_params = _texture_params;
	op.blocky_value = _value;
	op.channel = get_channel();
	op.strength = get_sdf_strength();

	if (!is_area_editable(op.box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelData &data = _terrain->get_storage();

	data.pre_generate_box(op.box);

	VoxelDataGrid grid;
	data.get_blocks_grid(grid, op.box, 0);
	op.block_access.grid = &grid;

	{
		VoxelDataGrid::LockWrite wlock(grid);
		op();
	}

	_post_edit(op.box);
}

void VoxelToolLodTerrain::do_sphere(Vector3 center, float radius) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_terrain == nullptr);

	ops::DoSphere op;
	op.shape.center = to_vec3f(center);
	op.shape.radius = radius;
	op.shape.sdf_scale = get_sdf_scale();
	op.box = op.shape.get_box().clipped(_terrain->get_voxel_bounds());
	op.mode = static_cast<ops::Mode>(get_mode());
	op.texture_params = _texture_params;
	op.blocky_value = _value;
	op.channel = get_channel();
	op.strength = get_sdf_strength();

	const Box3i world_box = op.box;

	if (!is_area_editable(world_box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelData &data = _terrain->get_storage();

	data.pre_generate_box(world_box);
	data.get_blocks_grid(op.blocks, world_box, 0);

	// 通过在局部空间中执行操作，我们可以使用浮点数
	const Vector3i origin_in_voxels = op.blocks.get_origin_block_position_in_voxels();
	op.shape.center = to_vec3f(center - to_vec3(origin_in_voxels));
	op.box.position -= origin_in_voxels;
	op.blocks.use_relative_coordinates();

	op();

	_post_edit(world_box);
}

void VoxelToolLodTerrain::do_hemisphere(Vector3 center, float radius, Vector3 flat_direction, float smoothness) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_terrain == nullptr);

	ops::DoShapeChunked<ops::SdfHemisphere, ops::VoxelDataGridAccess> op;
	op.shape.center = to_vec3f(center);
	op.shape.radius = radius;
	op.shape.flat_direction = to_vec3f(flat_direction);
	op.shape.plane_d = flat_direction.dot(center);
	op.shape.smoothness = smoothness;
	op.shape.sdf_scale = get_sdf_scale();
	op.box = op.shape.get_box().clipped(_terrain->get_voxel_bounds());
	op.mode = static_cast<ops::Mode>(get_mode());
	op.texture_params = _texture_params;
	op.blocky_value = _value;
	op.channel = get_channel();
	op.strength = get_sdf_strength();

	if (!is_area_editable(op.box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelData &data = _terrain->get_storage();

	data.pre_generate_box(op.box);

	VoxelDataGrid grid;
	data.get_blocks_grid(grid, op.box, 0);
	op.block_access.grid = &grid;

	{
		VoxelDataGrid::LockWrite wlock(grid);
		op();
	}

	_post_edit(op.box);
}

void VoxelToolLodTerrain::do_path(Span<const Vector3> positions, Span<const float> radii) {
	VOXEL_ASSERT_RETURN(_terrain != nullptr);
	do_path_chunked(_terrain->get_storage(), positions, radii, true);
}

template <typename Op_T>
class VoxelToolAsyncEdit : public IThreadedTask {
public:
	VoxelToolAsyncEdit(Op_T op, std::shared_ptr<VoxelData> data) : _op(op), _data(data) {
		_tracker = make_shared_instance<AsyncDependencyTracker>(1);
	}

	const char *get_debug_name() const override {
		return "VoxelToolAsyncEdit";
	}

	void run(ThreadedTaskContext &ctx) override {
		VOXEL_PROFILE_SCOPE();
		VOXEL_ASSERT(_data != nullptr);
		// TODO 如果并非所有区块都找到，或许应当失败
		// TODO 需要应用修改器
		_data->get_blocks_grid(_op.blocks, _op.box, 0);
		_op();
		_tracker->post_complete();
	}

	std::shared_ptr<AsyncDependencyTracker> get_tracker() {
		return _tracker;
	}

private:
	Op_T _op;
	// 我们引用它只是为了保活 map 的指针
	std::shared_ptr<VoxelData> _data;
	std::shared_ptr<AsyncDependencyTracker> _tracker;
};

void VoxelToolLodTerrain::do_sphere_async(Vector3 center, float radius) {
	ERR_FAIL_COND(_terrain == nullptr);

	ops::DoSphere op;
	op.shape.center = to_vec3f(center);
	op.shape.radius = radius;
	op.shape.sdf_scale = get_sdf_scale();
	op.box = op.shape.get_box().clipped(_terrain->get_voxel_bounds());
	op.mode = ops::Mode(get_mode());
	op.texture_params = _texture_params;
	op.blocky_value = _value;
	op.channel = get_channel();
	op.strength = get_sdf_strength();

	if (!is_area_editable(op.box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	std::shared_ptr<VoxelData> data = _terrain->get_storage_shared();

	VoxelToolAsyncEdit<ops::DoSphere> *task = VOXEL_NEW(VoxelToolAsyncEdit<ops::DoSphere>(op, data));
	_terrain->push_async_edit(task, op.box, task->get_tracker());
}

void VoxelToolLodTerrain::copy(
		const Vector3i pos,
		VoxelBuffer &dst,
		const uint8_t p_channels_mask,
		const bool with_metadata
) const {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_terrain == nullptr);
	const unsigned int channels_mask = (p_channels_mask == 0 ? (1 << _channel) : p_channels_mask);
	_terrain->get_storage().copy(pos, dst, channels_mask, with_metadata);
}

void VoxelToolLodTerrain::paste(Vector3i pos, const VoxelBuffer &src, uint8_t channels_mask) {
	ERR_FAIL_COND(_terrain == nullptr);
	if (channels_mask == 0) {
		channels_mask = (1 << _channel);
	}
	const Box3i box(pos, src.get_size());
	if (!is_area_editable(box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelData &data = _terrain->get_storage();

	data.pre_generate_box(box);
	data.paste(pos, src, channels_mask, false, true);

	_post_edit(box);
}

void VoxelToolLodTerrain::set_voxel_metadata(const Vector3i pos, const Variant &meta) {
	VOXEL_ASSERT_RETURN(_terrain != nullptr);
	VoxelData &data = _terrain->get_storage();
	data.set_voxel_metadata(pos, meta);
	_terrain->post_edit_area(Box3i(pos, Vector3i(1, 1, 1)), false);
}

Variant VoxelToolLodTerrain::get_voxel_metadata(const Vector3i pos) const {
	VOXEL_ASSERT_RETURN_V(_terrain != nullptr, Variant());
	VoxelData &data = _terrain->get_storage();
	return data.get_voxel_metadata(pos);
}

float VoxelToolLodTerrain::get_voxel_f_interpolated(Vector3 position) const {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND_V(_terrain == nullptr, 0);
	const int channel = get_channel();
	VoxelData &data = _terrain->get_storage();
	// TODO 优化：是否有必要为此实现一条快速路径？
	return get_sdf_interpolated(
			[&data, channel](Vector3i ipos) {
				VoxelSingleValue defval;
				defval.f = constants::SDF_FAR_OUTSIDE;
				VoxelSingleValue value = data.get_voxel(ipos, channel, defval);
				return value.f;
			},
			position
	);
}

uint64_t VoxelToolLodTerrain::_get_voxel(Vector3i pos) const {
	ERR_FAIL_COND_V(_terrain == nullptr, 0);
	VoxelSingleValue defval;
	defval.i = 0;
	return _terrain->get_storage().get_voxel(pos, _channel, defval).i;
}

float VoxelToolLodTerrain::_get_voxel_f(Vector3i pos) const {
	ERR_FAIL_COND_V(_terrain == nullptr, 0);
	VoxelSingleValue defval;
	defval.f = constants::SDF_FAR_OUTSIDE;
	return _terrain->get_storage().get_voxel(pos, _channel, defval).f;
}

void VoxelToolLodTerrain::_set_voxel(Vector3i pos, uint64_t v) {
	ERR_FAIL_COND(_terrain == nullptr);
	_terrain->get_storage().try_set_voxel(v, pos, _channel);
	// 没有 post_update，由父类完成，这是一种通用的慢速实现。
}

void VoxelToolLodTerrain::_set_voxel_f(Vector3i pos, float v) {
	ERR_FAIL_COND(_terrain == nullptr);
	// TODO 格式应可从地形访问
	_terrain->get_storage().try_set_voxel_f(v, pos, _channel);
	// 没有 post_update，由父类完成，这是一种通用的慢速实现。
}

void VoxelToolLodTerrain::_post_edit(const Box3i &box) {
	ERR_FAIL_COND(_terrain == nullptr);
	_terrain->post_edit_area(box, true);
}

int VoxelToolLodTerrain::get_raycast_binary_search_iterations() const {
	return _raycast_binary_search_iterations;
}

void VoxelToolLodTerrain::set_raycast_binary_search_iterations(int iterations) {
	_raycast_binary_search_iterations = math::clamp(iterations, 0, 16);
}

#if defined(VOXEL_GODOT)
Array VoxelToolLodTerrain::separate_floating_chunks(AABB world_box, Node *parent_node) {
#endif
	ERR_FAIL_COND_V(_terrain == nullptr, Array());
	ERR_FAIL_COND_V(!math::is_valid_size(world_box.size), Array());
	Ref<VoxelMesher> mesher = _terrain->get_mesher();
	Array materials;
	materials.append(_terrain->get_material());
	const Box3i int_world_box(math::floor_to_int(world_box.position), math::ceil_to_int(world_box.size));
	return voxel::separate_floating_chunks(
			*this, int_world_box, parent_node, _terrain->get_global_transform(), mesher, materials
	);
}

#ifdef VOXEL_ENABLE_MESH_SDF

// 将预计算的 SDF 与地形在指定位置、旋转和缩放下合并。
//
// `transform` 表示缓冲区应应用到地形上的位置。
//
// `isolevel` 会改变 SDF 的形状：正值使其“膨胀”，负值使其“侵蚀”。它在 `sdf_scale` 之后应用。
//
// `sdf_scale` 会缩放 SDF 值（它不会改变形状的大小）。通常默认为 1，但在
// 地形 SDF 中使用的缩放导致出现伪影时，可能会取更小的值。
//
void VoxelToolLodTerrain::stamp_sdf(
		Ref<VoxelMeshSDF> mesh_sdf,
		Transform3D transform,
		float isolevel,
		float sdf_scale
) {
	VOXEL_PRINT_WARNING_ONCE("This method is deprecated. Use `do_mesh` instead.");
	VOXEL_PROFILE_SCOPE();

	ERR_FAIL_COND(_terrain == nullptr);
	ERR_FAIL_COND(mesh_sdf.is_null());
	ERR_FAIL_COND(!mesh_sdf->is_baked());
	Ref<godot::VoxelBuffer> buffer_ref = mesh_sdf->get_voxel_buffer();
	ERR_FAIL_COND(buffer_ref.is_null());
	const VoxelBuffer &buffer = buffer_ref->get_buffer();
	const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_SDF;
	ERR_FAIL_COND(buffer.get_channel_compression(channel) == VoxelBuffer::COMPRESSION_UNIFORM);
	ERR_FAIL_COND(buffer.get_channel_depth(channel) != VoxelBuffer::DEPTH_32_BIT);

	const Transform3D &box_to_world = transform;
	const AABB local_aabb = mesh_sdf->get_aabb();

	// 注意，变换是相对于地形局部的
	const AABB aabb = box_to_world.xform(local_aabb);
	const Box3i voxel_box = Box3i::from_min_max(aabb.position.floor(), (aabb.position + aabb.size).ceil());

	// TODO 有时在尚未加载的区块附近会失败，即使变换后的包围盒并未与它们相交。
	// 这可以通过盒体/变换后盒体的相交判定算法来避免。如果确实出现这种使用场景，可以考虑研究一下
	// 的情况。在完全加载模式下不会发生，其他形状也会受此影响。
	if (!is_area_editable(voxel_box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelData &data = _terrain->get_storage();

	data.pre_generate_box(voxel_box);

	// TODO 也许将盒体“光栅化”会更高效？我们目前会遍历盒体并未相交的体素。
	// TODO 也许我们也应该根据变换的缩放来缩放 SDF 值

	const Transform3D buffer_to_box =
			Transform3D(Basis().scaled(Vector3(local_aabb.size / buffer.get_size())), local_aabb.position);
	const Transform3D buffer_to_world = box_to_world * buffer_to_box;

	// TODO 支持其他位深，格式应可从体积中访问
	ops::SdfOperation16bit<ops::SdfUnion, ops::SdfBufferShape> op;
	op.op.strength = get_sdf_strength();
	op.shape.world_to_buffer = buffer_to_world.affine_inverse();
	op.shape.buffer_size = buffer.get_size();
	op.shape.isolevel = isolevel;
	op.shape.sdf_scale = sdf_scale;
	// 注意，传入的缓冲区不能与另一个线程共享。
	// buffer.decompress_channel(channel);
	VOXEL_ASSERT_RETURN(buffer.get_channel_data_read_only(channel, op.shape.buffer));

	VoxelDataGrid grid;
	data.get_blocks_grid(grid, voxel_box, 0);
	grid.write_box(voxel_box, VoxelBuffer::CHANNEL_SDF, op);

	_post_edit(voxel_box);
}

void VoxelToolLodTerrain::do_mesh(const VoxelMeshSDF &mesh_sdf, const Transform3D &transform, const float isolevel) {
	VOXEL_ASSERT_RETURN(_terrain != nullptr);
	do_mesh_chunked(mesh_sdf, _terrain->get_storage(), transform, isolevel, true);
}

#endif

// 在给定的地形包围盒中运行该图。
// 该图必须具有 SDF 输出，也可以有 SDF 输入以读取源体素。
// transform 包含编辑的位置、朝向和缩放。
// 图的基准大小是笔刷的原始大小（在图中设计），它将通过 transform 进行缩放。
void VoxelToolLodTerrain::do_graph(Ref<VoxelGeneratorGraph> graph, Transform3D transform, Vector3 graph_base_size) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_DSTACK();
	ERR_FAIL_COND(_terrain == nullptr);

	const Vector3 area_size = math::abs(transform.basis.xform(graph_base_size));

	const Box3i box = Box3i::from_min_max(
							  math::floor_to_int(transform.origin - 0.5 * area_size),
							  math::ceil_to_int(transform.origin + 0.5 * area_size)
	)
							  .padded(2)
							  .clipped(_terrain->get_voxel_bounds());

	if (!is_area_editable(box)) {
		VOXEL_PRINT_WARNING("Area not editable");
		return;
	}

	VoxelData &data = _terrain->get_storage();

	data.pre_generate_box(box);

	const unsigned int channel_index = VoxelBuffer::CHANNEL_SDF;

	VoxelBuffer buffer(VoxelBuffer::ALLOCATOR_POOL);
	buffer.create(box.size);
	data.copy(box.position, buffer, 1 << channel_index, false);

	buffer.decompress_channel(channel_index);

	// 转换输入 SDF
	static thread_local StdVector<float> tls_in_sdf_full;
	tls_in_sdf_full.resize(Vector3iUtil::get_volume_u64(buffer.get_size()));
	Span<float> in_sdf_full = to_span(tls_in_sdf_full);
	get_unscaled_sdf(buffer, in_sdf_full);

	static thread_local StdVector<float> tls_in_x;
	static thread_local StdVector<float> tls_in_y;
	static thread_local StdVector<float> tls_in_z;
	const unsigned int deck_area = box.size.x * box.size.y;
	tls_in_x.resize(deck_area);
	tls_in_y.resize(deck_area);
	tls_in_z.resize(deck_area);
	Span<float> in_x = to_span(tls_in_x);
	Span<float> in_y = to_span(tls_in_y);
	Span<float> in_z = to_span(tls_in_z);

	const Transform3D inv_transform = transform.affine_inverse();

	const int output_sdf_buffer_index = graph->get_sdf_output_port_address();
	VOXEL_ASSERT_RETURN_MSG(output_sdf_buffer_index != -1, "The graph has no SDF output, cannot use it as a brush");

	// 该图在固定维度上工作，因此如果用 Transform3D 缩放操作，我们也必须
	// 缩放该图所处理的 distance field（距离场）
	const float graph_scale = transform.basis.get_scale().length();
	const float inv_graph_scale = 1.f / graph_scale;

	for (float &sd : in_sdf_full) {
		sd *= inv_graph_scale;
	}

	const float op_strength = get_sdf_strength();

	{
		VOXEL_PROFILE_SCOPE_NAMED("Slices");
		// 对于盒体的每一层（这样做是为了减少内存占用，因为该图会为每次操作分配临时缓冲区，
		// 其数量可能很大，取决于图的复杂度）
		Vector3i pos;
		const Vector3i endpos = box.position + box.size;
		for (pos.z = box.position.z; pos.z < endpos.z; ++pos.z) {
			// 设置位置
			for (unsigned int i = 0; i < deck_area; ++i) {
				in_z[i] = pos.z;
			}
			{
				unsigned int i = 0;
				for (pos.x = box.position.x; pos.x < endpos.x; ++pos.x) {
					for (pos.y = box.position.y; pos.y < endpos.y; ++pos.y) {
						in_x[i] = pos.x;
						in_y[i] = pos.y;
						++i;
					}
				}
			}

			// 将位置转换为相对于图的局部坐标
			for (unsigned int i = 0; i < deck_area; ++i) {
				Vector3 graph_local_pos(in_x[i], in_y[i], in_z[i]);
				graph_local_pos = inv_transform.xform(graph_local_pos);
				in_x[i] = graph_local_pos.x;
				in_y[i] = graph_local_pos.y;
				in_z[i] = graph_local_pos.z;
			}

			// 获取 SDF 输入
			Span<float> in_sdf = in_sdf_full.sub(deck_area * (pos.z - box.position.z), deck_area);

			// 运行图
			graph->generate_series(in_x, in_y, in_z, in_sdf);

			// 读取结果
			const pg::Runtime::State &state = VoxelGeneratorGraph::get_last_state_from_current_thread();
			const pg::Runtime::Buffer &graph_buffer = state.get_buffer(output_sdf_buffer_index);

			// 应用 strength 和图缩放。输入同时也作为输出，两者不应重叠
			for (unsigned int i = 0; i < in_sdf.size(); ++i) {
				in_sdf[i] = Math::lerp(in_sdf[i], graph_buffer.data[i], op_strength) * graph_scale;
			}
		}
	}

	scale_and_store_sdf(buffer, in_sdf_full);

	data.paste(box.position, buffer, 1 << channel_index, false, false);

	_post_edit(box);
}

void VoxelToolLodTerrain::run_blocky_random_tick(
		const AABB voxel_area,
		const int voxel_count,
		const Callable &callback,
		const int block_batch_count,
		const uint32_t tags_mask
) {
	VOXEL_PROFILE_SCOPE();

	VOXEL_ASSERT_RETURN(_terrain != nullptr);

	Ref<VoxelMesherBlocky> mesher = _terrain->get_mesher();
	VOXEL_ASSERT_RETURN_MSG(
			mesher.is_valid(),
			format("This function requires a volume using {} with a valid library", VOXEL_CLASS_NAME_C(VoxelMesherBlocky))
	);
	Ref<VoxelBlockyLibraryBase> library = mesher->get_library();
	VOXEL_ASSERT_RETURN_MSG(library.is_valid(), format("{} has no library assigned", VOXEL_CLASS_NAME_C(VoxelMesherBlocky)));

	VOXEL_ASSERT_RETURN(callback.is_valid());
	VOXEL_ASSERT_RETURN(block_batch_count > 0);
	VOXEL_ASSERT_RETURN(voxel_count >= 0);
	VOXEL_ASSERT_RETURN(math::is_valid_size(voxel_area.size));

	if (voxel_count == 0) {
		return;
	}

	VoxelData &data = _terrain->get_storage();

	voxel::run_blocky_random_tick(
			data, voxel_area, **library, _random, voxel_count, block_batch_count, tags_mask, callback
	);
}

VoxelFormat VoxelToolLodTerrain::get_format() const {
	VOXEL_ASSERT(_terrain != nullptr);
	return _terrain->get_storage().get_format();
}

void VoxelToolLodTerrain::_bind_methods() {
	using Self = VoxelToolLodTerrain;

	ClassDB::bind_method(
			D_METHOD("set_raycast_binary_search_iterations", "iterations"), &Self::set_raycast_binary_search_iterations
	);
	ClassDB::bind_method(D_METHOD("get_raycast_binary_search_iterations"), &Self::get_raycast_binary_search_iterations);
	ClassDB::bind_method(D_METHOD("get_voxel_f_interpolated", "position"), &Self::get_voxel_f_interpolated);
	ClassDB::bind_method(D_METHOD("separate_floating_chunks", "box", "parent_node"), &Self::separate_floating_chunks);
	ClassDB::bind_method(D_METHOD("do_sphere_async", "center", "radius"), &Self::do_sphere_async);
#ifdef VOXEL_ENABLE_MESH_SDF
	ClassDB::bind_method(D_METHOD("stamp_sdf", "mesh_sdf", "transform", "isolevel", "sdf_scale"), &Self::stamp_sdf);
#endif
	ClassDB::bind_method(D_METHOD("do_graph", "graph", "transform", "area_size"), &Self::do_graph);
	ClassDB::bind_method(
			D_METHOD("do_hemisphere", "center", "radius", "flat_direction", "smoothness"),
			&Self::do_hemisphere,
			DEFVAL(0.0)
	);
	ClassDB::bind_method(
			D_METHOD("run_blocky_random_tick", "area", "voxel_count", "callback", "batch_count", "tags_mask"),
			&Self::run_blocky_random_tick,
			DEFVAL(16),
			DEFVAL(0xffffffff)
	);
}

} // namespace voxel
