#include "voxel_terrain.h"
#include "../../constants/voxel_constants.h"
#include "../../constants/voxel_string_names.h"
#include "../../edition/voxel_tool_terrain.h"
#include "../../engine/buffered_task_scheduler.h"
#include "../../engine/voxel_engine.h"
#include "../../engine/voxel_engine_updater.h"
#include "../../generators/generate_block_task.h"
#include "../../meshers/blocky/voxel_mesher_blocky.h"
#include "../../meshers/mesh_block_task.h"
#include "../../storage/voxel_buffer_gd.h"
#include "../../storage/voxel_data.h"
#include "../../streams/load_block_data_task.h"
#include "../../streams/save_block_data_task.h"
#include "../../util/containers/container_funcs.h"
#include <scene/resources/material.h>
#include "../../util/godot/classes/concave_polygon_shape_3d.h"
#include "../../util/godot/classes/engine.h"
#include <scene/3d/mesh_instance_3d.h>
#include <scene/main/multiplayer_api.h>
#include <scene/main/multiplayer_peer.h>
#include <scene/main/scene_tree.h>
#include <core/object/script_language.h>
#include <scene/resources/material.h>
#include <core/variant/array.h>
#include "../../util/godot/core/string.h"
#include "../../util/macros.h"
#include "../../util/math/conv.h"
#include "../../util/profiling.h"
#include "../../util/profiling_clock.h"
#include "../../util/string/format.h"
#include "../../util/tasks/async_dependency_tracker.h"
#include "../voxel_data_block_enter_info.h"
#include "../voxel_save_completion_tracker.h"
#include "voxel_terrain_multiplayer_synchronizer.h"
#include <scene/resources/material.h> // 用于发布模式下的属性提示
#include <scene/3d/mesh_instance_3d.h>
#include <scene/main/multiplayer_api.h>
#include <scene/main/multiplayer_peer.h>
#include <scene/main/scene_tree.h>
#include <core/object/script_language.h>
#include <scene/resources/material.h>
#include <core/variant/array.h>
#define ADD_DEBUG_DRAW_FLAG(m_name, m_flag)                                                                            \
	ADD_PROPERTYI(                                                                                                     \
			PropertyInfo(Variant::BOOL, m_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR),                        \
			"debug_set_draw_flag",                                                                                     \
			"debug_get_draw_flag",                                                                                     \
			m_flag                                                                                                     \
	);

#ifdef TOOLS_ENABLED
#include "../../meshers/transvoxel/voxel_mesher_transvoxel.h"
#endif

#ifdef VOXEL_ENABLE_INSTANCER
#include "../instancing/voxel_instancer.h"
#endif

namespace voxel {

VoxelTerrain::VoxelTerrain() {
	// 注意：不要在构造函数中做任何繁重的事情。
	// 由于 ClassDB 获取默认值的方式，Godot 可能在启动时创建和销毁所有节点类型的几十个实例。

	set_notify_transform(true);

	_data = make_shared_instance<VoxelData>();

	// TODO 是否应该为了更好的发现性而设为有限？
	// 默认无限
	_data->set_bounds(Box3i::from_center_extents(Vector3i(), Vector3iUtil::create(constants::MAX_VOLUME_EXTENT)));

	_streaming_dependency = make_shared_instance<StreamingDependency>();
	_meshing_dependency = make_shared_instance<MeshingDependency>();

	struct ApplyMeshUpdateTask : public ITimeSpreadTask {
		void run(TimeSpreadTaskContext &ctx) override {
			if (!VoxelEngine::get_singleton().is_volume_valid(volume_id)) {
				// 该任务仍在等待时，节点可能已被销毁
				VOXEL_PRINT_VERBOSE("Cancelling ApplyMeshUpdateTask, volume_id is invalid");
				return;
			}
			self->apply_mesh_update(data);
		}
		VolumeID volume_id;
		VoxelTerrain *self = nullptr;
		VoxelEngine::BlockMeshOutput data;
	};

	// 网格更新通过调度到 VoxelEngine 的任务运行器来分散到多帧执行，
	// 但我们不用接收缓冲区，而是使用回调，
	// 因为这种任务调度方式否则会使更新延迟 1 帧
	VoxelEngine::VolumeCallbacks callbacks;
	callbacks.data = this;
	callbacks.mesh_output_callback = [](void *cb_data, VoxelEngine::BlockMeshOutput &ob) {
		VoxelTerrain *self = reinterpret_cast<VoxelTerrain *>(cb_data);
		ApplyMeshUpdateTask *task = VOXEL_NEW(ApplyMeshUpdateTask);
		task->volume_id = self->_volume_id;
		task->self = self;
		task->data = std::move(ob);
		VoxelEngine::get_singleton().push_main_thread_time_spread_task(task);
	};
	callbacks.data_output_callback = [](void *cb_data, VoxelEngine::BlockDataOutput &ob) {
		VoxelTerrain *self = reinterpret_cast<VoxelTerrain *>(cb_data);
		self->apply_data_block_response(ob);
	};

	_volume_id = VoxelEngine::get_singleton().add_volume(callbacks);

	// TODO 由于 Godot 4 的警告，无法再设置默认网格器……
	// 为了编辑器中的易用性
	// Ref<VoxelMesherBlocky> default_mesher;
	// default_mesher.instantiate();
	// _mesher = default_mesher;
}

VoxelTerrain::~VoxelTerrain() {
	VOXEL_PRINT_VERBOSE("Destroying VoxelTerrain");
	_streaming_dependency->valid = false;
	_meshing_dependency->valid = false;
	VoxelEngine::get_singleton().remove_volume(_volume_id);
}

void VoxelTerrain::set_material_override(Ref<Material> material) {
	if (_material_override == material) {
		return;
	}
	_material_override = material;
	_mesh_map.for_each_block([material](VoxelMeshBlockVT &block) { //
		block.set_material_override(material);
	});
}

Ref<Material> VoxelTerrain::get_material_override() const {
	return _material_override;
}

#ifdef VOXEL_ENABLE_GPU
void VoxelTerrain::set_generator_use_gpu(bool enabled) {
	_generator_use_gpu = enabled;
}

bool VoxelTerrain::get_generator_use_gpu() const {
	return _generator_use_gpu;
}
#endif

VoxelData &VoxelTerrain::get_storage() const {
	VOXEL_ASSERT(_data != nullptr);
	return *_data;
}

void VoxelTerrain::set_stream(Ref<VoxelStream> p_stream) {
	if (p_stream == get_stream()) {
		return;
	}

	_data->set_stream(p_stream);

	StreamingDependency::reset(_streaming_dependency, p_stream, get_generator());

	_on_stream_params_changed();
}

Ref<VoxelStream> VoxelTerrain::get_stream() const {
	return _data->get_stream();
}

void VoxelTerrain::set_generator(Ref<VoxelGenerator> p_generator) {
	if (p_generator == get_generator()) {
		return;
	}

	Ref<VoxelGenerator> prev_generator = get_generator();
	if (prev_generator.is_valid()) {
		prev_generator->clear_cache();
		// TODO 如果我们要在多个地形上共享这个生成器，缓存不应被完全清空。相反，
		// 我们应该只从所有配对的观察者中移除该区域。
	}

	_data->set_generator(p_generator);

	MeshingDependency::reset(_meshing_dependency, _mesher, p_generator);
	StreamingDependency::reset(_streaming_dependency, get_stream(), p_generator);

	_on_stream_params_changed();
}

Ref<VoxelGenerator> VoxelTerrain::get_generator() const {
	return _data->get_generator();
}

// void VoxelTerrain::_set_block_size_po2(int p_block_size_po2) {
// 	_data_map.create(0);
// }

unsigned int VoxelTerrain::get_data_block_size_pow2() const {
	return _data->get_block_size_po2();
}

unsigned int VoxelTerrain::get_mesh_block_size_pow2() const {
	return _mesh_block_size_po2;
}

void VoxelTerrain::set_mesh_block_size(unsigned int mesh_block_size) {
	mesh_block_size = math::clamp(mesh_block_size, get_data_block_size(), constants::MAX_BLOCK_SIZE);

	unsigned int po2;
	switch (mesh_block_size) {
		case 16:
			po2 = 4;
			break;
		case 32:
			po2 = 5;
			break;
		default:
			mesh_block_size = 16;
			po2 = 4;
			break;
	}
	if (mesh_block_size == get_mesh_block_size()) {
		return;
	}

	_mesh_block_size_po2 = po2;

	// 无论引用计数如何，卸载所有网格数据块
	clear_mesh_map();

	// 让配对的观察者重新观察新的可网格化区域
	for (unsigned int i = 0; i < _paired_viewers.size(); ++i) {
		PairedViewer &viewer = _paired_viewers[i];
		// 两者都重置，因为这是一次重新初始化。
		// 我们也可以在它们被移位之前或之后做这件事。
		viewer.state.mesh_box = Box3i();
		viewer.prev_state.mesh_box = Box3i();
	}

#ifdef VOXEL_ENABLE_INSTANCER
	// 在此之后做，因为 `on_mesh_block_exit` 可能使用旧的尺寸
	if (_instancer != nullptr) {
		_instancer->set_mesh_block_size_po2(po2);
	}
#endif

	// VoxelEngine::get_singleton().set_volume_render_block_size(_volume_id, mesh_block_size);

	// 不需要更新边界，因为只要它是数据块大小的倍数，我们就可以支持不匹配
	// set_bounds(_bounds_in_voxels);
}

void VoxelTerrain::restart_stream() {
	_on_stream_params_changed();
}

void VoxelTerrain::_on_stream_params_changed() {
	stop_streamer();
	stop_updater();

	// if (_stream.is_valid()) {
	// 	const int stream_block_size_po2 = _stream->get_block_size_po2();
	// 	_set_block_size_po2(stream_block_size_po2);
	// }

	// 整个地图可能都会改变，因此重新生成它
	reset_map();

	_data->set_format(get_internal_format());

	if (((get_stream().is_valid() && get_stream()->is_runnable()) ||
		 (get_generator().is_valid() && get_generator()->is_runnable()))) {
		start_streamer();
		start_updater();
	}

	update_configuration_warnings();
}

void VoxelTerrain::_on_gi_mode_changed() {
	const GeometryInstance3D::GIMode gi_mode = get_gi_mode();
	_mesh_map.for_each_block([gi_mode](VoxelMeshBlockVT &block) { //
		block.set_gi_mode(gi_mode);
	});
}

void VoxelTerrain::_on_shadow_casting_changed() {
	const RenderingServerEnums::ShadowCastingSetting mode =
			RenderingServerEnums::ShadowCastingSetting(get_shadow_casting());
	_mesh_map.for_each_block([mode](VoxelMeshBlockVT &block) { //
		block.set_shadow_casting(mode);
	});
}

void VoxelTerrain::_on_render_layers_mask_changed() {
	const int mask = get_render_layers_mask();
	_mesh_map.for_each_block([mask](VoxelMeshBlockVT &block) { //
		block.set_render_layers_mask(mask);
	});
}

Ref<VoxelMesher> VoxelTerrain::get_mesher() const {
	return _mesher;
}

void VoxelTerrain::set_mesher(Ref<VoxelMesher> mesher) {
	if (mesher == _mesher) {
		return;
	}

	_mesher = mesher;

	MeshingDependency::reset(_meshing_dependency, _mesher, get_generator());

	stop_updater();

	if (_mesher.is_valid()) {
		start_updater();
		// 体素外观可能会完全改变
		remesh_all_blocks();
	}

	update_configuration_warnings();
}

void VoxelTerrain::get_viewers_in_area(StdVector<ViewerID> &out_viewer_ids, Box3i voxel_box) const {
	const Box3i block_box = voxel_box.downscaled(get_data_block_size());

	for (auto it = _paired_viewers.begin(); it != _paired_viewers.end(); ++it) {
		const PairedViewer &viewer = *it;

		if (viewer.state.data_box.intersects(block_box)) {
			out_viewer_ids.push_back(viewer.id);
		}
	}
}

void VoxelTerrain::set_generate_collisions(bool enabled) {
	_generate_collisions = enabled;
}

void VoxelTerrain::set_collision_layer(int layer) {
	_collision_layer = layer;
	_mesh_map.for_each_block([layer](VoxelMeshBlockVT &block) { //
		block.set_collision_layer(layer);
	});
}

int VoxelTerrain::get_collision_layer() const {
	return _collision_layer;
}

void VoxelTerrain::set_collision_mask(int mask) {
	_collision_mask = mask;
	_mesh_map.for_each_block([mask](VoxelMeshBlockVT &block) { //
		block.set_collision_mask(mask);
	});
}

int VoxelTerrain::get_collision_mask() const {
	return _collision_mask;
}

void VoxelTerrain::set_collision_margin(float margin) {
	_collision_margin = margin;
	_mesh_map.for_each_block([margin](VoxelMeshBlockVT &block) { //
		block.set_collision_margin(margin);
	});
}

float VoxelTerrain::get_collision_margin() const {
	return _collision_margin;
}

int VoxelTerrain::get_max_view_distance() const {
	return _max_view_distance_voxels;
}

void VoxelTerrain::set_max_view_distance(int distance_in_voxels) {
	ERR_FAIL_COND(distance_in_voxels < 0);
	_max_view_distance_voxels = distance_in_voxels;

#ifdef VOXEL_ENABLE_INSTANCER
	if (_instancer != nullptr) {
		_instancer->update_mesh_lod_distances_from_parent();
	}
#endif
}

void VoxelTerrain::set_block_enter_notification_enabled(bool enable) {
	_block_enter_notification_enabled = enable;

	if (enable == false) {
		for (auto it = _loading_blocks.begin(); it != _loading_blocks.end(); ++it) {
			LoadingBlock &lb = it->second;
			lb.viewers_to_notify.clear();
		}
	}
}

bool VoxelTerrain::is_block_enter_notification_enabled() const {
	return _block_enter_notification_enabled;
}

void VoxelTerrain::set_area_edit_notification_enabled(bool enable) {
	_area_edit_notification_enabled = enable;
}

bool VoxelTerrain::is_area_edit_notification_enabled() const {
	return _area_edit_notification_enabled;
}

void VoxelTerrain::set_automatic_loading_enabled(bool enable) {
	_automatic_loading_enabled = enable;
}

bool VoxelTerrain::is_automatic_loading_enabled() const {
	return _automatic_loading_enabled;
}

void VoxelTerrain::try_schedule_mesh_update(VoxelMeshBlockVT &mesh_block) {
	VOXEL_PROFILE_SCOPE();
	if (mesh_block.is_in_update_list) {
		// 已在列表中
		return;
	}
	if (mesh_block.mesh_viewers.get() == 0 && mesh_block.collision_viewers.get() == 0) {
		// 没有观察者想要这个数据块的网格（那为什么还要调用这个函数？）
		return;
	}

	const int render_to_data_factor = get_mesh_block_size() / get_data_block_size();

	const Box3i data_box =
			Box3i(mesh_block.position * render_to_data_factor, Vector3iUtil::create(render_to_data_factor)).padded(1);

	// 如果此时得到空盒子，说明调用方出了问题
	VOXEL_ASSERT_RETURN(!data_box.is_empty());

	const bool data_available = _data->has_all_blocks_in_area(data_box, 0);

	if (data_available) {
		// 无论更新器是否已经在更新该数据块，
		// 数据块都可能已被再次修改，因此我们调度另一次更新
		mesh_block.is_in_update_list = true;
		_blocks_pending_update.push_back(mesh_block.position);
	}
}

void VoxelTerrain::view_mesh_block(Vector3i bpos, bool mesh_flag, bool collision_flag) {
	if (mesh_flag == false && collision_flag == false) {
		// 为什么还要调用这个函数？
		return;
	}

	VoxelMeshBlockVT *block = _mesh_map.get_block(bpos);

	if (block == nullptr) {
		// 未找到则创建
		block = VOXEL_NEW(VoxelMeshBlockVT(bpos, get_mesh_block_size()));
		block->set_world(get_world_3d());
		_mesh_map.set_block(bpos, block);
	}
	CRASH_COND(block == nullptr);

	if (mesh_flag) {
		block->mesh_viewers.add();
	}
	if (collision_flag) {
		block->collision_viewers.add();
	}

	// 当观察者想要在已有数据块的位置查看网格时，需要这个调用。
	// 在此之前，网格只在数据块被加载或修改时更新，
	// 因此更改数据块大小或观察者标志不会让网格出现。
	try_schedule_mesh_update(*block);

	// TODO 即使已经有网格，此逻辑也会调度网格更新。它掩盖了一个事实：混用带碰撞的观察者
	// 和不带碰撞的观察者，并不会单独创建碰撞体/网格。

	// TODO 目前不支持游戏中观察者标志变化的情况。
	// 它们必须被重新创建，这可能导致世界重新加载……
}

void VoxelTerrain::unview_mesh_block(Vector3i bpos, bool mesh_flag, bool collision_flag) {
	VoxelMeshBlockVT *block = _mesh_map.get_block(bpos);
	// 网格数据块在第一次 view 调用时创建，
	// 因此如果我们来到这里，就意味着在没有先 view 的情况下 unview
	ERR_FAIL_COND(block == nullptr);

	if (mesh_flag) {
		block->mesh_viewers.remove();
		if (block->mesh_viewers.get() == 0) {
			// 不再需要网格
			block->drop_mesh();
			block->set_visible(false);
		}
	}

	if (collision_flag) {
		block->collision_viewers.remove();
		if (block->collision_viewers.get() == 0) {
			// 不再需要碰撞
			block->drop_collision();
			block->set_collision_enabled(false);
		}
	}

	if (block->mesh_viewers.get() == 0 && block->collision_viewers.get() == 0) {
		unload_mesh_block(bpos);
	}
}

void VoxelTerrain::unload_mesh_block(Vector3i bpos) {
	StdVector<Vector3i> &blocks_pending_update = _blocks_pending_update;

	bool was_loaded = false;
	_mesh_map.remove_block(bpos, [&blocks_pending_update, &was_loaded](const VoxelMeshBlockVT &block) {
		if (block.is_in_update_list) {
			// 该数据块在流程循环稍后要更新的数据块列表中，我们需要将其注销。
			// 我们期望该数据块就在那个列表中。如果不是，说明它的状态出了问题。
			ERR_FAIL_COND(!unordered_remove_value(blocks_pending_update, block.position));
		}
		was_loaded = block.is_loaded;
	});

#ifdef VOXEL_ENABLE_INSTANCER
	if (_instancer != nullptr) {
		_instancer->on_mesh_block_exit(bpos, 0);
	}
#endif

	// 数据块可能是在观察者移动时被添加的，但没有时间接收它的第一次网格更新
	if (was_loaded) {
		emit_mesh_block_exited(bpos);
	}
}

void VoxelTerrain::save_all_modified_blocks(bool with_copy, std::shared_ptr<AsyncDependencyTracker> tracker) {
	VOXEL_PROFILE_SCOPE();
	Ref<VoxelStream> stream = get_stream();
	ERR_FAIL_COND_MSG(stream.is_null(), "Attempting to save modified blocks, but there is no stream to save them to.");

	BufferedTaskScheduler &task_scheduler = BufferedTaskScheduler::get_for_current_thread();

	// 这可能会导致卡顿，因此应该在玩家注意不到的时候使用
	_data->consume_all_modifications(_blocks_to_save, with_copy);

#ifdef VOXEL_ENABLE_INSTANCER
	if (stream.is_valid() && _instancer != nullptr && stream->supports_instance_blocks()) {
		_instancer->save_all_modified_blocks(task_scheduler, tracker, true);
	}
#endif

	consume_block_data_save_requests(
			task_scheduler,
			tracker,
			// 如果流使用缓存，则要求我们刚收集的所有数据被写入磁盘。因此，如果
			// 在所有任务完成后游戏崩溃或被终止，数据不会丢失。
			true
	);

	if (tracker != nullptr) {
		// 使用缓冲计数而不是 `_blocks_to_save`，因为它也可能包含来自 VoxelInstancer 的任务
		tracker->set_count(task_scheduler.get_io_count());
	}

	// 调度所有任务
	task_scheduler.flush();
}

const VoxelTerrain::Stats &VoxelTerrain::get_stats() const {
	return _stats;
}

Node3D *VoxelTerrain::convert_to_nodes(const BitField<NodeConversionFlags> flags) const {
	Node3D *root = memnew(Node3D);
	root->set_name(get_name());
	root->set_transform(get_transform());

	const GeometryInstance3D::GIMode gi_mode = get_gi_mode();
	const GeometryInstance3D::ShadowCastingSetting shadow_casting = get_shadow_casting();
	const int render_layers_mask = get_render_layers_mask();

	Ref<Material> non_shader_material = get_material_override();
	Ref<ShaderMaterial> shader_material = non_shader_material;
	if (shader_material.is_valid()) {
		non_shader_material = Ref<Material>();
	}

	const int block_size = get_mesh_block_size();

	_mesh_map.for_each_block(
			[root, flags, gi_mode, shadow_casting, render_layers_mask, non_shader_material, block_size](
					const VoxelMeshBlockVT &block
			) {
				if (!flags.has_flag(NODE_CONVERSION_INCLUDE_INVISIBLE_BLOCKS)) {
					if (!block.is_visible()) {
						return;
					}
				}
				Ref<Mesh> mesh = block.get_mesh();

				if (mesh.is_valid()) {
					MeshInstance3D *mi = memnew(MeshInstance3D);
					mi->set_name(String("Block_{0}_{1}_{2}")
										 .format(varray(block.position.x, block.position.y, block.position.z)));
					mi->set_mesh(mesh);
					const Transform3D transform(Basis(), Vector3(block.position * block_size));
					mi->set_transform(transform);
					mi->set_visible(block.is_visible());
					mi->set_gi_mode(gi_mode);
					mi->set_cast_shadows_setting(shadow_casting);
					mi->set_layer_mask(render_layers_mask);

					if (flags.has_flag(NODE_CONVERSION_INCLUDE_MATERIAL_OVERRIDES)) {
						if (non_shader_material.is_valid()) {
							mi->set_material_override(non_shader_material);
						}
					}

					root->add_child(mi);
				}
			}
	);

#ifdef VOXEL_ENABLE_INSTANCER
	if (flags.has_flag(NODE_CONVERSION_INCLUDE_INSTANCER) && _instancer != nullptr) {
		Node *instances_root = _instancer->convert_to_nodes(
				flags.has_flag(NODE_CONVERSION_INCLUDE_MATERIAL_OVERRIDES)
						? VoxelInstancer::NODE_CONVERSION_INCLUDE_MATERIAL_OVERRIDES
						: 0
		);
		if (instances_root != nullptr) {
			root->add_child(instances_root);
		}
	}
#endif

	return root;
}

#ifdef VOXEL_ENABLE_INSTANCER
void VoxelTerrain::set_instancer(VoxelInstancer *instancer) {
	if (_instancer != nullptr && instancer != nullptr) {
		ERR_FAIL_COND_MSG(_instancer != nullptr, "No more than one VoxelInstancer per terrain");
	}
	_instancer = instancer;
}
#endif

void VoxelTerrain::get_meshed_block_positions(StdVector<Vector3i> &out_positions) const {
	_mesh_map.for_each_block([&out_positions](const VoxelMeshBlock &mesh_block) {
		if (mesh_block.has_mesh()) {
			out_positions.push_back(mesh_block.position);
		}
	});
}

// 这个函数目前主要用于编辑器用例。
// 它比使用实例生成事件更慢，
// 因为它必须查询 VisualServer，而 VisualServer 会分配并解码顶点缓冲区（假设它们被缓存）。
Array VoxelTerrain::get_mesh_block_surface(Vector3i block_pos) const {
	VOXEL_PROFILE_SCOPE();

	Ref<Mesh> mesh;
	{
		const VoxelMeshBlockVT *block = _mesh_map.get_block(block_pos);
		if (block != nullptr) {
			mesh = block->get_mesh();
		}
	}

	if (mesh.is_valid()) {
		return mesh->surface_get_arrays(0);
	}

	return Array();
}

Dictionary VoxelTerrain::_b_get_statistics() const {
	Dictionary d;

	// _process 中耗时细分
	d["time_detect_required_blocks"] = _stats.time_detect_required_blocks;
	d["time_request_blocks_to_load"] = _stats.time_request_blocks_to_load;
	d["time_process_load_responses"] = _stats.time_process_load_responses;
	d["time_request_blocks_to_update"] = _stats.time_request_blocks_to_update;

	d["dropped_block_loads"] = _stats.dropped_block_loads;
	d["dropped_block_meshs"] = _stats.dropped_block_meshs;
	d["updated_blocks"] = _stats.updated_blocks;

	return d;
}

void VoxelTerrain::start_updater() {
	Ref<VoxelMesherBlocky> blocky_mesher = _mesher;
	if (blocky_mesher.is_valid()) {
		Ref<VoxelBlockyLibraryBase> library = blocky_mesher->get_library();
		if (library.is_valid()) {
			// TODO 有没有办法在 TRES 资源加载器完成加载后立即执行此函数？
			// VoxelBlockyLibrary 应该像 MeshLibrary 一样提前烘焙
			library->bake();
		}
	}

	// VoxelEngine::get_singleton().set_volume_mesher(_volume_id, _mesher);
}

void VoxelTerrain::stop_updater() {
	// 使待处理的任务失效
	MeshingDependency::reset(_meshing_dependency, _mesher, get_generator());

	// VoxelEngine::get_singleton().set_volume_mesher(_volume_id, Ref<VoxelMesher>());

	// TODO 在此之后我们仍可能收到一些延迟的网格更新。这会是个问题吗？
	//_reception_buffers.mesh_output.clear();

	for (const Vector3i bpos : _blocks_pending_update) {
		VoxelMeshBlockVT *block = _mesh_map.get_block(bpos);
		if (block != nullptr) {
			block->is_in_update_list = false;
		}
	}

	_blocks_pending_update.clear();
}

void VoxelTerrain::remesh_all_blocks() {
	_mesh_map.for_each_block([this](VoxelMeshBlockVT &block) { //
		try_schedule_mesh_update(block);
	});
}

// 目前，这个函数用于多人在线场景中的客户端侧用例
void VoxelTerrain::generate_block_async(Vector3i block_position) {
	if (_data->has_block(block_position, 0)) {
		// 已存在
		return;
	}
	if (_loading_blocks.find(block_position) != _loading_blocks.end()) {
		// 正在加载
		return;
	}

	// if (require_notification) {
	// 	new_loading_block.viewers_to_notify.push_back(viewer_id);
	// }

	LoadingBlock new_loading_block;
	const Box3i block_box(_data->block_to_voxel(block_position), Vector3iUtil::create(_data->get_block_size()));
	for (size_t i = 0; i < _paired_viewers.size(); ++i) {
		const PairedViewer &viewer = _paired_viewers[i];
		if (viewer.state.data_box.intersects(block_box)) {
			new_loading_block.viewers.add();
		}
	}

	if (new_loading_block.viewers.get() == 0) {
		return;
	}

	// 调度一个加载请求
	// TODO 这也可能最终会从流中加载
	_loading_blocks.insert({ block_position, new_loading_block });
	_blocks_pending_load.push_back(block_position);
}

void VoxelTerrain::start_streamer() {
	// VoxelEngine::get_singleton().set_volume_stream(_volume_id, _stream);
	// VoxelEngine::get_singleton().set_volume_generator(_volume_id, _generator);
}

void VoxelTerrain::stop_streamer() {
	// 使待处理的任务失效
	StreamingDependency::reset(_streaming_dependency, get_stream(), get_generator());
	// VoxelEngine::get_singleton().set_volume_stream(_volume_id, Ref<VoxelStream>());
	// VoxelEngine::get_singleton().set_volume_generator(_volume_id, Ref<VoxelGenerator>());
	_loading_blocks.clear();
	_blocks_pending_load.clear();
	_quick_reloading_blocks.clear();
	_unloaded_saving_blocks.clear();
}

void VoxelTerrain::clear_mesh_map() {
#ifdef VOXEL_ENABLE_INSTANCER
	if (_instancer != nullptr) {
		VoxelInstancer &instancer = *_instancer;
		_mesh_map.for_each_block([&instancer, this](VoxelMeshBlockVT &block) { //
			instancer.on_mesh_block_exit(block.position, 0);
			if (block.is_loaded) {
				emit_mesh_block_exited(block.position);
			}
		});
	} else
#endif
	{
		_mesh_map.for_each_block([this](VoxelMeshBlockVT &block) { //
			if (block.is_loaded) {
				emit_mesh_block_exited(block.position);
			}
		});
	}

	_mesh_map.clear();
}

void VoxelTerrain::reset_map() {
	// 丢弃一切，以便重新加载全部

	_data->for_each_block_position([this](const Vector3i &bpos) { //
		emit_data_block_unloaded(bpos);
	});
	_data->reset_maps();

	clear_mesh_map();

	_loading_blocks.clear();
	_blocks_pending_load.clear();
	_blocks_pending_update.clear();
	_blocks_to_save.clear();

	// 无需关心引用计数，反正我们会丢弃所有内容。将在下次 process 时重新配对。
	_paired_viewers.clear();

	Ref<VoxelGenerator> generator = get_generator();
	if (generator.is_valid()) {
		generator->clear_cache();
	}
}

void VoxelTerrain::post_edit_voxel(Vector3i pos) {
	post_edit_area(Box3i(pos, Vector3i(1, 1, 1)), true);
}

void VoxelTerrain::try_schedule_mesh_update_from_data(const Box3i &box_in_voxels) {
	VOXEL_PROFILE_SCOPE();
	if (_mesher.is_null()) {
		// 没有网格器，无法进行更新
		return;
	}
	// 我们填充 1，因为相邻数据块可能在视觉上受影响（例如，烘焙的环境光遮蔽）
	const Box3i mesh_box = box_in_voxels.padded(1).downscaled(get_mesh_block_size());
	mesh_box.for_each_cell([this](Vector3i pos) {
		VoxelMeshBlockVT *block = _mesh_map.get_block(pos);
		// 不一定存在网格数据块，如果编辑发生在边界处，
		// 或者编辑发生在不需要网格的观察者旁边
		if (block != nullptr) {
			try_schedule_mesh_update(*block);
		}
	});
}

void VoxelTerrain::post_edit_area(Box3i box_in_voxels, bool update_mesh) {
	_data->mark_area_modified(box_in_voxels, nullptr, false);

	box_in_voxels.clip(_data->get_bounds());

	// TODO 也许可以删除这个，而优先使用多人同步器的虚函数？
	if (_area_edit_notification_enabled) {
		GDVIRTUAL_CALL(_on_area_edited, box_in_voxels.position, box_in_voxels.size);
	}

	if (_multiplayer_synchronizer != nullptr && _multiplayer_synchronizer->is_server()) {
		// TODO 当用户在某区域内进行大量单独修改时，这样做效率不高。
		// 我们要么需要以某种方式批量处理修改过的区域，要么向用户暴露一个事务性 API
		// （begin(area)、在区域内编辑、end(area)）
		_multiplayer_synchronizer->send_area(box_in_voxels);
	}

	if (update_mesh) {
		try_schedule_mesh_update_from_data(box_in_voxels);

#ifdef VOXEL_ENABLE_INSTANCER
		if (_instancer != nullptr) {
			_instancer->on_area_edited(box_in_voxels);
		}
#endif
	}
}

void VoxelTerrain::_notification(int p_what) {
	struct SetWorldAction {
		World3D *world;
		SetWorldAction(World3D *w) : world(w) {}
		void operator()(VoxelMeshBlockVT &block) {
			block.set_world(world);
		}
	};

	struct SetParentVisibilityAction {
		bool visible;
		SetParentVisibilityAction(bool v) : visible(v) {}
		void operator()(VoxelMeshBlockVT &block) {
			block.set_parent_visible(visible);
		}
	};

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			set_process(true);
#ifdef TOOLS_ENABLED
#ifdef VOXEL_ENABLE_SMOOTH_MESHING
			// 在编辑器中，为了方便，自动配置一个默认网格器。
			// 因为 Godot 有一个属性提示可以自动实例化资源，但如果该资源是
			// 抽象的，它就不起作用……而且它不能是默认值，因为这种做法在 Godot 4 中已被
			// 弃用并带有警告。
			if (Engine::get_singleton()->is_editor_hint() && !get_mesher().is_valid()) {
				Ref<VoxelMesherTransvoxel> mesher;
				mesher.instantiate();
				set_mesher(mesher);
			}
#endif
#endif
			break;

		case NOTIFICATION_PROCESS:
			// 不能在这里做，因为 Godot 此时"仍在设置子节点"。
			// 也不能在 ready 中做，因为 Godot 说节点状态已被锁定。
			// 这个 hack 相当糟糕。
			VoxelEngineUpdater::ensure_existence(get_tree());

			process();
			break;

		case NOTIFICATION_EXIT_TREE:
			break;

		case NOTIFICATION_ENTER_WORLD: {
			World3D *world = *get_world_3d();
			_mesh_map.for_each_block(SetWorldAction(world));
#ifdef TOOLS_ENABLED
			if (debug_is_draw_enabled()) {
				_debug_renderer.set_world(is_visible_in_tree() ? world : nullptr);
			}
#endif
		} break;

		case NOTIFICATION_EXIT_WORLD:
			_mesh_map.for_each_block(SetWorldAction(nullptr));
#ifdef TOOLS_ENABLED
			_debug_renderer.set_world(nullptr);
#endif
			break;

		case NOTIFICATION_VISIBILITY_CHANGED:
			_mesh_map.for_each_block(SetParentVisibilityAction(is_visible()));
#ifdef TOOLS_ENABLED
			if (debug_is_draw_enabled()) {
				_debug_renderer.set_world(is_visible_in_tree() ? *get_world_3d() : nullptr);
			}
#endif
			break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			const Transform3D transform = get_global_transform();
			// VoxelEngine::get_singleton().set_volume_transform(_volume_id, transform);

			if (!is_inside_tree()) {
				// 变换和其它属性可以由场景加载器设置，
				// 在我们进入场景树之前
				return;
			}

			_mesh_map.for_each_block([&transform](VoxelMeshBlockVT &block) { //
				block.set_parent_transform(transform);
			});

		} break;

		default:
			break;
	}
}

namespace {

Vector3i get_block_center(Vector3i pos, int bs) {
	return pos * bs + Vector3iUtil::create(bs / 2);
}

void init_sparse_grid_priority_dependency(
		PriorityDependency &dep,
		Vector3i block_position,
		int block_size,
		std::shared_ptr<PriorityDependency::ViewersData> &shared_viewers_data,
		const Transform3D &volume_transform
) {
	const Vector3i voxel_pos = get_block_center(block_position, block_size);
	const float block_radius = block_size / 2;
	dep.shared = shared_viewers_data;
	dep.world_position = to_vec3f(volume_transform.xform(voxel_pos));
	const float transformed_block_radius =
			volume_transform.basis.xform(Vector3(block_radius, block_radius, block_radius)).length();

	// 超出此距离后，任何视野都无法与该数据块重叠。
	// 将数据块半径加倍，以计入额外的数据块边距，
	// 因为网格化时需要它们作为邻居
	dep.drop_distance_squared =
			math::squared(shared_viewers_data->highest_view_distance + 2.f * transformed_block_radius);
}

void request_block_load(
		VolumeID volume_id,
		std::shared_ptr<StreamingDependency> stream_dependency,
		Vector3i block_pos,
		std::shared_ptr<PriorityDependency::ViewersData> &shared_viewers_data,
		const Transform3D volume_transform,
		BufferedTaskScheduler &scheduler,
		bool use_gpu,
		const std::shared_ptr<VoxelData> &voxel_data
) {
	VOXEL_ASSERT(stream_dependency != nullptr);

#ifdef VOXEL_ENABLE_GPU
	if (use_gpu && (stream_dependency->generator.is_null() || !stream_dependency->generator->supports_shaders())) {
		use_gpu = false;
	}
#endif

	const unsigned int data_block_size = voxel_data->get_block_size();

	if (stream_dependency->stream.is_valid()) {
		PriorityDependency priority_dependency;
		init_sparse_grid_priority_dependency(
				priority_dependency, block_pos, data_block_size, shared_viewers_data, volume_transform
		);

		const bool request_instances = false;
		LoadBlockDataTask *task = VOXEL_NEW(LoadBlockDataTask(
				volume_id,
				block_pos,
				0,
				data_block_size,
				request_instances,
				stream_dependency,
				priority_dependency,
				true,
				use_gpu,
				voxel_data,
				TaskCancellationToken()
		));

		scheduler.push_io_task(task);

	} else {
		// 不检查流，直接生成数据块
		ERR_FAIL_COND(stream_dependency->generator.is_null());

		VoxelGenerator::BlockTaskParams params;
		params.format = voxel_data->get_format();
		params.volume_id = volume_id;
		params.block_position = block_pos;
		params.block_size = data_block_size;
		params.stream_dependency = stream_dependency;
#ifdef VOXEL_ENABLE_GPU
		params.use_gpu = use_gpu;
#endif
		params.data = voxel_data;

		init_sparse_grid_priority_dependency(
				params.priority_dependency, block_pos, data_block_size, shared_viewers_data, volume_transform
		);

		IThreadedTask *task = stream_dependency->generator->create_block_task(params);

		scheduler.push_main_task(task);
	}
}

} // namespace

void VoxelTerrain::send_data_load_requests() {
	VOXEL_PROFILE_SCOPE();

	if (_blocks_pending_load.size() > 0) {
		std::shared_ptr<PriorityDependency::ViewersData> shared_viewers_data =
				VoxelEngine::get_singleton().get_shared_viewers_data_from_default_world();

		const Transform3D volume_transform = get_global_transform();

		BufferedTaskScheduler &scheduler = BufferedTaskScheduler::get_for_current_thread();

		// 要加载的数据块
		for (size_t i = 0; i < _blocks_pending_load.size(); ++i) {
			const Vector3i block_pos = _blocks_pending_load[i];

			auto saving_block_it = _unloaded_saving_blocks.find(block_pos);
			const bool quick_reloading = saving_block_it != _unloaded_saving_blocks.end();

			if (quick_reloading) {
				VOXEL_PROFILE_SCOPE_NAMED("Quick reloading");
				// 该数据块已卸载，正在等待保存，但我们希望立刻取回它。这模拟了一个
				// 请求，并将在下次 process 时完成。
				// 理想情况下这不应频繁发生。这是玩家快速来回移动或任务运行器过载时出现的
				// 边缘情况。
				std::shared_ptr<VoxelBuffer> voxel_data =
						make_shared_instance<VoxelBuffer>(VoxelBuffer::ALLOCATOR_POOL);
				// 复制一份，以确保正在保存的版本不会被后续可能的修改所改变。
				saving_block_it->second->copy_to(*voxel_data, true);
				_quick_reloading_blocks.push_back(QuickReloadingBlock{ voxel_data, block_pos });
				// 先不要擦除它，我们可能只在确认它已保存后才这样做
				// _unloaded_saving_blocks.erase(saving_block_it);

				// 备注：
				// 我们能否改变设计，让保存任务实际保存一盒 VoxelData？
				// 要做到这一点，我们必须不删除引用计数变为 0 的数据块。相反，所有权
				// 某种程度上会交给保存任务。该任务会复制修改过的数据块，然后
				// 如果它们仍然有 0 个观察者才删除。
				// 如果观察者移回该区域，它只需再次找到这些数据块，无需加载。
				// 如果在保存期间这些数据块被修改，仍然可以正常工作，因为保存任务
				// 会锁定要保存的区域以进行读取（这目前已经是个问题，因为实际上
				// 根本没有发生任何锁定！）。

			} else {
				request_block_load(
						_volume_id,
						_streaming_dependency,
						block_pos,
						shared_viewers_data,
						volume_transform,
						scheduler,
						_generator_use_gpu,
						_data
				);
			}
		}
		scheduler.flush();
		_blocks_pending_load.clear();
	}
}

void VoxelTerrain::consume_block_data_save_requests(
		BufferedTaskScheduler &task_scheduler,
		std::shared_ptr<AsyncDependencyTracker> saving_tracker,
		bool with_flush
) {
	VOXEL_PROFILE_SCOPE();

	// 需要保存的数据块
	if (get_stream().is_valid()) {
		for (const VoxelData::BlockToSave &b : _blocks_to_save) {
			VOXEL_PRINT_VERBOSE(format("Requesting save of block {}", b.position));

			SaveBlockDataTask *task = VOXEL_NEW(SaveBlockDataTask(
					_volume_id, b.position, 0, b.voxels, _streaming_dependency, saving_tracker, with_flush
			));

			// 没有优先级数据，保存不需要排序。
			task_scheduler.push_io_task(task);
		}
	} else {
		if (_blocks_to_save.size() > 0) {
			VOXEL_PRINT_VERBOSE(format("Not saving {} blocks because no stream is assigned", _blocks_to_save.size()));
		}
	}

	// print_line(String("Sending {0} block requests").format(varray(input.blocks_to_emerge.size())));
	_blocks_to_save.clear();
}

void VoxelTerrain::emit_data_block_loaded(Vector3i bpos) {
	// 不确定是否要直接暴露缓冲区……它们上面的一些内容可以直接获取很有用，
	// 但这也允许脚本编写者以他们本不该做的方式摆弄体素。
	// 示例：在未先加锁的情况下修改体素，而另一个线程可能同时在读取它们。
	// 反过来也可能发生（线程任务在修改体素时你试图读取它们）。目前不计划暴露
	// VoxelBuffer 锁，因为数量太多，未来可能会转向另一个系统，甚至可能被改为不再继承
	// Reference。因此除非绝对必要，缓冲区不会被暴露。解决办法：使用 VoxelTool
	// const Variant vbuffer = block->voxels;
	// const Variant *args[2] = { &vpos, &vbuffer };
	emit_signal(VoxelStringNames::get_singleton().block_loaded, bpos);
}

void VoxelTerrain::emit_data_block_unloaded(Vector3i bpos) {
	emit_signal(VoxelStringNames::get_singleton().block_unloaded, bpos);
}

void VoxelTerrain::emit_mesh_block_entered(Vector3i bpos) {
	// 不确定是否要直接暴露缓冲区……它们上面的一些内容可以直接获取很有用，
	// 但这也允许脚本编写者以他们本不该做的方式摆弄体素。
	// 示例：在未先加锁的情况下修改体素，而另一个线程可能同时在读取它们。
	// 反过来也可能发生（线程任务在修改体素时你试图读取它们）。目前不计划暴露
	// VoxelBuffer 锁，因为数量太多，未来可能会转向另一个系统，甚至可能被改为不再继承
	// Reference。因此除非绝对必要，缓冲区不会被暴露。解决办法：使用 VoxelTool
	// const Variant vbuffer = block->voxels;
	// const Variant *args[2] = { &vpos, &vbuffer };
	emit_signal(VoxelStringNames::get_singleton().mesh_block_entered, bpos);
}

void VoxelTerrain::emit_mesh_block_exited(Vector3i bpos) {
	emit_signal(VoxelStringNames::get_singleton().mesh_block_exited, bpos);
}

bool VoxelTerrain::try_get_paired_viewer_index(ViewerID id, size_t &out_i) const {
	for (size_t i = 0; i < _paired_viewers.size(); ++i) {
		const PairedViewer &p = _paired_viewers[i];
		if (p.id == id) {
			out_i = i;
			return true;
		}
	}
	return false;
}

// TODO 尚不清楚这个 API 是否会保留。我感觉到它可能会消耗大量 CPU
void VoxelTerrain::notify_data_block_enter(const VoxelDataBlock &block, Vector3i bpos, ViewerID viewer_id) {
	if (!VoxelEngine::get_singleton().viewer_exists(viewer_id)) {
		// 观察者可能在我们请求数据块与加载完成之间的时刻被移除
		return;
	}
	if (_data_block_enter_info_obj == nullptr) {
		_data_block_enter_info_obj = voxel::godot::make_unique<VoxelDataBlockEnterInfo>();
	}
	const int network_peer_id = VoxelEngine::get_singleton().get_viewer_network_peer_id(viewer_id);
	_data_block_enter_info_obj->network_peer_id = network_peer_id;
	_data_block_enter_info_obj->voxel_block = block;
	_data_block_enter_info_obj->block_position = bpos;

	if (!GDVIRTUAL_CALL(_on_data_block_entered, _data_block_enter_info_obj.get()) &&
		_multiplayer_synchronizer == nullptr) {
		WARN_PRINT_ONCE("VoxelTerrain::_on_data_block_entered is unimplemented!");
	}

	if (_multiplayer_synchronizer != nullptr && !Engine::get_singleton()->is_editor_hint() &&
		network_peer_id != MultiplayerPeer::TARGET_PEER_SERVER && _multiplayer_synchronizer->is_server()) {
		_multiplayer_synchronizer->send_block(network_peer_id, block, bpos);
	}
}

void VoxelTerrain::process() {
	VOXEL_PROFILE_SCOPE();

#ifdef VOXEL_ENABLE_GPU
	if (get_generator_use_gpu()) {
		Ref<VoxelGenerator> generator = get_generator();
		if (generator.is_valid() && generator->supports_shaders() &&
			generator->get_block_rendering_shader() == nullptr) {
			generator->compile_shaders();
		}
	}
#endif

	{
		for (const QuickReloadingBlock &qrb : _quick_reloading_blocks) {
			VoxelEngine::BlockDataOutput ob{
				VoxelEngine::BlockDataOutput::TYPE_LOADED, //
				qrb.voxels, //
#ifdef VOXEL_ENABLE_INSTANCER
				// TODO 这不能与 VoxelInstancer 一起使用，因为它基于网格来卸载……
				nullptr, //
#endif
				qrb.position, //
				0, // lod_index
				false, // dropped
				false, // max_lod_hint
				false, // initial_load
				false, // had_instances
				true // had_voxels
			};
			apply_data_block_response(ob);
		}
		_quick_reloading_blocks.clear();
	}

	process_viewers();
	// process_received_data_blocks();
	process_meshing();

#ifdef TOOLS_ENABLED
	if (debug_is_draw_enabled() && is_visible_in_tree()) {
		process_debug_draw();
	}
#endif
}

void VoxelTerrain::process_viewers() {
	ProfilingClock profiling_clock;

	// 按配对观察者列表中的索引升序排列
	StdVector<size_t> unpaired_viewer_indexes;

	// 在此同步，以确保任务评估到更新的距离。否则，观察者可能生成（或传送到
	// 远处），触发任务，但如果任务优先级被评估时同步仍未运行，任务可能因为
	// "距离观察者太远"而自我取消。
	// 并不理想，因为 VoxelEngine 已经调用了它，但它应该足够快。
	// 另一种方案是使用显式的取消令牌，VLT Clipbox 中使用了这种方法。
	VoxelEngine::get_singleton().sync_viewers_task_priority_data();

	// 更新观察者
	{
		// 我们的节点还没有边界，所以目前观察者总是配对的。
		// TODO 更新：节点现在有边界了，需要改变这一点

		// 已销毁的观察者
		for (size_t i = 0; i < _paired_viewers.size(); ++i) {
			PairedViewer &p = _paired_viewers[i];
			if (!VoxelEngine::get_singleton().viewer_exists(p.id)) {
				VOXEL_PRINT_VERBOSE(format("Detected destroyed viewer {} in VoxelTerrain", p.id));
				// 将移除解释为观察距离被置零，这样处理数据块加载的同一套代码
				// 也会被用来卸载该观察者所观察的数据块。
				// 我们实际上会在第二遍中移除未配对的观察者。
				p.state.vertical_view_distance_voxels = 0;
				p.state.horizontal_view_distance_voxels = 0;
				// 同时更新盒子，它们不会更新，因为观察者已被移除。
				// 赋值给 prev state，否则在某些情况下重置盒子会使它们等于 prev state，
				// 从而不会发生卸载
				p.prev_state = p.state;
				p.state.data_box = Box3i();
				p.state.mesh_box = Box3i();
				unpaired_viewer_indexes.push_back(i);
			}
		}

		const Transform3D local_to_world_transform = get_global_transform();
		const Transform3D world_to_local_transform = local_to_world_transform.affine_inverse();

		// 注意，这不支持非均匀缩放
		// TODO 可能还有更好的办法
		const float view_distance_scale = world_to_local_transform.basis.xform(Vector3(1, 0, 0)).length();

		const Box3i bounds_in_voxels = _data->get_bounds();

		const Box3i bounds_in_data_blocks = bounds_in_voxels.downscaled(get_data_block_size());
		const Box3i bounds_in_mesh_blocks = bounds_in_voxels.downscaled(get_mesh_block_size());

		struct UpdatePairedViewer {
			VoxelTerrain &self;
			const Box3i bounds_in_data_blocks;
			const Box3i bounds_in_mesh_blocks;
			const Transform3D world_to_local_transform;
			const float view_distance_scale;

			inline void operator()(ViewerID viewer_id, const VoxelEngine::Viewer &viewer) {
				size_t paired_viewer_index;
				if (!self.try_get_paired_viewer_index(viewer_id, paired_viewer_index)) {
					// 新观察者
					PairedViewer p;
					p.id = viewer_id;
					paired_viewer_index = self._paired_viewers.size();
					self._paired_viewers.push_back(p);
					VOXEL_PRINT_VERBOSE(format("Pairing viewer {} to VoxelTerrain", viewer_id));
				}

				PairedViewer &paired_viewer = self._paired_viewers[paired_viewer_index];
				paired_viewer.prev_state = paired_viewer.state;
				PairedViewer::State &state = paired_viewer.state;

				const unsigned int view_distance_voxels_h = static_cast<unsigned int>(
						static_cast<float>(viewer.view_distances.horizontal) * view_distance_scale
				);
				const unsigned int view_distance_voxels_v = static_cast<unsigned int>(
						static_cast<float>(viewer.view_distances.vertical) * view_distance_scale
				);

				const Vector3 local_position = world_to_local_transform.xform(viewer.world_position);

				state.horizontal_view_distance_voxels =
						math::min(view_distance_voxels_h, self._max_view_distance_voxels);
				state.vertical_view_distance_voxels = math::min(view_distance_voxels_v, self._max_view_distance_voxels);

				state.local_position_voxels = math::floor_to_int(local_position);
				state.requires_collisions = VoxelEngine::get_singleton().is_viewer_requiring_collisions(viewer_id);
				state.requires_meshes =
						VoxelEngine::get_singleton().is_viewer_requiring_visuals(viewer_id) && self._mesher.is_valid();

				// 更新数据和网格观察盒子

				const int data_block_size = self.get_data_block_size();
				const int mesh_block_size = self.get_mesh_block_size();

				int view_distance_data_blocks_h;
				int view_distance_data_blocks_v;
				Vector3i data_block_pos;

				if (state.requires_meshes || state.requires_collisions) {
					const int view_distance_mesh_blocks_h =
							math::ceildiv(state.horizontal_view_distance_voxels, mesh_block_size);
					const int view_distance_mesh_blocks_v =
							math::ceildiv(state.vertical_view_distance_voxels, mesh_block_size);

					const int render_to_data_factor = (mesh_block_size / data_block_size);
					const Vector3i mesh_block_pos = math::floordiv(state.local_position_voxels, mesh_block_size);

					// 添加一个数据块的填充，因为网格化需要邻居
					view_distance_data_blocks_h = view_distance_mesh_blocks_h * render_to_data_factor + 1;
					view_distance_data_blocks_v = view_distance_mesh_blocks_v * render_to_data_factor + 1;

					data_block_pos = mesh_block_pos * render_to_data_factor;
					state.mesh_box = Box3i::from_center_extents(
											 mesh_block_pos,
											 Vector3i(
													 view_distance_mesh_blocks_h,
													 view_distance_mesh_blocks_v,
													 view_distance_mesh_blocks_h
											 )
					)
											 .clipped(bounds_in_mesh_blocks);

				} else {
					view_distance_data_blocks_h = math::ceildiv(state.horizontal_view_distance_voxels, data_block_size);
					view_distance_data_blocks_v = math::ceildiv(state.vertical_view_distance_voxels, data_block_size);

					data_block_pos = math::floordiv(state.local_position_voxels, data_block_size);
					state.mesh_box = Box3i();
				}

				state.data_box = Box3i::from_center_extents(
										 data_block_pos,
										 Vector3i(
												 view_distance_data_blocks_h,
												 view_distance_data_blocks_v,
												 view_distance_data_blocks_h
										 )
				)
										 .clipped(bounds_in_data_blocks);
			}
		};

		// 新的观察者和更新。被移除的观察者不会被迭代，但会一直保持配对直到稍后。
		UpdatePairedViewer u{
			*this, bounds_in_data_blocks, bounds_in_mesh_blocks, world_to_local_transform, view_distance_scale
		};
		VoxelEngine::get_singleton().for_each_viewer(u);
	}

	const bool can_load_blocks =
			((_automatic_loading_enabled &&
			  (_multiplayer_synchronizer == nullptr || _multiplayer_synchronizer->is_server())) &&
			 ((get_stream().is_valid() && get_stream()->is_runnable()) ||
			  (get_generator().is_valid() && get_generator()->is_runnable())));

	// 找出哪些数据块需要出现，哪些需要被卸载
	{
		VOXEL_PROFILE_SCOPE();

		for (size_t i = 0; i < _paired_viewers.size(); ++i) {
			const PairedViewer &viewer = _paired_viewers[i];

			{
				const Box3i &new_data_box = viewer.state.data_box;
				const Box3i &prev_data_box = viewer.prev_state.data_box;

				if (prev_data_box != new_data_box) {
					process_viewer_data_box_change(viewer.id, prev_data_box, new_data_box, can_load_blocks);
				}
			}

			{
				const Box3i &new_mesh_box = viewer.state.mesh_box;
				const Box3i &prev_mesh_box = viewer.prev_state.mesh_box;

				if (prev_mesh_box != new_mesh_box) {
					VOXEL_PROFILE_SCOPE();

					// TODO 在观察新数据块之前取消观察旧数据块，有什么理由吗？
					// 因为如果一个观察者被移除而另一个被添加，即使它们的盒子相同，也会重新加载整个区域。

					// 取消观察刚超出范围的数据块
					prev_mesh_box.difference(new_mesh_box, [this, &viewer](Box3i out_of_range_box) {
						out_of_range_box.for_each_cell([this, &viewer](Vector3i bpos) {
							unview_mesh_block(
									bpos, viewer.prev_state.requires_meshes, viewer.prev_state.requires_collisions
							);
						});
					});

					// 观察刚进入范围的数据块
					new_mesh_box.difference(prev_mesh_box, [this, &viewer](Box3i box_to_load) {
						box_to_load.for_each_cell([this, &viewer](Vector3i bpos) {
							// 加载或更新数据块
							view_mesh_block(bpos, viewer.state.requires_meshes, viewer.state.requires_collisions);
						});
					});
				}

				// 如果观察者标志被修改，那些仍保持在观察者范围内的数据块可能也需要一些改变。
				// 这操作的是与上面不同的一组数据块。

				if (viewer.state.requires_collisions != viewer.prev_state.requires_collisions) {
					const Box3i box = new_mesh_box.clipped(prev_mesh_box);
					if (viewer.state.requires_collisions) {
						box.for_each_cell([this](Vector3i bpos) { //
							view_mesh_block(bpos, false, true);
						});

					} else {
						box.for_each_cell([this](Vector3i bpos) { //
							unview_mesh_block(bpos, false, true);
						});
					}
				}

				if (viewer.state.requires_meshes != viewer.prev_state.requires_meshes) {
					const Box3i box = new_mesh_box.clipped(prev_mesh_box);
					if (viewer.state.requires_meshes) {
						box.for_each_cell([this](Vector3i bpos) { //
							view_mesh_block(bpos, true, false);
						});

					} else {
						box.for_each_cell([this](Vector3i bpos) { //
							unview_mesh_block(bpos, true, false);
						});
					}
				}
			}
		}
	}

	_stats.time_detect_required_blocks = profiling_clock.restart();

	// 我们不再需要未配对的观察者。
	for (size_t i = 0; i < unpaired_viewer_indexes.size(); ++i) {
		// 反向迭代，这样需要移除的配对观察者的索引不会因为移除本身而改变
		const size_t vi = unpaired_viewer_indexes[unpaired_viewer_indexes.size() - i - 1];
		VOXEL_PRINT_VERBOSE(format("Unpairing viewer {} from VoxelTerrain", _paired_viewers[vi].id));
		_paired_viewers[vi] = _paired_viewers.back();
		_paired_viewers.pop_back();
	}

	// 用户可能还没有设置流，或者流已关闭
	if (can_load_blocks) {
		send_data_load_requests();
		BufferedTaskScheduler &task_scheduler = BufferedTaskScheduler::get_for_current_thread();
		consume_block_data_save_requests(task_scheduler, nullptr, false);
		task_scheduler.flush();
	}

	_stats.time_request_blocks_to_load = profiling_clock.restart();
}

void VoxelTerrain::process_viewer_data_box_change(
		const ViewerID viewer_id,
		const Box3i prev_data_box,
		const Box3i new_data_box,
		const bool can_load_blocks
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(prev_data_box != new_data_box);

	static thread_local StdVector<Vector3i> tls_missing_blocks;
	static thread_local StdVector<Vector3i> tls_found_blocks_positions;

	Ref<VoxelGenerator> generator = get_generator();
	if (generator.is_valid()) {
		generator->process_viewer_diff(viewer_id, new_data_box, prev_data_box);
	}

	// 取消观察刚超出范围的数据块
	//
	// TODO 在观察新数据块之前取消观察旧数据块，有什么理由吗？
	// 因为如果一个观察者被移除而另一个被添加，即使它们的盒子相同，也会重新加载整个区域。
	{
		const bool may_save = get_stream().is_valid() && get_stream()->is_runnable();

		tls_missing_blocks.clear();
		tls_found_blocks_positions.clear();

		const unsigned int to_save_index0 = _blocks_to_save.size();

		// 递减已加载数据块的引用计数，并卸载它们
		prev_data_box.difference(new_data_box, [this, may_save](Box3i out_of_range_box) {
			// VOXEL_PRINT_VERBOSE(format("Unview data box {}", out_of_range_box));
			_data->unview_area(
					out_of_range_box,
					0,
					&tls_found_blocks_positions,
					&tls_missing_blocks,
					may_save ? &_blocks_to_save : nullptr
			);
		});

		// 将已卸载的数据块临时存储到映射中，直到保存完成
		for (unsigned int i = to_save_index0; i < _blocks_to_save.size(); ++i) {
			const VoxelData::BlockToSave &bts = _blocks_to_save[i];
			_unloaded_saving_blocks[bts.position] = bts.voxels;
		}

		{
			VOXEL_PROFILE_SCOPE_NAMED("Unload signals");
			// 移除加载中的数据块（那些已加载且引用计数降为零的）
			for (const Vector3i bpos : tls_found_blocks_positions) {
				emit_data_block_unloaded(bpos);
				// TODO 如果它们已加载，为什么会在加载中的列表里？
				// 可能是我们移动太快，数据块甚至还没完成加载
				_loading_blocks.erase(bpos);
			}
		}

		// 移除加载中数据块的引用计数，如果降为零则取消加载
		{
			VOXEL_PROFILE_SCOPE_NAMED("Cancel missing blocks");
			for (const Vector3i bpos : tls_missing_blocks) {
				auto loading_block_it = _loading_blocks.find(bpos);
				if (loading_block_it == _loading_blocks.end()) {
					VOXEL_PRINT_VERBOSE("Request to unview a loading block that was never requested");
					// 不符合预期，但我想也没关系
					return;
				}

				LoadingBlock &loading_block = loading_block_it->second;
				loading_block.viewers.remove();

				if (loading_block.viewers.get() == 0) {
					// 不再想要加载它
					_loading_blocks.erase(loading_block_it);

					// TODO 我们真的还需要那个向量吗？
					for (size_t i = 0; i < _blocks_pending_load.size(); ++i) {
						if (_blocks_pending_load[i] == bpos) {
							_blocks_pending_load[i] = _blocks_pending_load.back();
							_blocks_pending_load.pop_back();
							break;
						}
					}
				}
			}
		}
	}

	// 观察进入范围的数据块
	if (can_load_blocks) {
		const bool require_notifications =
				(_block_enter_notification_enabled ||
				 (_multiplayer_synchronizer != nullptr && _multiplayer_synchronizer->is_server())) &&
				VoxelEngine::get_singleton().viewer_exists(viewer_id) && // 可能是已销毁的观察者
				VoxelEngine::get_singleton().is_viewer_requiring_data_block_notifications(viewer_id);

		static thread_local StdVector<VoxelDataBlock> tls_found_blocks;

		tls_missing_blocks.clear();
		tls_found_blocks.clear();
		tls_found_blocks_positions.clear();

		new_data_box.difference(prev_data_box, [this](Box3i box_to_load) {
			// VOXEL_PRINT_VERBOSE(format("View data box {}", box_to_load));
			_data->view_area(box_to_load, 0, &tls_missing_blocks, &tls_found_blocks_positions, &tls_found_blocks);
		});

		// 调度缺失数据块的加载
		{
			VOXEL_PROFILE_SCOPE_NAMED("Gather missing blocks");
			for (const Vector3i missing_bpos : tls_missing_blocks) {
				auto loading_block_it = _loading_blocks.find(missing_bpos);

				if (loading_block_it == _loading_blocks.end()) {
					// 第一个请求它的观察者
					LoadingBlock new_loading_block;
					new_loading_block.viewers.add();

					if (require_notifications) {
						new_loading_block.viewers_to_notify.push_back(viewer_id);
					}

					_loading_blocks.insert({ missing_bpos, new_loading_block });
					_blocks_pending_load.push_back(missing_bpos);

				} else {
					// 更多观察者
					LoadingBlock &loading_block = loading_block_it->second;
					loading_block.viewers.add();

					if (require_notifications) {
						loading_block.viewers_to_notify.push_back(viewer_id);
					}
				}
			}
		}

		if (require_notifications) {
			VOXEL_PROFILE_SCOPE_NAMED("Enter notifications");
			// 对已经加载的数据块发出通知
			for (unsigned int i = 0; i < tls_found_blocks.size(); ++i) {
				const Vector3i bpos = tls_found_blocks_positions[i];
				const VoxelDataBlock &block = tls_found_blocks[i];
				notify_data_block_enter(block, bpos, viewer_id);
			}
		}

		// 确保清空它，因为它持有带引用计数的内容。如果不这样做，可能在退出时崩溃，因为
		// 体素引擎在 thread_locals 被销毁之前就反初始化了它的内容
		tls_found_blocks.clear();

		// TODO 目前不支持游戏中观察者标志变化的情况。
		// 它们必须被重新创建，这可能导致世界重新加载……
	}
}

void VoxelTerrain::apply_data_block_response(VoxelEngine::BlockDataOutput &ob) {
	VOXEL_PROFILE_SCOPE();

	// print_line(String("Receiving {0} blocks").format(varray(output.emerged_blocks.size())));

	if (ob.type == VoxelEngine::BlockDataOutput::TYPE_SAVED) {
		if (ob.dropped) {
			ERR_PRINT(String("Could not save block {0}").format(varray(ob.position)));

		} else if (ob.had_voxels) {
			// TODO 如果保存的版本比我们缓存的版本更旧怎么办？
			// 要出现这个问题，你需要编辑一个数据块、离开、再回来、再次编辑、再离开，
			// 并且让第一次保存先于第二次完成。
			// 但我们可以考虑添加版本号，这需要添加数据块元数据
			_unloaded_saving_blocks.erase(ob.position);

		}
#ifdef VOXEL_ENABLE_INSTANCER
		else if (ob.had_instances && _instancer != nullptr) {
			_instancer->on_data_block_saved(ob.position, ob.lod_index);
		}
#endif
		return;
	}

	CRASH_COND(
			ob.type != VoxelEngine::BlockDataOutput::TYPE_LOADED &&
			ob.type != VoxelEngine::BlockDataOutput::TYPE_GENERATED
	);

	const Vector3i block_pos = ob.position;

	if (ob.dropped) {
		if (_loading_blocks.find(block_pos) == _loading_blocks.end()) {
			// 我们不再期望这个数据块，忽略
			return;
		}
		// 那个数据块被取消了，但我们仍然在期望它。
		// 我们必须再次请求它。
		VOXEL_PRINT_VERBOSE(
				format("Received a block loading drop while we were still expecting it: "
					   "lod{} ({}, {}, {}), re-requesting it",
					   int(ob.lod_index),
					   ob.position.x,
					   ob.position.y,
					   ob.position.z)
		);

		++_stats.dropped_block_loads;

		_blocks_pending_load.push_back(ob.position);
		return;
	}

	LoadingBlock loading_block;
	{
		auto loading_block_it = _loading_blocks.find(block_pos);

		if (loading_block_it == _loading_blocks.end()) {
			// 那个数据块没有被请求，或已不再需要，丢弃它。
			++_stats.dropped_block_loads;
			return;
		}

		// 使用移动语义，因为它可能包含一个已分配的向量
		loading_block = std::move(loading_block_it->second);

		// 现在我们已经得到数据块。如果仍然需要丢弃它，原因将是一个错误。
		_loading_blocks.erase(loading_block_it);
	}

	VOXEL_ASSERT_RETURN(ob.voxels != nullptr);

	VoxelDataBlock block(ob.voxels, ob.lod_index);
	block.set_edited(ob.type == VoxelEngine::BlockDataOutput::TYPE_LOADED);
	// 只有在数据块不存在时才会设置观察者
	block.viewers = loading_block.viewers;

	if (block.has_voxels() && block.get_voxels_const().get_size() != Vector3iUtil::create(_data->get_block_size())) {
		// 体素数据块尺寸不正确，丢弃它
		VOXEL_PRINT_ERROR(
				format("Block is different from expected size. Expected {}, got {}",
					   Vector3iUtil::create(_data->get_block_size()),
					   block.get_voxels_const().get_size())
		);
		++_stats.dropped_block_loads;
		return;
	}

	_data->try_set_block(
			block_pos,
			block,
			[
#ifdef DEBUG_ENABLED
					block_pos
#endif
	](VoxelDataBlock &existing_block, const VoxelDataBlock &incoming_block) {
#ifdef DEBUG_ENABLED
				VOXEL_PRINT_VERBOSE(format("Replacing existing data block {}", block_pos));
#endif
				existing_block.set_voxels(incoming_block.get_voxels_shared());
				existing_block.set_edited(incoming_block.is_edited());
			}
	);

	emit_data_block_loaded(block_pos);

	for (unsigned int i = 0; i < loading_block.viewers_to_notify.size(); ++i) {
		const ViewerID viewer_id = loading_block.viewers_to_notify[i];
		notify_data_block_enter(block, block_pos, viewer_id);
	}

	// 数据块本身可能还不适合进行网格化，但它周围的数据块现在可能可以了
	// TODO 优化：初次加载在这里可能会卡住一段时间。
	// 因为大量数据块同时被加载，导致大量数据块查询。
	try_schedule_mesh_update_from_data(
			Box3i(_data->block_to_voxel(block_pos), Vector3iUtil::create(get_data_block_size()))
	);

	// 我们可能已经再次请求了一些数据块（如果我们仍然需要它们时得到一个被丢弃的）
	// if (stream_enabled) {
	// 	send_block_data_requests();
	// }

	// if (_instancer != nullptr && ob.instances != nullptr) {
	// 	_instancer->on_data_block_loaded(ob.position, ob.lod_index, std::move(ob.instances));
	// }
}

// 设置一个数据块的体素数据，如果有任何现有数据则丢弃。
// 如果给定的数据块坐标不在任何观察者的区域内，此函数不做任何事并返回
// false。如果某个数据块正在此位置加载或生成，它将被取消。
bool VoxelTerrain::try_set_block_data(Vector3i position, std::shared_ptr<VoxelBuffer> &voxel_data) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND_V(voxel_data == nullptr, false);

	const Vector3i expected_block_size = Vector3iUtil::create(_data->get_block_size());
	ERR_FAIL_COND_V_MSG(
			voxel_data->get_size() != expected_block_size,
			false,
			String("Block size is different from expected size. "
				   "Expected {0}, got {1}")
					.format(varray(expected_block_size, voxel_data->get_size()))
	);

	// 设置与此数据块相交的观察者计数
	RefCount refcount;
	for (unsigned int i = 0; i < _paired_viewers.size(); ++i) {
		const PairedViewer &viewer = _paired_viewers[i];
		if (viewer.state.data_box.contains(position)) {
			refcount.add();
		}
	}

	if (refcount.get() == 0) {
		// 实际上，这个数据块甚至不在范围内。所以我们可以忽略它。
		// 如果我们不想要这种行为，可以引入一个虚拟观察者，只要启用它，就会为
		// 该体积中的所有数据块添加引用？
		VOXEL_PRINT_VERBOSE("Trying to set a data block outside of any viewer range");
		return false;
	}

	// 取消加载中的版本（如果有）
	_loading_blocks.erase(position);

	VoxelDataBlock block(voxel_data, 0);
	// TODO 如何设置 `edited` 标志？它在这个函数的用例中重要吗？
	block.set_edited(true);
	block.viewers = refcount;

	// 创建或更新数据块数据
	_data->try_set_block(position, block, [](VoxelDataBlock &existing_block, const VoxelDataBlock &incoming_block) {
		existing_block.set_voxels(incoming_block.get_voxels_shared());
		existing_block.set_edited(incoming_block.is_edited());
	});

	// 数据块本身可能还不适合进行网格化，但它周围的数据块现在可能可以了
	try_schedule_mesh_update_from_data(
			Box3i(_data->block_to_voxel(position), Vector3iUtil::create(get_data_block_size()))
	);

	return true;
}

bool VoxelTerrain::has_data_block(Vector3i position) const {
	return _data->has_block(position, 0);
}

void VoxelTerrain::process_meshing() {
	VOXEL_PROFILE_SCOPE();
	ProfilingClock profiling_clock;

	_stats.dropped_block_meshs = 0;

	// 发送网格更新

	const Transform3D volume_transform = get_global_transform();
	std::shared_ptr<PriorityDependency::ViewersData> shared_viewers_data =
			VoxelEngine::get_singleton().get_shared_viewers_data_from_default_world();

	// const int used_channels_mask = get_used_channels_mask();
	const int mesh_to_data_factor = get_mesh_block_size() / get_data_block_size();

	BufferedTaskScheduler &scheduler = BufferedTaskScheduler::get_for_current_thread();

	for (size_t bi = 0; bi < _blocks_pending_update.size(); ++bi) {
		VOXEL_PROFILE_SCOPE_NAMED("Block");
		const Vector3i mesh_block_pos = _blocks_pending_update[bi];

		VoxelMeshBlockVT *mesh_block = _mesh_map.get_block(mesh_block_pos);

		// 如果我们来到这里，一定是因为调度了一次更新
		VOXEL_ASSERT_CONTINUE(mesh_block != nullptr);
		VOXEL_ASSERT_CONTINUE(mesh_block->is_in_update_list);

		// 填充 1，因为网格化需要邻居
		const Box3i data_box =
				Box3i(mesh_block_pos * mesh_to_data_factor, Vector3iUtil::create(mesh_to_data_factor)).padded(1);

#ifdef DEBUG_ENABLED
		// 我们必须已经取到了有效的数据块
		{
			const Vector3i anchor_pos = data_box.position + Vector3i(1, 1, 1);
			VOXEL_ASSERT_CONTINUE(_data->has_block(anchor_pos, 0));
		}
#endif

		// print_line(String("DDD request {0}").format(varray(mesh_request.render_block_position.to_vec3())));
		// 我们会相当频繁地分配这个。如果它成为问题，应该很容易做对象池化。
		MeshBlockTask *task = VOXEL_NEW(MeshBlockTask);
		task->volume_id = _volume_id;
		task->mesh_block_position = mesh_block_pos;
		task->lod_index = 0;
		task->meshing_dependency = _meshing_dependency;
		task->require_visual = mesh_block->mesh_viewers.get() > 0;
		task->collision_hint = _generate_collisions && mesh_block->collision_viewers.get() > 0;
		task->data = _data;

		// 这个迭代顺序是特意选择的，以匹配 VoxelEngine 和线程化访问
		_data->get_blocks_with_voxel_data(data_box, 0, to_span(task->blocks));
		task->blocks_count = Vector3iUtil::get_volume_u64(data_box.size);

#ifdef DEBUG_ENABLED
		{
			unsigned int count = 0;
			for (unsigned int i = 0; i < task->blocks_count; ++i) {
				if (task->blocks[i] != nullptr) {
					++count;
				}
			}
			// 列表中的这些数据块一定已被调度，因为我们有它们的数据！
			if (count == 0) {
				VOXEL_PRINT_ERROR("Unexpected empty block list in meshing block task");
				VOXEL_DELETE(task);
				continue;
			}
		}
#endif

		init_sparse_grid_priority_dependency(
				task->priority_dependency,
				task->mesh_block_position,
				get_mesh_block_size(),
				shared_viewers_data,
				volume_transform
		);

		scheduler.push_main_task(task);

		mesh_block->is_in_update_list = false;
	}

	scheduler.flush();

	_blocks_pending_update.clear();

	_stats.time_request_blocks_to_update = profiling_clock.restart();

	// print_line(String("d:") + String::num(_dirty_blocks.size()) + String(", q:") +
	// String::num(_block_update_queue.size()));
}

void VoxelTerrain::apply_mesh_update(const VoxelEngine::BlockMeshOutput &ob) {
	VOXEL_PROFILE_SCOPE();
	// print_line(String("DDD receive {0}").format(varray(ob.position.to_vec3())));

	VoxelMeshBlockVT *block = _mesh_map.get_block(ob.position);
	if (block == nullptr) {
		// print_line("- no longer loaded");
		// 该数据块已不再加载，丢弃结果
		++_stats.dropped_block_meshs;
		return;
	}

	if (ob.type == VoxelEngine::BlockMeshOutput::TYPE_DROPPED) {
		// 该数据块已加载，但其网格化请求被丢弃。
		// TODO 不确定在这种情况下该怎么做，发送更新查询的代码需要调整
		VOXEL_PRINT_VERBOSE("Received a block mesh drop while we were still expecting it");
		++_stats.dropped_block_meshs;
		return;
	}

	// 在将 mesher 设为空之后，仍有可能出现一些更新。这样可以避免崩溃。
	if (_mesher.is_null()) {
		++_stats.dropped_block_meshs;
		return;
	}

	Ref<ArrayMesh> mesh;
	Ref<Mesh> shadow_occluder_mesh;
	StdVector<uint16_t> material_indices;
	if (ob.visual_was_required) {
		if (ob.has_mesh_resource) {
			// 网格已作为线程任务的一部分构建完成
			mesh = ob.mesh;
			shadow_occluder_mesh = ob.shadow_occluder_mesh;
			// 它可能为空
			material_indices = std::move(ob.mesh_material_indices);
		} else {
			// 不能在线程中构建网格，在此处构建
			material_indices.clear();
			mesh = build_mesh(
					to_span_const(ob.surfaces.surfaces),
					ob.surfaces.primitive_type,
					ob.surfaces.mesh_flags,
					material_indices
			);
			shadow_occluder_mesh = build_mesh(ob.surfaces.shadow_occluder);
		}
	}
	if (mesh.is_valid()) {
		const unsigned int surface_count = mesh->get_surface_count();
		for (unsigned int surface_index = 0; surface_index < surface_count; ++surface_index) {
			const unsigned int material_index = material_indices[surface_index];
			Ref<Material> material = _mesher->get_material_by_index(material_index);
			mesh->surface_set_material(surface_index, material);
		}
	}

	if (mesh.is_null() && block->has_mesh()) {
		// 该数据块不再有表面
#ifdef VOXEL_ENABLE_INSTANCER
		if (_instancer != nullptr) {
			_instancer->on_mesh_block_exit(ob.position, ob.lod);
		}
#endif
	}
	if (ob.surfaces.surfaces.size() > 0 && mesh.is_valid() && !block->has_mesh()) {
		// TODO 网格可能来自被编辑过的区域！
		// 我们需要知道特定的体素是否被编辑过，或者是否与生成器的结果不同
		// TODO 在 VoxelInstancer 中支持多表面
#ifdef VOXEL_ENABLE_INSTANCER
		if (_instancer != nullptr) {
			_instancer->on_mesh_block_enter(
					ob.position,
					ob.lod,
					ob.surfaces.surfaces[0].arrays,
					ob.surfaces.collision_surface.submesh_vertex_end,
					ob.surfaces.collision_surface.submesh_index_end
			);
		}
#endif
	}

#ifdef TOOLS_ENABLED
	const RenderingServerEnums::ShadowCastingSetting shadow_occluder_mode = _debug_draw_shadow_occluders
			? RenderingServerEnums::SHADOW_CASTING_SETTING_ON
			: RenderingServerEnums::SHADOW_CASTING_SETTING_SHADOWS_ONLY;
#endif

	block->set_mesh(
			mesh,
			get_gi_mode(),
			static_cast<RenderingServerEnums::ShadowCastingSetting>(get_shadow_casting()),
			get_render_layers_mask(),
			shadow_occluder_mesh
#ifdef TOOLS_ENABLED
			,
			shadow_occluder_mode
#endif
	);

	if (_material_override.is_valid()) {
		block->set_material_override(_material_override);
	}

	const bool gen_collisions = _generate_collisions && block->collision_viewers.get() > 0;
	if (gen_collisions) {
		Ref<Shape3D> collision_shape = make_collision_shape_from_mesher_output(ob.surfaces, **_mesher);

		bool debug_collisions = false;
		if (is_inside_tree()) {
			const SceneTree *scene_tree = get_tree();
#if DEBUG_ENABLED
			if (collision_shape.is_valid()) {
				const Color debug_color = voxel::godot::get_shape_3d_default_color(*scene_tree);
				voxel::godot::set_shape_3d_debug_color(**collision_shape, debug_color);
			}
#endif
			debug_collisions = scene_tree->is_debugging_collisions_hint();
		}

		block->set_collision_shape(collision_shape, debug_collisions, this, _collision_margin);

		block->set_collision_layer(_collision_layer);
		block->set_collision_mask(_collision_mask);
	}

	block->set_visible(block->mesh_viewers.get() > 0);
	block->set_collision_enabled(gen_collisions);
	block->set_parent_visible(is_visible());
	block->set_parent_transform(get_global_transform());
	// TODO 我们没有在任何地方设置 MESH_UP_TO_DATE，但似乎能正常工作？
	// 不能设置该状态，因为可能有多个更新正在进行。也许它需要重构。
	// block->set_mesh_state(VoxelMeshBlockVT::MESH_UP_TO_DATE);

	if (block->is_loaded == false) {
		block->is_loaded = true;
		emit_mesh_block_entered(ob.position);
	}
}

Ref<VoxelTool> VoxelTerrain::get_voxel_tool() {
	Ref<VoxelTool> vt = memnew(VoxelToolTerrain(this));
	const int used_channels_mask = get_used_channels_mask();
	// 自动选择第一个使用的通道
	for (int channel = 0; channel < VoxelBuffer::MAX_CHANNELS; ++channel) {
		if ((used_channels_mask & (1 << channel)) != 0) {
			vt->set_channel(VoxelBuffer::ChannelId(channel));
			break;
		}
	}
	return vt;
}

void VoxelTerrain::set_bounds(Box3i box) {
	Box3i bounds_in_voxels =
			box.clipped(Box3i::from_center_extents(Vector3i(), Vector3iUtil::create(constants::MAX_VOLUME_EXTENT)));

	const int smallest_dimension = get_data_block_size();
	bounds_in_voxels.size = math::max(bounds_in_voxels.size, Vector3iUtil::create(smallest_dimension));

	// 四舍五入到数据块大小
	bounds_in_voxels = bounds_in_voxels.snapped(get_data_block_size());

	_data->set_bounds(bounds_in_voxels);

	const unsigned int largest_dimension =
			static_cast<unsigned int>(math::max(math::max(box.size.x, box.size.y), box.size.z));
	if (largest_dimension > MAX_VIEW_DISTANCE_FOR_LARGE_VOLUME) {
		// 限制观察距离，确保在更改参数时不会意外耗尽内存
		if (_max_view_distance_voxels > MAX_VIEW_DISTANCE_FOR_LARGE_VOLUME) {
			_max_view_distance_voxels = math::min(_max_view_distance_voxels, MAX_VIEW_DISTANCE_FOR_LARGE_VOLUME);
			notify_property_list_changed();
		}
	}
	// TODO 编辑器 gizmo 边界

	update_configuration_warnings();
}

Box3i VoxelTerrain::get_bounds() const {
	return _data->get_bounds();
}

void VoxelTerrain::set_multiplayer_synchronizer(VoxelTerrainMultiplayerSynchronizer *synchronizer) {
	_multiplayer_synchronizer = synchronizer;
}

const VoxelTerrainMultiplayerSynchronizer *VoxelTerrain::get_multiplayer_synchronizer() const {
	return _multiplayer_synchronizer;
}

bool VoxelTerrain::is_area_meshed(const Box3i &box_in_voxels) const {
	// 这里假设即使没有网格，我们也存储网格数据块
	const Box3i mesh_box = box_in_voxels.downscaled(get_mesh_block_size());
	return mesh_box.all_cells_match([this](Vector3i bpos) {
		const VoxelMeshBlockVT *block = _mesh_map.get_block(bpos);
		return block != nullptr && block->is_loaded;
	});
}

#ifdef TOOLS_ENABLED

void VoxelTerrain::get_configuration_warnings(PackedStringArray &warnings) const {
	VoxelNode::get_configuration_warnings(warnings);

#ifdef VOXEL_ENABLE_GPU
	if (get_generator_use_gpu()) {
		Ref<VoxelGenerator> generator = get_generator();
		if (generator.is_valid() && !generator->supports_shaders()) {
			warnings.append(String("`use_gpu_generation` is enabled, but {0} does not support running on the GPU.")
									.format(varray(generator->get_class())));
		}
	}
#endif

	if (get_bounds().is_empty()) {
		warnings.append(String("Terrain bounds have an empty size."));
	}
}

#endif

void VoxelTerrain::on_format_changed() {
	_on_stream_params_changed();
}

// 调试区

void VoxelTerrain::debug_set_draw_enabled(bool enabled) {
#ifdef TOOLS_ENABLED
	_debug_draw_enabled = enabled;
	if (_debug_draw_enabled) {
		if (is_inside_tree()) {
			_debug_renderer.set_world(is_visible_in_tree() ? *get_world_3d() : nullptr);
		}
	} else {
		_debug_renderer.clear();
		// _debug_mesh_update_items.clear();
		// _debug_edit_items.clear();
	}
#endif
}

bool VoxelTerrain::debug_is_draw_enabled() const {
#ifdef TOOLS_ENABLED
	return _debug_draw_enabled;
#else
	return false;
#endif
}

void VoxelTerrain::debug_set_draw_flag(DebugDrawFlag flag_index, bool enabled) {
#ifdef TOOLS_ENABLED
	ERR_FAIL_INDEX(flag_index, DEBUG_DRAW_FLAGS_COUNT);
	if (enabled) {
		_debug_draw_flags |= (1 << flag_index);
	} else {
		_debug_draw_flags &= ~(1 << flag_index);
	}
#endif
}

bool VoxelTerrain::debug_get_draw_flag(DebugDrawFlag flag_index) const {
#ifdef TOOLS_ENABLED
	ERR_FAIL_INDEX_V(flag_index, DEBUG_DRAW_FLAGS_COUNT, false);
	return (_debug_draw_flags & (1 << flag_index)) != 0;
#else
	return false;
#endif
}

void VoxelTerrain::debug_set_draw_shadow_occluders(bool enable) {
#ifdef TOOLS_ENABLED
	if (enable == _debug_draw_shadow_occluders) {
		return;
	}
	_debug_draw_shadow_occluders = enable;
	const RenderingServerEnums::ShadowCastingSetting mode = enable
			? RenderingServerEnums::SHADOW_CASTING_SETTING_ON
			: RenderingServerEnums::SHADOW_CASTING_SETTING_SHADOWS_ONLY;
	_mesh_map.for_each_block([mode](VoxelMeshBlockVT &block) {
		if (block.shadow_occluder.is_valid()) {
			block.shadow_occluder.set_cast_shadows_setting(mode);
		}
	});
#endif
}

bool VoxelTerrain::debug_get_draw_shadow_occluders() const {
#ifdef TOOLS_ENABLED
	return _debug_draw_shadow_occluders;
#else
	return false;
#endif
}

#ifdef TOOLS_ENABLED

void VoxelTerrain::process_debug_draw() {
	VOXEL_PROFILE_SCOPE();

	voxel::godot::DebugRenderer &dr = _debug_renderer;
	dr.begin();

	const Transform3D parent_transform = get_global_transform();

	// 体积边界
	if (debug_get_draw_flag(DEBUG_DRAW_VOLUME_BOUNDS)) {
		const Box3i bounds_in_voxels = get_bounds();
		const float bounds_in_voxels_len = Vector3(bounds_in_voxels.size).length();

		if (bounds_in_voxels_len < 10000) {
			const Vector3 margin = Vector3(1, 1, 1) * bounds_in_voxels_len * 0.0025f;
			const Vector3 size = bounds_in_voxels.size;
			const Transform3D local_transform(
					Basis().scaled(size + margin * 2.f), Vector3(bounds_in_voxels.position) - margin
			);
			dr.draw_box(parent_transform * local_transform, Color(1, 1, 1));
		}
	}

	if (debug_get_draw_flag(DEBUG_DRAW_VISUAL_AND_COLLISION_BLOCKS)) {
		const int mesh_block_size = get_mesh_block_size();
		_mesh_map.for_each_block([&parent_transform, &dr, mesh_block_size](const VoxelMeshBlockVT &block) {
			Color8 color;
			const bool visual = block.is_visible();
			const bool collision = block.is_collision_enabled();
			if (visual && collision) {
				color = Color8(255, 255, 0, 255);
			} else if (visual) {
				color = Color8(0, 255, 0, 255);
			} else if (collision) {
				color = Color8(255, 0, 0, 255);
			} else {
				return;
			}
			const Vector3i voxel_pos = block.position * mesh_block_size;
			const Transform3D local_transform(
					Basis().scaled(Vector3(mesh_block_size, mesh_block_size, mesh_block_size)), voxel_pos
			);
			const Transform3D t = parent_transform * local_transform;
			dr.draw_box(t, color);
		});
	}

	if (debug_get_draw_flag(DEBUG_DRAW_VOXEL_METADATA)) {
		const int data_block_size = get_data_block_size();
		_data->for_each_block_at_lod_r(
				[&dr, parent_transform, data_block_size](const Vector3i &bpos, const VoxelDataBlock &block) {
					if (block.has_voxels()) {
						const VoxelBuffer &vb = block.get_voxels_const();
						const FlatMapMoveOnly<Vector3i, VoxelMetadata> &meta_map = vb.get_voxel_metadata();
						const Vector3i block_origin = bpos * data_block_size;

						for (auto it = meta_map.begin(); it != meta_map.end(); ++it) {
							const Vector3i rpos = it->key;
							const Transform3D local_transform(Basis(), to_vec3(block_origin + rpos));
							const Transform3D t = parent_transform * local_transform;
							dr.draw_box(t, Color8(255, 255, 0, 255));
						}
					}
				},
				0
		);
	}

	dr.end();
}

#endif

// 绑定区

Vector3i VoxelTerrain::_b_voxel_to_data_block(Vector3 pos) const {
	return _data->voxel_to_block(math::floor_to_int(pos));
}

Vector3i VoxelTerrain::_b_data_block_to_voxel(Vector3i pos) const {
	return _data->block_to_voxel(pos);
}

Ref<VoxelSaveCompletionTracker> VoxelTerrain::_b_save_modified_blocks() {
	std::shared_ptr<AsyncDependencyTracker> tracker = make_shared_instance<AsyncDependencyTracker>();
	save_all_modified_blocks(true, tracker);
	VOXEL_ASSERT_RETURN_V(tracker != nullptr, Ref<VoxelSaveCompletionTracker>());
	return VoxelSaveCompletionTracker::create(tracker);
}

// 显式请求保存被修改过的数据块
void VoxelTerrain::_b_save_block(Vector3i p_block_pos) {
	VoxelData::BlockToSave to_save;
	if (_data->consume_block_modifications(p_block_pos, to_save)) {
		_blocks_to_save.push_back(to_save);
	}
}

void VoxelTerrain::_b_set_bounds(AABB aabb) {
	ERR_FAIL_COND(!math::is_valid_size(aabb.size));
	set_bounds(Box3i(math::round_to_int(aabb.position), math::round_to_int(aabb.size)));
}

AABB VoxelTerrain::_b_get_bounds() const {
	const Box3i b = get_bounds();
	return AABB(b.position, b.size);
}

bool VoxelTerrain::_b_try_set_block_data(Vector3i position, Ref<godot::VoxelBuffer> voxel_data) {
	ERR_FAIL_COND_V(voxel_data.is_null(), false);
	std::shared_ptr<VoxelBuffer> buffer = voxel_data->get_buffer_shared();

#ifdef DEBUG_ENABLED
	// 不允许使用同一个体素缓冲区在两个不同位置调用此函数
	const StringName &key = VoxelStringNames::get_singleton()._voxel_debug_vt_position;
	if (voxel_data->has_meta(key)) {
		const Vector3i meta_pos = voxel_data->get_meta(key);
		ERR_FAIL_COND_V_MSG(
				meta_pos != position,
				false,
				String("Setting the same {0} at different positions is not supported")
						.format(varray(godot::VoxelBuffer::get_class_static()))
		);
	} else {
		voxel_data->set_meta(key, position);
	}
#endif

	return try_set_block_data(position, buffer);
}

PackedInt32Array VoxelTerrain::_b_get_viewer_network_peer_ids_in_area(Vector3i area_origin, Vector3i area_size) const {
	static thread_local StdVector<ViewerID> s_ids;
	StdVector<ViewerID> &viewer_ids = s_ids;
	viewer_ids.clear();
	get_viewers_in_area(viewer_ids, Box3i(area_origin, area_size));

	PackedInt32Array peer_ids;
	peer_ids.resize(viewer_ids.size());
	// 使用直接访问以获得更高性能
	int32_t *peer_ids_data = peer_ids.ptrw();
	VOXEL_ASSERT_RETURN_V(peer_ids_data != nullptr, peer_ids);
	for (size_t i = 0; i < viewer_ids.size(); ++i) {
		const int peer_id = VoxelEngine::get_singleton().get_viewer_network_peer_id(viewer_ids[i]);
		peer_ids_data[i] = peer_id;
	}

	return peer_ids;
}

bool VoxelTerrain::_b_is_area_meshed(AABB aabb) const {
	return is_area_meshed(Box3i(aabb.position, aabb.size));
}

void VoxelTerrain::_bind_methods() {
	using Self = VoxelTerrain;

	ClassDB::bind_method(D_METHOD("set_material_override", "material"), &Self::set_material_override);
	ClassDB::bind_method(D_METHOD("get_material_override"), &Self::get_material_override);

	ClassDB::bind_method(D_METHOD("set_max_view_distance", "distance_in_voxels"), &Self::set_max_view_distance);
	ClassDB::bind_method(D_METHOD("get_max_view_distance"), &Self::get_max_view_distance);

	ClassDB::bind_method(
			D_METHOD("set_block_enter_notification_enabled", "enabled"), &Self::set_block_enter_notification_enabled
	);
	ClassDB::bind_method(D_METHOD("is_block_enter_notification_enabled"), &Self::is_block_enter_notification_enabled);

	ClassDB::bind_method(
			D_METHOD("set_area_edit_notification_enabled", "enabled"), &Self::set_area_edit_notification_enabled
	);
	ClassDB::bind_method(D_METHOD("is_area_edit_notification_enabled"), &Self::is_area_edit_notification_enabled);

	ClassDB::bind_method(D_METHOD("get_generate_collisions"), &Self::get_generate_collisions);
	ClassDB::bind_method(D_METHOD("set_generate_collisions", "enabled"), &Self::set_generate_collisions);

	ClassDB::bind_method(D_METHOD("get_collision_layer"), &Self::get_collision_layer);
	ClassDB::bind_method(D_METHOD("set_collision_layer", "layer"), &Self::set_collision_layer);

	ClassDB::bind_method(D_METHOD("get_collision_mask"), &Self::get_collision_mask);
	ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &Self::set_collision_mask);

	ClassDB::bind_method(D_METHOD("get_collision_margin"), &Self::get_collision_margin);
	ClassDB::bind_method(D_METHOD("set_collision_margin", "margin"), &Self::set_collision_margin);

	ClassDB::bind_method(D_METHOD("voxel_to_data_block", "voxel_pos"), &Self::_b_voxel_to_data_block);
	ClassDB::bind_method(D_METHOD("data_block_to_voxel", "block_pos"), &Self::_b_data_block_to_voxel);

	ClassDB::bind_method(D_METHOD("get_data_block_size"), &Self::get_data_block_size);

	ClassDB::bind_method(D_METHOD("get_mesh_block_size"), &Self::get_mesh_block_size);
	ClassDB::bind_method(D_METHOD("set_mesh_block_size", "size"), &Self::set_mesh_block_size);

	ClassDB::bind_method(D_METHOD("get_statistics"), &Self::_b_get_statistics);

	ClassDB::bind_method(D_METHOD("save_modified_blocks"), &Self::_b_save_modified_blocks);
	ClassDB::bind_method(D_METHOD("save_block", "position"), &Self::_b_save_block);

	ClassDB::bind_method(D_METHOD("set_automatic_loading_enabled", "enable"), &Self::set_automatic_loading_enabled);
	ClassDB::bind_method(D_METHOD("is_automatic_loading_enabled"), &Self::is_automatic_loading_enabled);

#ifdef VOXEL_ENABLE_GPU
	ClassDB::bind_method(D_METHOD("set_generator_use_gpu", "enable"), &Self::set_generator_use_gpu);
	ClassDB::bind_method(D_METHOD("get_generator_use_gpu"), &Self::get_generator_use_gpu);
#endif

	// TODO 重命名 `_voxel_bounds`
	ClassDB::bind_method(D_METHOD("set_bounds", "bounds"), &Self::_b_set_bounds);
	ClassDB::bind_method(D_METHOD("get_bounds"), &Self::_b_get_bounds);

	ClassDB::bind_method(D_METHOD("try_set_block_data", "position", "voxels"), &Self::_b_try_set_block_data);

	ClassDB::bind_method(
			D_METHOD("get_viewer_network_peer_ids_in_area", "area_origin", "area_size"),
			&Self::_b_get_viewer_network_peer_ids_in_area
	);

	ClassDB::bind_method(D_METHOD("has_data_block", "block_position"), &Self::has_data_block);
	ClassDB::bind_method(D_METHOD("is_area_meshed", "area_in_voxels"), &Self::_b_is_area_meshed);

	ClassDB::bind_method(D_METHOD("debug_set_draw_enabled", "enabled"), &Self::debug_set_draw_enabled);
	ClassDB::bind_method(D_METHOD("debug_is_draw_enabled"), &Self::debug_is_draw_enabled);
	ClassDB::bind_method(D_METHOD("debug_set_draw_flag", "flag_index", "enabled"), &Self::debug_set_draw_flag);
	ClassDB::bind_method(D_METHOD("debug_get_draw_flag", "flag_index"), &Self::debug_get_draw_flag);

	ClassDB::bind_method(
			D_METHOD("debug_set_draw_shadow_occluders", "enabled"), &Self::debug_set_draw_shadow_occluders
	);
	ClassDB::bind_method(D_METHOD("debug_get_draw_shadow_occluders"), &Self::debug_get_draw_shadow_occluders);

	GDVIRTUAL_BIND(_on_data_block_entered, "info");
	GDVIRTUAL_BIND(_on_area_edited, "area_origin", "area_size");

	ADD_GROUP("Bounds", "");

	ADD_PROPERTY(PropertyInfo(Variant::AABB, "bounds"), "set_bounds", "get_bounds");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_view_distance"), "set_max_view_distance", "get_max_view_distance");

	ADD_GROUP("Collisions", "");

	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "generate_collisions"), "set_generate_collisions", "get_generate_collisions"
	);
	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_3D_PHYSICS),
			"set_collision_layer",
			"get_collision_layer"
	);
	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS),
			"set_collision_mask",
			"get_collision_mask"
	);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "collision_margin"), "set_collision_margin", "get_collision_margin");

	ADD_GROUP("Materials", "");

	ADD_PROPERTY(
			PropertyInfo(
					Variant::OBJECT,
					"material_override",
					PROPERTY_HINT_RESOURCE_TYPE,
					voxel::godot::MATERIAL_3D_PROPERTY_HINT_STRING
			),
			"set_material_override",
			"get_material_override"
	);

	ADD_GROUP("Networking", "");

	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "block_enter_notification_enabled"),
			"set_block_enter_notification_enabled",
			"is_block_enter_notification_enabled"
	);

	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "area_edit_notification_enabled"),
			"set_area_edit_notification_enabled",
			"is_area_edit_notification_enabled"
	);

	// 在多人在线设计中，当服务器负责发送数据块时，此值可能被设为 false
	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "automatic_loading_enabled"),
			"set_automatic_loading_enabled",
			"is_automatic_loading_enabled"
	);

	ADD_GROUP("Advanced", "");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "mesh_block_size"), "set_mesh_block_size", "get_mesh_block_size");
#ifdef VOXEL_ENABLE_GPU
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_gpu_generation"), "set_generator_use_gpu", "get_generator_use_gpu");
#endif

	ADD_GROUP("Debug", "debug_");

	// 调试绘制不是持久的

	BIND_ENUM_CONSTANT(DEBUG_DRAW_VOLUME_BOUNDS);
	BIND_ENUM_CONSTANT(DEBUG_DRAW_VISUAL_AND_COLLISION_BLOCKS);
	BIND_ENUM_CONSTANT(DEBUG_DRAW_VOXEL_METADATA);
	BIND_ENUM_CONSTANT(DEBUG_DRAW_FLAGS_COUNT);

	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "debug_draw_enabled", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR),
			"debug_set_draw_enabled",
			"debug_is_draw_enabled"
	);

#define ADD_DEBUG_DRAW_FLAG(m_name, m_flag)                                                                            \
	ADD_PROPERTYI(                                                                                                     \
			PropertyInfo(Variant::BOOL, m_name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR),                        \
			"debug_set_draw_flag",                                                                                     \
			"debug_get_draw_flag",                                                                                     \
			m_flag                                                                                                     \
	);

	ADD_DEBUG_DRAW_FLAG("debug_draw_volume_bounds", DEBUG_DRAW_VOLUME_BOUNDS);
	ADD_DEBUG_DRAW_FLAG("debug_draw_visual_and_collision_blocks", DEBUG_DRAW_VISUAL_AND_COLLISION_BLOCKS);
	ADD_DEBUG_DRAW_FLAG("debug_draw_voxel_metadata", DEBUG_DRAW_VOXEL_METADATA);

	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "debug_draw_shadow_occluders", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR),
			"debug_set_draw_shadow_occluders",
			"debug_get_draw_shadow_occluders"
	);

	// TODO 重新提供对数据块的访问，但需使用保证多线程安全的 API
	ADD_SIGNAL(MethodInfo("block_loaded", PropertyInfo(Variant::VECTOR3I, "position")));
	ADD_SIGNAL(MethodInfo("block_unloaded", PropertyInfo(Variant::VECTOR3I, "position")));

	ADD_SIGNAL(MethodInfo("mesh_block_entered", PropertyInfo(Variant::VECTOR3I, "position")));
	ADD_SIGNAL(MethodInfo("mesh_block_exited", PropertyInfo(Variant::VECTOR3I, "position")));
}

} // namespace voxel
