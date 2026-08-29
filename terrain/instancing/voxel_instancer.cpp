#include "../../constants/voxel_string_names.h"
#include "../../edition/voxel_tool.h"
#include "../../engine/buffered_task_scheduler.h"
#include "../../streams/save_block_data_task.h"
#include "../../util/containers/container_funcs.h"
#include "../../util/dstack.h"
#include "../../util/godot/classes/camera_3d.h"
#include "../../util/godot/classes/collision_shape_3d.h"
#include "../../util/godot/classes/engine.h"
#include "../../util/godot/classes/mesh_instance_3d.h"
#include "../../util/godot/classes/multimesh.h"
#include "../../util/godot/classes/node.h"
#include "../../util/godot/classes/ref_counted.h"
#include "../../util/godot/classes/resource_saver.h"
#include "../../util/godot/classes/time.h"
#include "../../util/godot/classes/viewport.h"
#include "../../util/godot/core/aabb.h"
#include "../../util/godot/core/array.h"
#include "../../util/godot/core/basis.h"
#include "../../util/math/conv.h"
#include "../../util/profiling.h"
#include "../../util/string/format.h"
#include "../fixed_lod/voxel_terrain.h"
#include "../variable_lod/voxel_lod_terrain.h"
#include "instancer_quick_reloading_cache.h"
#include "load_instance_block_task.h"
#include "voxel_instance_component.h"
#include "voxel_instance_generator.h"
#include "voxel_instance_library_multimesh_item.h"
#include "voxel_instance_library_scene_item.h"
#include "voxel_instancer_rigidbody.h"

#ifdef TOOLS_ENABLED
#include "../../editor/camera_cache.h"
#include "../../util/godot/core/packed_arrays.h"
#endif

// 仅用于调试目的，否则直接使用 RenderingServer
#include "../../util/godot/classes/multimesh_instance_3d.h"

#include <algorithm>

namespace voxel {

namespace {
StdVector<Transform3f> &get_tls_transform_cache() {
	static thread_local StdVector<Transform3f> tls_transform_cache;
	return tls_transform_cache;
}
} // namespace

VoxelInstancer::VoxelInstancer() {
	set_notify_transform(true);
	set_process_internal(true);
	_loading_results = make_shared_instance<InstancerTaskOutputQueue>();
	fill(_mesh_lod_distances, 0.f);
}

VoxelInstancer::~VoxelInstancer() {
	// 销毁所有内容
	// 注意：我们不通过节点销毁实例，假定它们已被分离

	if (_library.is_valid()) {
		_library->remove_listener(this);
	}
}

void VoxelInstancer::clear_blocks() {
	VOXEL_PROFILE_SCOPE();
	// 销毁数据块，保留已配置的图层
	for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
		Block &block = **it;
		for (unsigned int i = 0; i < block.bodies.size(); ++i) {
			VoxelInstancerRigidBody *body = block.bodies[i];
			body->detach_and_destroy();
		}
		for (unsigned int i = 0; i < block.scene_instances.size(); ++i) {
			SceneInstance instance = block.scene_instances[i];
			ERR_CONTINUE(instance.component == nullptr);
			instance.component->detach();
			ERR_CONTINUE(instance.root == nullptr);
			instance.root->queue_free();
		}
	}
	_blocks.clear();
	for (auto it = _layers.begin(); it != _layers.end(); ++it) {
		Layer &layer = it->second;
		layer.blocks.clear();
	}
	for (unsigned int lod_index = 0; lod_index < _lods.size(); ++lod_index) {
		Lod &lod = _lods[lod_index];
		lod.modified_blocks.clear();
	}
}

void VoxelInstancer::clear_blocks_in_layer(int layer_id) {
	// 不是最优方案，但暂时应该能用
	for (size_t i = 0; i < _blocks.size(); ++i) {
		Block &block = *_blocks[i];
		if (block.layer_id == layer_id) {
			remove_block(i, false);
			// remove_block 执行交换删除，因此我们必须重新迭代同一槽位
			--i;
		}
	}
}

void VoxelInstancer::clear_layers() {
	clear_blocks();
	for (unsigned int lod_index = 0; lod_index < _lods.size(); ++lod_index) {
		Lod &lod = _lods[lod_index];
		lod.layers.clear();
		lod.modified_blocks.clear();
	}
	_layers.clear();
}

void VoxelInstancer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_WORLD:
			set_world(*get_world_3d());
			update_visibility();
#ifdef TOOLS_ENABLED
			_debug_renderer.set_world(get_world_3d().ptr());
#endif
			break;

		case NOTIFICATION_EXIT_WORLD:
			set_world(nullptr);
#ifdef TOOLS_ENABLED
			_debug_renderer.set_world(nullptr);
#endif
			break;

		case NOTIFICATION_PARENTED: {
			VoxelLodTerrain *vlt = Object::cast_to<VoxelLodTerrain>(get_parent());
			if (vlt != nullptr) {
				_parent = vlt;
				_parent_data_block_size_po2 = vlt->get_data_block_size_pow2();
				_parent_mesh_block_size_po2 = vlt->get_mesh_block_size_pow2();
				update_mesh_lod_distances_from_parent();
				vlt->set_instancer(this);
			} else {
				VoxelTerrain *vt = Object::cast_to<VoxelTerrain>(get_parent());
				if (vt != nullptr) {
					_parent = vt;
					_parent_data_block_size_po2 = vt->get_data_block_size_pow2();
					_parent_mesh_block_size_po2 = vt->get_mesh_block_size_pow2();
					update_mesh_lod_distances_from_parent();
					vt->set_instancer(this);
				}
			}
			// TODO 可能需要重新加载所有实例？不确定是否值得实现这种用例
		} break;

		case NOTIFICATION_UNPARENTED:
			clear_blocks();
			if (_parent != nullptr) {
				VoxelLodTerrain *vlt = Object::cast_to<VoxelLodTerrain>(_parent);
				if (vlt != nullptr) {
					vlt->set_instancer(nullptr);
				} else {
					VoxelTerrain *vt = Object::cast_to<VoxelTerrain>(get_parent());
					if (vt != nullptr) {
						vt->set_instancer(nullptr);
					}
				}
				_parent = nullptr;
			}
			break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			VOXEL_PROFILE_SCOPE_NAMED("VoxelInstancer::NOTIFICATION_TRANSFORM_CHANGED");

			if (!is_inside_tree() || _parent == nullptr) {
				// 变换和其他属性可能在进入场景树之前由场景加载器设置，
				// 此时我们尚未进入场景树
				return;
			}

			const Transform3D parent_transform = get_global_transform();
			const int base_block_size_po2 = _parent_mesh_block_size_po2;
			// print_line(String("IP: {0}").format(varray(parent_transform.origin)));

			for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
				Block &block = **it;
				if (!block.multimesh_instance.is_valid()) {
					// 该数据块以空数据块形式存在（若它不存在，则会被生成）
					continue;
				}
				const int block_size_po2 = base_block_size_po2 + block.lod_index;
				const Vector3 block_local_pos(block.grid_position << block_size_po2);
				// 局部数据块变换从不包含旋转或缩放，因此我们可以走捷径
				const Transform3D block_transform(parent_transform.basis, parent_transform.xform(block_local_pos));
				block.multimesh_instance.set_transform(block_transform);
			}
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED:
			update_visibility();
#ifdef TOOLS_ENABLED
			if (_gizmos_enabled) {
				_debug_renderer.set_world(is_visible_in_tree() ? *get_world_3d() : nullptr);
			}
#endif
			break;

		case NOTIFICATION_INTERNAL_PROCESS:
			process();
			break;
	}
}

void VoxelInstancer::process() {
	VOXEL_PROFILE_SCOPE();

	process_task_results();

	if (_parent != nullptr) {
		if (_library.is_valid()) {
			if (_mesh_lod_distances[0] > 0.f) {
				process_mesh_lods();
			}
			process_collision_distances();
		}

#ifdef TOOLS_ENABLED
		if (_gizmos_enabled && is_visible_in_tree()) {
			process_gizmos();
		}
#endif
	}

	process_fading();
}

void VoxelInstancer::process_task_results() {
	VOXEL_PROFILE_SCOPE();
	static thread_local StdVector<InstanceLoadingTaskOutput> tls_results;
	StdVector<InstanceLoadingTaskOutput> &results = tls_results;
#ifdef DEBUG_ENABLED
	if (results.size()) {
		VOXEL_PRINT_ERROR("Results were not cleaned up?");
	}
#endif
	{
		MutexLock mlock(_loading_results->mutex);
		// 将结果复制到临时缓冲区
		StdVector<InstanceLoadingTaskOutput> &src = _loading_results->results;
		results.resize(src.size());
		for (unsigned int i = 0; i < src.size(); ++i) {
			results[i] = std::move(src[i]);
		}
		src.clear();
	}

	if (results.size() == 0) {
		return;
	}

	Ref<World3D> maybe_world = get_world_3d();
	VOXEL_ASSERT_RETURN(maybe_world.is_valid());
	World3D &world = **maybe_world;

	const Transform3D parent_transform = get_global_transform();

	const int data_block_size_base = (1 << _parent_mesh_block_size_po2);
	const int mesh_block_size_base = (1 << _parent_mesh_block_size_po2);
	const int render_to_data_factor = mesh_block_size_base / data_block_size_base;

	for (InstanceLoadingTaskOutput &output : results) {
		auto layer_it = _layers.find(output.layer_id);
		if (layer_it == _layers.end()) {
			// 图层是否已被移除？
			VOXEL_PRINT_VERBOSE(
					format("Processing async instance generator results, but the layer isn't present ({}).",
						   static_cast<int>(output.layer_id))
			);
			continue;
		}
		Layer &layer = layer_it->second;

		const VoxelInstanceLibraryItem *item = _library->get_item(output.layer_id);
		VOXEL_ASSERT_CONTINUE_MSG(item != nullptr, "Item removed from library while it was loading?");

		auto block_it = layer.blocks.find(output.render_block_position);
		if (block_it == layer.blocks.end()) {
			// 生成过程运行时数据块是否已被移除？
			VOXEL_PRINT_VERBOSE("Processing async instance generator results, but the block was removed.");
			continue;
		}

		if (output.edited_mask != 0) {
			Lod &lod = _lods[layer.lod_index];
			const Vector3i minp = output.render_block_position * render_to_data_factor;
			const Vector3i maxp = minp + Vector3iUtil::create(render_to_data_factor);
			Vector3i bpos;
			unsigned int i = 0;
			for (bpos.z = minp.z; bpos.z < maxp.z; ++bpos.z) {
				for (bpos.y = minp.y; bpos.y < maxp.y; ++bpos.y) {
					for (bpos.x = minp.x; bpos.x < maxp.x; ++bpos.x) {
						if ((output.edited_mask & (1 << i)) != 0) {
							lod.edited_data_blocks.insert(bpos);
						}
						++i;
					}
				}
			}
		}

		const int mesh_block_size = mesh_block_size_base << layer.lod_index;
		const Transform3D block_local_transform = Transform3D(Basis(), output.render_block_position * mesh_block_size);
		const Transform3D block_global_transform = parent_transform * block_local_transform;

		update_block_from_transforms(
				block_it->second,
				to_span_const(output.transforms),
				output.render_block_position,
				layer,
				*item,
				output.layer_id,
				world,
				block_global_transform,
				block_local_transform.origin
		);

		if (_fading_enabled) {
			const VoxelInstanceLibraryMultiMeshItem *mm_item =
					Object::cast_to<const VoxelInstanceLibraryMultiMeshItem>(item);

			if (mm_item != nullptr) {
				FadingInBlock fb;
				fb.grid_position = output.render_block_position;
				fb.layer_id = output.layer_id;
				_fading_in_blocks.push_back(fb);
			}
		}
	}

	results.clear();
}

#ifdef TOOLS_ENABLED

void VoxelInstancer::process_gizmos() {
	VOXEL_PROFILE_SCOPE();

	using namespace voxel::godot;

	struct L {
		static inline void draw_box(
				DebugRenderer &dr,
				const Transform3D parent_transform,
				Vector3i bpos,
				unsigned int lod_index,
				unsigned int base_block_size_po2,
				Color8 color
		) {
			const int block_size_po2 = base_block_size_po2 + lod_index;
			const int block_size = 1 << block_size_po2;
			const Vector3 block_local_pos(bpos << block_size_po2);
			const Transform3D box_transform(
					parent_transform.basis * (Basis().scaled(Vector3(block_size, block_size, block_size))),
					parent_transform.xform(block_local_pos)
			);
			dr.draw_box(box_transform, color);
		}
	};

	ERR_FAIL_COND(_parent == nullptr);
	const Transform3D parent_transform = get_global_transform();
	const int base_mesh_block_size_po2 = _parent_mesh_block_size_po2;
	const int base_data_block_size_po2 = _parent_data_block_size_po2;

	_debug_renderer.begin();

	if (debug_get_draw_flag(DEBUG_DRAW_ALL_BLOCKS)) {
		for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
			const Block &block = **it;

			Color8 color(0, 255, 0, 255);
			if (block.multimesh_instance.is_valid()) {
				if (block.multimesh_instance.get_multimesh().is_null()) {
					// 已分配但没有 multimesh（啥？）
					color = Color8(128, 0, 0, 255);
				} else if (get_visible_instance_count(**block.multimesh_instance.get_multimesh()) == 0) {
					// 已分配但 multimesh 为空
					color = Color8(255, 64, 0, 255);
				}
			} else if (block.scene_instances.size() == 0) {
				// 仅绘制已设置好的数据块
				continue;
			}

			L::draw_box(
					_debug_renderer,
					parent_transform,
					block.grid_position,
					block.lod_index,
					base_mesh_block_size_po2,
					color
			);
		}
	}

	if (debug_get_draw_flag(DEBUG_DRAW_EDITED_BLOCKS)) {
		for (unsigned int lod_index = 0; lod_index < _lods.size(); ++lod_index) {
			const Lod &lod = _lods[lod_index];

			const Color8 edited_color(0, 255, 0, 255);
			const Color8 unsaved_color(255, 255, 0, 255);

			for (auto it = lod.edited_data_blocks.begin(); it != lod.edited_data_blocks.end(); ++it) {
				L::draw_box(_debug_renderer, parent_transform, *it, lod_index, base_data_block_size_po2, edited_color);
			}

			for (auto it = lod.modified_blocks.begin(); it != lod.modified_blocks.end(); ++it) {
				L::draw_box(_debug_renderer, parent_transform, *it, lod_index, base_data_block_size_po2, unsaved_color);
			}
		}
	}

	_debug_renderer.end();
}

#endif

VoxelInstancer::Layer &VoxelInstancer::get_layer(int id) {
	auto it = _layers.find(id);
	VOXEL_ASSERT(it != _layers.end());
	return it->second;
}

const VoxelInstancer::Layer &VoxelInstancer::get_layer_const(int id) const {
	auto it = _layers.find(id);
	VOXEL_ASSERT(it != _layers.end());
	return it->second;
}

namespace {
Vector3 get_global_camera_position(const Node &node) {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return voxel::godot::get_3d_editor_camera_position();
	}
#endif
	const Viewport *viewport = node.get_viewport();
	VOXEL_ASSERT_RETURN_V(viewport != nullptr, Vector3());
	const Camera3D *camera = viewport->get_camera_3d();
	if (camera == nullptr) {
		return Vector3();
	}
	return camera->get_global_position();
}

} // namespace

void VoxelInstancer::update_mesh_from_mesh_lod(
		Block &block,
		const VoxelInstanceLibraryMultiMeshItem::Settings &settings,
		bool hide_beyond_max_lod,
		bool instancer_is_visible
) {
	if (hide_beyond_max_lod && block.current_mesh_lod == settings.mesh_lod_count) {
		// Godot 不喜欢空网格，因此我们必须实现不同的代码路径

		// 若此数据块中当前没有实例，则可能无效
		if (block.multimesh_instance.is_valid()) {
			block.multimesh_instance.set_visible(false);
		}

	} else {
		Ref<MultiMesh> multimesh = block.multimesh_instance.get_multimesh();
		if (multimesh.is_valid()) {
			block.multimesh_instance.set_visible(instancer_is_visible);
			VOXEL_PROFILE_SCOPE();
			multimesh->set_mesh(settings.mesh_lods[block.current_mesh_lod]);
		}
	}
}

void VoxelInstancer::update_mesh_lod_distances_from_parent() {
	VOXEL_ASSERT_RETURN(_parent != nullptr);

	VoxelLodTerrain *vlt = Object::cast_to<VoxelLodTerrain>(_parent);
	if (vlt != nullptr) {
		vlt->get_lod_distances(to_span(_mesh_lod_distances));
		return;
	}

	VoxelTerrain *vt = Object::cast_to<VoxelTerrain>(_parent);
	if (vt != nullptr) {
		_mesh_lod_distances[0] = vt->get_max_view_distance();
	}
}

void VoxelInstancer::process_mesh_lods() {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_library.is_null());

	// 注意，这种 LOD 必须仅是视觉上的。它仅支持一个相机。

	// 获取观察者位置
	const Transform3D gtrans = get_global_transform();
	const Vector3 cam_pos_global = get_global_camera_position(*this);
	const Vector3 cam_pos_local = gtrans.affine_inverse().xform(cam_pos_global);

	ERR_FAIL_COND(_parent == nullptr);
	const unsigned int block_size = 1 << _parent_mesh_block_size_po2;

	const float hysteresis = 1.05;

	const uint64_t time_up_time = Time::get_singleton()->get_ticks_usec() + _mesh_lod_update_budget_microseconds;

	const bool instancer_is_visible = is_visible_in_tree();

	// const unsigned int initial_mesh_lod_time_sliced_block_index = _mesh_lod_time_sliced_block_index;

	while (_mesh_lod_time_sliced_block_index < _blocks.size()) {
		// 迭代一部分数据块，然后检查一次时间预算
		const unsigned int desired_portion_size = 64;
		const unsigned int portion_end = math::min(
				_mesh_lod_time_sliced_block_index + desired_portion_size, static_cast<unsigned int>(_blocks.size())
		);
		Span<UniquePtr<Block>> blocks_portion = to_span_from_position_and_size(
				_blocks, _mesh_lod_time_sliced_block_index, portion_end - _mesh_lod_time_sliced_block_index
		);
		_mesh_lod_time_sliced_block_index = portion_end;

		for (UniquePtr<Block> &block_ptr : blocks_portion) {
			Block &block = *block_ptr;
			// 空数据块提前退出（我们仅对 multimesh 执行此操作，因此无需检查其它内容）
			if (!block.multimesh_instance.is_valid()) {
				continue;
			}

			const VoxelInstanceLibraryItem *item_base = _library->get_item_const(block.layer_id);
			ERR_CONTINUE(item_base == nullptr);
			// TODO 优化：如果仅迭代相同的项类型，就不需要这种转换了
			const VoxelInstanceLibraryMultiMeshItem *item =
					Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(item_base);
			if (item == nullptr) {
				// 不是 multimesh 项
				continue;
			}
			const VoxelInstanceLibraryMultiMeshItem::Settings &settings = item->get_multimesh_settings();
			const bool hide_beyond_max_lod = item->get_hide_beyond_max_lod();
			const unsigned int extended_mesh_lod_count = settings.mesh_lod_count + (hide_beyond_max_lod ? 1 : 0);
			// 注意，“超出最大 LOD 时隐藏”计为多出一个网格被隐藏的 LOD。因此一个项可以只有
			// 一个网格设置，却仍被视为具有 LOD
			if (extended_mesh_lod_count <= 1) {
				// 此数据块没有 LOD
				// TODO 优化：如果仅迭代定义了 LOD 的项类型，就不需要这个条件判断了
				continue;
			}

			const int lod_index = item->get_lod_index();

			Span<const float> distance_ratios = item->get_mesh_lod_distance_ratios();
			const float max_distance = _mesh_lod_distances[lod_index];

			// #ifdef DEBUG_ENABLED
			// 		ERR_FAIL_COND(mesh_lod_count < VoxelInstanceLibraryMultiMeshItem::MAX_MESH_LODS);
			// #endif

			// const Lod &lod = _lods[lod_index];

			const int lod_block_size = block_size << lod_index;
			const int hs = lod_block_size >> 1;
			const Vector3 block_center_local(block.grid_position * lod_block_size + Vector3i(hs, hs, hs));
			const float distance_squared = cam_pos_local.distance_squared_to(block_center_local);

			// 计算当前网格 LOD 索引（注意，block.current_mesh_lod 可能因最终的配置更改而完全越界，
			// 甚至可以作为强制更新的手段。这里会将其带回有效范围）
			unsigned int current_mesh_lod = block.current_mesh_lod;
			while (current_mesh_lod + 1 < extended_mesh_lod_count &&
				   distance_squared > math::squared(
											  distance_ratios[current_mesh_lod] *
											  max_distance
											  // 退出距离略高，因此在接近阈值时更不容易振荡
											  // 切换
											  * hysteresis
									  )) {
				// 降低细节
				++current_mesh_lod;
			}
			while (current_mesh_lod > 0 &&
				   (distance_squared < math::squared(distance_ratios[current_mesh_lod - 1] * max_distance)
					// 若数量设置更低，则允许网格 LOD 索引下降
					|| current_mesh_lod >= extended_mesh_lod_count)) {
				// 提高细节
				--current_mesh_lod;
			}

			// 若发生变化则应用
			if (block.current_mesh_lod != current_mesh_lod) {
				block.current_mesh_lod = current_mesh_lod;
				update_mesh_from_mesh_lod(block, settings, hide_beyond_max_lod, instancer_is_visible);
			}
		}

		if (Time::get_singleton()->get_ticks_usec() > time_up_time) {
			break;
		}
	}

	// const int64_t updated_blocks_count = _mesh_lod_time_sliced_block_index -
	// initial_mesh_lod_time_sliced_block_index; const float updated_blocks_ratio = _blocks.size() != 0 ?
	// updated_blocks_count / float(_blocks.size()) : 0; VOXEL_PROFILE_PLOT("Updated Instancer Blocks Mesh LOD",
	// updated_blocks_ratio);

	// 目前每帧都重新启动更新
	if (_mesh_lod_time_sliced_block_index >= _blocks.size()) {
		_mesh_lod_time_sliced_block_index = 0;
	}
}

void VoxelInstancer::process_collision_distances() {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(_library.is_valid());

	// Godot 的物理引擎（包括 Jolt）在用户希望为可能出现数万次的项添加碰撞体时，
	// 会以神秘的方式崩溃。
	// 用户不想降低实例生成的间距，而是只对碰撞体这样做，这似乎就能解决问题。

	// 注意，目前这是一个仅限客户端的过程。在服务器权威场景中，必须为每个 VoxelViewer
	// 执行此操作，这会更加昂贵。

	// 获取观察者位置
	const Transform3D gtrans = get_global_transform();
	const Vector3 cam_pos_global = get_global_camera_position(*this);
	const Vector3 cam_pos_local = gtrans.affine_inverse().xform(cam_pos_global);

	ERR_FAIL_COND(_parent == nullptr);
	const unsigned int block_size = 1 << _parent_mesh_block_size_po2;

	const float hysteresis = 1.05;

	const uint64_t time_up_time =
			Time::get_singleton()->get_ticks_usec() + _collision_distance_update_budget_microseconds;

	// TODO 临时分配器的候选
	StdVector<Transform3f> transforms;

	while (_collision_distance_time_sliced_block_index < _blocks.size()) {
		// 迭代一部分数据块，然后检查一次时间预算
		const unsigned int desired_portion_size = 64;

		const unsigned int portion_begin = _collision_distance_time_sliced_block_index;
		const unsigned int portion_end =
				math::min(portion_begin + desired_portion_size, static_cast<unsigned int>(_blocks.size()));

		Span<UniquePtr<Block>> blocks_portion =
				to_span_from_position_and_size(_blocks, portion_begin, portion_end - portion_begin);

		_collision_distance_time_sliced_block_index = portion_end;

		for (unsigned int rel_block_index = 0; rel_block_index < blocks_portion.size(); ++rel_block_index) {
			UniquePtr<Block> &block_ptr = blocks_portion[rel_block_index];
			Block &block = *block_ptr;
			// 空数据块提前退出（我们仅对 multimesh 执行此操作，因此无需检查其它内容）
			if (!block.multimesh_instance.is_valid()) {
				continue;
			}

			const VoxelInstanceLibraryItem *item_base = _library->get_item_const(block.layer_id);
			VOXEL_ASSERT_CONTINUE(item_base != nullptr);
			// TODO 优化：如果仅迭代相同的项类型，就不需要这种转换了
			const VoxelInstanceLibraryMultiMeshItem *item =
					Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(item_base);
			if (item == nullptr) {
				// 不是 multimesh 项
				continue;
			}
			const float collision_distance = item->get_collision_distance();
			if (collision_distance <= 0.f) {
				// 已禁用，该项在所有距离都必须有碰撞体
				continue;
			}

			const int lod_block_size = block_size << block.lod_index;
			const Vector3i block_origin = block.grid_position * lod_block_size;

			const float distance_squared = voxel::distance_squared(
					AABB(Vector3(block_origin), voxel::godot::Vector3Utility::splat(lod_block_size)), cam_pos_local
			);

			if (block.distance_colliders_active) {
				// 退出距离略高一些，以减少在阈值附近频繁振荡的机会
				if (distance_squared > math::squared(collision_distance * hysteresis)) {
					destroy_multimesh_block_colliders(block);
					block.distance_colliders_active = false;
				}
			} else {
				if (distance_squared < math::squared(collision_distance)) {
					const unsigned int block_index = portion_begin + rel_block_index;

					get_instance_transforms_local(block, transforms);

					update_multimesh_block_colliders(
							block, block_index, item->get_multimesh_settings(), to_span(transforms), block_origin
					);
					block.distance_colliders_active = true;
				}
			}
		}

		if (Time::get_singleton()->get_ticks_usec() > time_up_time) {
			break;
		}
	}

	// 目前每帧都重新启动更新
	if (_collision_distance_time_sliced_block_index >= _blocks.size()) {
		_collision_distance_time_sliced_block_index = 0;
	}
}

void VoxelInstancer::process_fading() {
	VOXEL_PROFILE_SCOPE();

	const float delta_time = get_process_delta_time();
	const float fading_delta = delta_time / math::max(_fading_duration, 0.0001f);

	const StringName &shader_param_name = VoxelStringNames::get_singleton().u_lod_fade;

	{
		unsigned int i = _fading_in_blocks.size();
		while (i > 0) {
			--i;
			FadingInBlock &fb = _fading_in_blocks[i];
			fb.progress = math::min(fb.progress + fading_delta, 1.f);

			auto layer_it = _layers.find(fb.layer_id);
			if (layer_it != _layers.end()) {
				Layer &layer = layer_it->second;

				auto block_index_it = layer.blocks.find(fb.grid_position);
				if (block_index_it != layer.blocks.end()) {
					const unsigned int block_index = block_index_it->second;

					const UniquePtr<Block> &block_ptr = _blocks[block_index];
#ifdef DEV_ENABLED
					VOXEL_ASSERT(block_ptr != nullptr);
#endif
					Block &block = *block_ptr.get();
					if (block.multimesh_instance.is_valid()) {
						const Vector2 v(fb.progress, 1.f);
						block.multimesh_instance.set_shader_instance_parameter(shader_param_name, v);
					}
				}
			}

			if (fb.progress >= 1.f) {
				unordered_remove(_fading_in_blocks, i);
			}
		}
	}

	{
		unsigned int i = _fading_out_blocks.size();
		while (i > 0) {
			--i;
			FadingOutBlock &fb = _fading_out_blocks[i];
			fb.progress = math::min(fb.progress + fading_delta, 1.f);
			const Vector2 v(fb.progress, 0.f);
#ifdef DEV_ENABLED
			VOXEL_ASSERT(fb.multimesh_instance.is_valid());
#endif
			fb.multimesh_instance.set_shader_instance_parameter(shader_param_name, v);

			if (fb.progress >= 1.f) {
				// fb.multimesh_instance.destroy();
				// unordered_remove(_fading_out_blocks, i);
				_fading_out_blocks[i] = std::move(_fading_out_blocks.back());
				_fading_out_blocks.pop_back();
			}
		}
	}
}

// 我们需要自行处理，因为对于 multimesh 我们不使用节点
void VoxelInstancer::update_visibility() {
	if (!is_inside_tree()) {
		return;
	}
	const bool instancer_is_visible = is_visible_in_tree();

	for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
		Block &block = **it;

		if (block.multimesh_instance.is_valid()) {
			bool visible_with_lod = true;
			{
				const VoxelInstanceLibraryItem *item_base = _library->get_item_const(block.layer_id);
				ERR_CONTINUE(item_base == nullptr);
				// TODO 优化：如果仅迭代相同的项类型，就不需要这种转换了
				const VoxelInstanceLibraryMultiMeshItem *item =
						Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(item_base);
				if (item != nullptr) {
					const VoxelInstanceLibraryMultiMeshItem::Settings &settings = item->get_multimesh_settings();
					const bool hide_beyond_max_lod = item->get_hide_beyond_max_lod();
					if (hide_beyond_max_lod) {
						visible_with_lod = block.current_mesh_lod < settings.mesh_lod_count;
					}
				}
			}

			block.multimesh_instance.set_visible(instancer_is_visible && visible_with_lod);
		}
	}
}

void VoxelInstancer::set_world(World3D *world) {
	for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
		Block &block = **it;
		if (block.multimesh_instance.is_valid()) {
			block.multimesh_instance.set_world(world);
		}
	}
}

void VoxelInstancer::set_up_mode(UpMode mode) {
	ERR_FAIL_COND(mode < 0 || mode >= UP_MODE_COUNT);
	if (_up_mode == mode) {
		return;
	}
	_up_mode = mode;
	if (_parent != nullptr && is_inside_tree()) {
		for (auto it = _layers.begin(); it != _layers.end(); ++it) {
			regenerate_layer(it->first, false);
		}
	}
}

VoxelInstancer::UpMode VoxelInstancer::get_up_mode() const {
	return _up_mode;
}

void VoxelInstancer::set_library(Ref<VoxelInstanceLibrary> library) {
	if (library == _library) {
		return;
	}

	if (_library.is_valid()) {
		_library->remove_listener(this);
	}

	_library = library;

	clear_layers();

	if (_library.is_valid()) {
		_library->for_each_item([this](int id, const VoxelInstanceLibraryItem &item) {
			add_layer(id, item.get_lod_index());
			if (_parent != nullptr && is_inside_tree()) {
				regenerate_layer(id, true);
			}
		});

		_library->add_listener(this);
	}

	update_configuration_warnings();
}

Ref<VoxelInstanceLibrary> VoxelInstancer::get_library() const {
	return _library;
}

int VoxelInstancer::get_mesh_lod_update_budget_microseconds() const {
	return _mesh_lod_update_budget_microseconds;
}

void VoxelInstancer::set_mesh_lod_update_budget_microseconds(const int p_micros) {
	_mesh_lod_update_budget_microseconds = math::max(p_micros, 0);
}

int VoxelInstancer::get_collision_update_budget_microseconds() const {
	return _collision_distance_update_budget_microseconds;
}

void VoxelInstancer::set_collision_update_budget_microseconds(const int p_micros) {
	_collision_distance_update_budget_microseconds = math::max(p_micros, 0);
}

void VoxelInstancer::set_fading_enabled(const bool enabled) {
	_fading_enabled = enabled;
}

bool VoxelInstancer::get_fading_enabled() const {
	return _fading_enabled;
}

void VoxelInstancer::set_fading_duration(const float duration) {
	_fading_duration = math::clamp(duration, 0.f, 5.f);
}

float VoxelInstancer::get_fading_duration() const {
	return _fading_duration;
}

void VoxelInstancer::regenerate_layer(uint16_t layer_id, bool regenerate_blocks) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_parent == nullptr);

	Ref<World3D> world_ref = get_world_3d();
	ERR_FAIL_COND(world_ref.is_null());
	World3D &world = **world_ref;

	Layer &layer = get_layer(layer_id);

	Ref<VoxelInstanceLibraryItem> item = _library->get_item(layer_id);
	ERR_FAIL_COND(item.is_null());
	if (item->get_generator().is_null()) {
		return;
	}

	const Transform3D parent_transform = get_global_transform();

	const VoxelLodTerrain *parent_vlt = Object::cast_to<VoxelLodTerrain>(_parent);
	const VoxelTerrain *parent_vt = Object::cast_to<VoxelTerrain>(_parent);

	if (regenerate_blocks) {
		// 创建数据块
		StdVector<Vector3i> positions;

		if (parent_vlt != nullptr) {
			parent_vlt->get_meshed_block_positions_at_lod(layer.lod_index, positions);

		} else if (parent_vt != nullptr) {
			// 仅支持 LOD 0
			if (layer.lod_index == 0) {
				parent_vt->get_meshed_block_positions(positions);
			}
		}

		for (unsigned int i = 0; i < positions.size(); ++i) {
			const Vector3i pos = positions[i];

			auto it = layer.blocks.find(pos);
			if (it != layer.blocks.end()) {
				continue;
			}

			create_block(layer, layer_id, pos, false);
		}
	}

	const int render_to_data_factor = 1 << (_parent_mesh_block_size_po2 - _parent_mesh_block_size_po2);
	ERR_FAIL_COND(render_to_data_factor <= 0 || render_to_data_factor > 2);

	struct L {
		// 不返回 bool，以便在移位运算中使用而不产生编译器警告。
		// 也可将其视为 bool 使用。
		static inline uint8_t has_edited_block(const Lod &lod, Vector3i pos) {
			return lod.edited_data_blocks.find(pos) != lod.edited_data_blocks.end();
		}

		static inline void extract_octant_transforms(
				const Block &render_block,
				StdVector<Transform3f> &dst,
				uint8_t octant_mask,
				int render_block_size
		) {
			if (!render_block.multimesh_instance.is_valid()) {
				return;
			}
			Ref<MultiMesh> multimesh = render_block.multimesh_instance.get_multimesh();
			ERR_FAIL_COND(multimesh.is_null());
			const int instance_count = voxel::godot::get_visible_instance_count(**multimesh);
			const float h = render_block_size / 2;
			for (int i = 0; i < instance_count; ++i) {
				// TODO 优化：首次执行非常慢，之后仍有开销。
				const Transform3D t = multimesh->get_instance_transform(i);
				const uint8_t octant_index = VoxelInstanceGenerator::get_octant_index(to_vec3f(t.origin), h);
				if ((octant_mask & (1 << octant_index)) != 0) {
					dst.push_back(to_transform3f(t));
				}
			}
		}
	};

	Ref<VoxelGenerator> voxel_generator = _parent->get_generator();

	// 更新现有数据块
	for (size_t block_index = 0; block_index < _blocks.size(); ++block_index) {
		Block &block = *_blocks[block_index];
		if (block.layer_id != layer_id) {
			continue;
		}
		const int lod_index = block.lod_index;
		const Lod &lod = _lods[lod_index];

		// 每个位表示“是否应生成此卦限”。若为 0，表示它已被编辑且不应更改
		uint8_t octant_mask = 0xff;
		if (render_to_data_factor == 1) {
			if (L::has_edited_block(lod, block.grid_position)) {
				// 已被编辑，此数据块不重新生成
				continue;
			}
		} else if (render_to_data_factor == 2) {
			// 渲染数据块对应 8 个更小的数据块
			uint8_t edited_mask = 0;
			const Vector3i data_pos0 = block.grid_position * render_to_data_factor;
			edited_mask |= L::has_edited_block(lod, Vector3i(data_pos0.x, data_pos0.y, data_pos0.z));
			edited_mask |= (L::has_edited_block(lod, Vector3i(data_pos0.x + 1, data_pos0.y, data_pos0.z)) << 1);
			edited_mask |= (L::has_edited_block(lod, Vector3i(data_pos0.x, data_pos0.y + 1, data_pos0.z)) << 2);
			edited_mask |= (L::has_edited_block(lod, Vector3i(data_pos0.x + 1, data_pos0.y + 1, data_pos0.z)) << 3);
			edited_mask |= (L::has_edited_block(lod, Vector3i(data_pos0.x, data_pos0.y, data_pos0.z + 1)) << 4);
			edited_mask |= (L::has_edited_block(lod, Vector3i(data_pos0.x + 1, data_pos0.y, data_pos0.z + 1)) << 5);
			edited_mask |= (L::has_edited_block(lod, Vector3i(data_pos0.x, data_pos0.y + 1, data_pos0.z + 1)) << 6);
			edited_mask |= (L::has_edited_block(lod, Vector3i(data_pos0.x + 1, data_pos0.y + 1, data_pos0.z + 1)) << 7);
			octant_mask = ~edited_mask;
			if (octant_mask == 0) {
				// 所有数据块均已被编辑，整个渲染数据块不重新生成
				continue;
			}
		}

		StdVector<Transform3f> &transform_cache = get_tls_transform_cache();
		transform_cache.clear();

		Array surface_arrays;
		int32_t vertex_range_end = -1;
		int32_t index_range_end = -1;
		if (parent_vlt != nullptr) {
			surface_arrays = parent_vlt->get_mesh_block_surface(
					block.grid_position, lod_index, vertex_range_end, index_range_end
			);
		} else if (parent_vt != nullptr) {
			surface_arrays = parent_vt->get_mesh_block_surface(block.grid_position);
		}

		const int mesh_block_size = 1 << _parent_mesh_block_size_po2;
		const int lod_block_size = mesh_block_size << lod_index;

		item->get_generator()->generate_transforms(
				transform_cache,
				block.grid_position,
				block.lod_index,
				layer_id,
				surface_arrays,
				vertex_range_end,
				index_range_end,
				_up_mode,
				octant_mask,
				lod_block_size,
				voxel_generator
		);

		if (render_to_data_factor == 2 && octant_mask != 0xff) {
			// 用已编辑的变换补全变换
			L::extract_octant_transforms(block, transform_cache, ~octant_mask, mesh_block_size);
			// TODO 如果这些数据块已加载数据但尚未上传用于渲染，该怎么办？
			// 我们也可以设置本地变换列表，因为从 VisualServer 获取变换很昂贵
		}

		const Transform3D block_local_transform(Basis(), Vector3(block.grid_position * lod_block_size));
		const Transform3D block_transform = parent_transform * block_local_transform;

		update_block_from_transforms(
				block_index,
				to_span_const(transform_cache),
				block.grid_position,
				layer,
				**item,
				layer_id,
				world,
				block_transform,
				block_local_transform.origin
		);
	}
}

void VoxelInstancer::update_layer_meshes(int layer_id) {
	Ref<VoxelInstanceLibraryItem> item_base = _library->get_item(layer_id);
	ERR_FAIL_COND(item_base.is_null());
	// 此方法预期在 multimesh 图层上运行
	VoxelInstanceLibraryMultiMeshItem *item = Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(*item_base);
	ERR_FAIL_COND(item == nullptr);

	const bool hide_beyond_max_lod = item->get_hide_beyond_max_lod();

	const bool instancer_is_visible = is_inside_tree() && is_visible_in_tree();

	const VoxelInstanceLibraryMultiMeshItem::Settings &settings = item->get_multimesh_settings();
	const unsigned int extended_mesh_lod_count = settings.mesh_lod_count + (hide_beyond_max_lod ? 1 : 0);

	for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
		Block &block = **it;
		if (block.layer_id != layer_id || !block.multimesh_instance.is_valid()) {
			continue;
		}
		block.multimesh_instance.set_render_layer(settings.render_layer);
		block.multimesh_instance.set_material_override(settings.material_override);
		block.multimesh_instance.set_cast_shadows_setting(settings.shadow_casting_setting);
		block.multimesh_instance.set_gi_mode(settings.gi_mode);

		block.current_mesh_lod = math::min(static_cast<unsigned int>(block.current_mesh_lod), extended_mesh_lod_count);
		update_mesh_from_mesh_lod(block, settings, hide_beyond_max_lod, instancer_is_visible);
	}
}

void VoxelInstancer::update_layer_scenes(int layer_id) {
	Ref<VoxelInstanceLibraryItem> item_base = _library->get_item(layer_id);
	ERR_FAIL_COND(item_base.is_null());
	// 此方法预期在场景图层上运行
	VoxelInstanceLibrarySceneItem *item = Object::cast_to<VoxelInstanceLibrarySceneItem>(*item_base);
	ERR_FAIL_COND(item == nullptr);
	const int data_block_size_po2 = _parent_data_block_size_po2;

	for (unsigned int block_index = 0; block_index < _blocks.size(); ++block_index) {
		Block &block = *_blocks[block_index];

		for (unsigned int instance_index = 0; instance_index < block.scene_instances.size(); ++instance_index) {
			SceneInstance prev_instance = block.scene_instances[instance_index];
			ERR_CONTINUE(prev_instance.root == nullptr);
			SceneInstance instance = create_scene_instance(
					*item, instance_index, block_index, prev_instance.root->get_transform(), data_block_size_po2
			);
			ERR_CONTINUE(instance.root == nullptr);
			block.scene_instances[instance_index] = instance;
			// 我们直接丢弃实例而不保存，因为此函数应仅在编辑器中调用，
			// 或仅在游戏中修改库的极少数情况下调用（这无论如何都会使保存失效）。
			prev_instance.root->queue_free();
		}
	}
}

void VoxelInstancer::on_library_item_changed(int item_id, IInstanceLibraryItemListener::ChangeType change) {
	ERR_FAIL_COND(_library.is_null());

	// TODO 尚不清楚某些代码路径在实例被编辑时是否能正确处理

	// 此回调将在库加载后触发，因此大多发生在编辑器中用户更改配置时。
	// 若发生在游戏中，可能会导致性能问题。若是如此，最好在将库分配给实例化器之前
	// 配置好库。

	switch (change) {
		case IInstanceLibraryItemListener::CHANGE_ADDED: {
			Ref<VoxelInstanceLibraryItem> item = _library->get_item(item_id);
			ERR_FAIL_COND(item.is_null());
			add_layer(item_id, item->get_lod_index());
			// 在编辑器中，若删除 VoxelInstancer，Godot 并不会真正删除它。相反，它会将其从
			// 场景树中移除并保留在 UndoRedo 历史中。但当库被修改时，该节点仍会收到
			// 通知...这会导致几个问题：
			// - 节点需要访问 World3D 才能更新，否则会报错
			// - 理论上我们可以不要求 World3D，但这仍意味着需要大量处理来
			// 重新生成图层，这对一个未激活或“当前”已被用户删除的节点来说浪费了 CPU。
			// 因此，在该状态下我们停止其重新生成图层。我不确定我们应在多大程度上
			// 支持树外自动刷新...可能还有比这更多的边界情况。
			if (is_inside_tree()) {
				regenerate_layer(item_id, true);
			}
			update_configuration_warnings();
		} break;

		case IInstanceLibraryItemListener::CHANGE_REMOVED:
			remove_layer(item_id);
			update_configuration_warnings();
			break;

		case IInstanceLibraryItemListener::CHANGE_GENERATOR:
			// 若节点已在编辑器中删除，则不更新...
			if (is_inside_tree()) {
				regenerate_layer(item_id, false);
			}
			break;

		case IInstanceLibraryItemListener::CHANGE_VISUAL:
			update_layer_meshes(item_id);
			break;

		case IInstanceLibraryItemListener::CHANGE_SCENE:
			update_layer_scenes(item_id);
			break;

		case IInstanceLibraryItemListener::CHANGE_LOD_INDEX: {
			Ref<VoxelInstanceLibraryItem> item = _library->get_item(item_id);
			ERR_FAIL_COND(item.is_null());

			clear_blocks_in_layer(item_id);

			Layer &layer = get_layer(item_id);

			Lod &prev_lod = _lods[layer.lod_index];
			unordered_remove_value(prev_lod.layers, item_id);

			layer.lod_index = item->get_lod_index();

			Lod &new_lod = _lods[layer.lod_index];
			new_lod.layers.push_back(item_id);

			// 若节点已在编辑器中删除，则不更新...
			if (is_inside_tree()) {
				regenerate_layer(item_id, true);
			}
		} break;

		default:
			ERR_PRINT("Unknown change");
			break;
	}

	update_configuration_warnings();
}

void VoxelInstancer::add_layer(int layer_id, int lod_index) {
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(lod_index < 0 || lod_index >= MAX_LOD);
	ERR_FAIL_COND_MSG(_layers.find(layer_id) != _layers.end(), "Trying to add a layer that already exists");
#endif

	Lod &lod = _lods[lod_index];

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(
			std::find(lod.layers.begin(), lod.layers.end(), layer_id) != lod.layers.end(),
			"Layer already referenced by this LOD"
	);
#endif

	Layer layer;
	layer.lod_index = lod_index;
	_layers.insert({ layer_id, layer });

	lod.layers.push_back(layer_id);
}

void VoxelInstancer::remove_layer(int layer_id) {
	Layer &layer = get_layer(layer_id);

	// 从对应的 LOD 结构中注销该图层
	Lod &lod = _lods[layer.lod_index];
	for (size_t i = 0; i < lod.layers.size(); ++i) {
		if (lod.layers[i] == layer_id) {
			lod.layers[i] = lod.layers.back();
			lod.layers.pop_back();
			break;
		}
	}

	clear_blocks_in_layer(layer_id);

	_layers.erase(layer_id);
}

void VoxelInstancer::destroy_multimesh_block_colliders(Block &block) {
	VOXEL_PROFILE_SCOPE();
	for (unsigned int i = 0; i < block.bodies.size(); ++i) {
		VoxelInstancerRigidBody *body = block.bodies[i];
		body->detach_and_destroy();
	}
	block.bodies.clear();
}

void VoxelInstancer::remove_block(const unsigned int block_index, const bool with_fade_out) {
#ifdef DEBUG_ENABLED
	CRASH_COND(block_index >= _blocks.size());
#endif
	// 我们将最后一个数据块移动到被移除数据块原先占用的索引处。
	// 这比移动数组中的每个数据块更便宜。
	// 先获取此引用，因为如果移除的是最后一个数据块，其地址将因移动语义而变为空
	const Block &moved_block = *_blocks.back();

	UniquePtr<Block> block = std::move(_blocks[block_index]);
	{
		Layer &layer = get_layer(block->layer_id);
		layer.blocks.erase(block->grid_position);
	}
	_blocks[block_index] = std::move(_blocks.back());
	_blocks.pop_back();

	// 销毁与该数据块关联的对象

	destroy_multimesh_block_colliders(*block);

	for (unsigned int i = 0; i < block->scene_instances.size(); ++i) {
		SceneInstance instance = block->scene_instances[i];
		ERR_CONTINUE(instance.component == nullptr);
		instance.component->detach();
		ERR_CONTINUE(instance.root == nullptr);
		instance.root->queue_free();
	}

	// 更新数据块索引引用，因为在交换删除期间我们必须更改最后一个数据块的索引。
	// 若被移除的数据块恰好是最后一个，则不会进入此处
	if (block.get() != &moved_block) {
		// 更新其图层中引用的被移动数据块的索引
		Layer &layer = get_layer(moved_block.layer_id);
		auto it = layer.blocks.find(moved_block.grid_position);
		CRASH_COND(it == layer.blocks.end());
		it->second = block_index;

		for (VoxelInstancerRigidBody *body : moved_block.bodies) {
			body->set_render_block_index(block_index);
		}
		for (const SceneInstance &scene : moved_block.scene_instances) {
			scene.component->set_render_block_index(block_index);
		}
	}

	if (with_fade_out && block->multimesh_instance.is_valid()) {
		FadingOutBlock fb;
		fb.multimesh_instance = std::move(block->multimesh_instance);
		_fading_out_blocks.push_back(std::move(fb));
	}
}

// void VoxelInstancer::on_data_block_loaded(
// 		Vector3i grid_position, unsigned int lod_index, UniquePtr<InstanceBlockData> instances) {
// 	ERR_FAIL_COND(lod_index >= _lods.size());
// 	Lod &lod = _lods[lod_index];
// 	lod.loaded_instances_data.insert(std::make_pair(grid_position, std::move(instances)));
// }

void VoxelInstancer::on_mesh_block_enter(
		const Vector3i render_grid_position,
		const unsigned int lod_index,
		Array surface_arrays,
		const int32_t vertex_range_end,
		const int32_t index_range_end
) {
	if (lod_index >= _lods.size()) {
		return;
	}
	create_render_blocks(render_grid_position, lod_index, surface_arrays, vertex_range_end, index_range_end);
}

void VoxelInstancer::on_mesh_block_exit(const Vector3i render_grid_position, const unsigned int lod_index) {
	if (lod_index >= _lods.size()) {
		// 实例化器不处理较大的 LOD
		return;
	}

	VOXEL_PROFILE_SCOPE();

	Lod &lod = _lods[lod_index];

	BufferedTaskScheduler &scheduler = BufferedTaskScheduler::get_for_current_thread();

	const bool can_save = _parent != nullptr && _parent->get_stream().is_valid();

	// 移除数据块
	const int render_to_data_factor = 1 << (_parent_mesh_block_size_po2 - _parent_data_block_size_po2);
	ERR_FAIL_COND(render_to_data_factor <= 0 || render_to_data_factor > 2);
	const Vector3i data_min_pos = render_grid_position * render_to_data_factor;
	const Vector3i data_max_pos = data_min_pos + Vector3iUtil::create(render_to_data_factor);
	Vector3i data_grid_pos;
	for (data_grid_pos.z = data_min_pos.z; data_grid_pos.z < data_max_pos.z; ++data_grid_pos.z) {
		for (data_grid_pos.y = data_min_pos.y; data_grid_pos.y < data_max_pos.y; ++data_grid_pos.y) {
			for (data_grid_pos.x = data_min_pos.x; data_grid_pos.x < data_max_pos.x; ++data_grid_pos.x) {
				// 若我们在那里加载了数据但从未使用过，无论如何都会将其卸载。
				// 注意，这些数据是我们最初加载的，不包含修改。
				lod.edited_data_blocks.erase(data_grid_pos);

				auto modified_block_it = lod.modified_blocks.find(data_grid_pos);
				if (modified_block_it != lod.modified_blocks.end()) {
					if (can_save) {
						SaveBlockDataTask *task = save_block(data_grid_pos, lod_index, nullptr, false, true);
						if (task != nullptr) {
							scheduler.push_io_task(task);
						}
					}
					lod.modified_blocks.erase(modified_block_it);
				}
			}
		}
	}

	scheduler.flush();

	// 移除渲染数据块
	for (auto layer_it = lod.layers.begin(); layer_it != lod.layers.end(); ++layer_it) {
		const int layer_id = *layer_it;

		Layer &layer = get_layer(layer_id);

		auto block_it = layer.blocks.find(render_grid_position);
		if (block_it != layer.blocks.end()) {
			remove_block(block_it->second, _fading_enabled);
		}
	}
}

void VoxelInstancer::save_all_modified_blocks(
		BufferedTaskScheduler &tasks,
		std::shared_ptr<AsyncDependencyTracker> tracker,
		bool with_flush
) {
	VOXEL_DSTACK();

	VOXEL_ASSERT_RETURN(_parent != nullptr);
	const bool can_save = _parent->get_stream().is_valid();
	VOXEL_ASSERT_RETURN_MSG(
			can_save,
			format("Cannot save instances, the parent {} has no {} assigned.",
				   _parent->get_class(),
				   String(VoxelStream::get_class_static()))
	);

	for (unsigned int lod_index = 0; lod_index < _lods.size(); ++lod_index) {
		Lod &lod = _lods[lod_index];
		for (auto it = lod.modified_blocks.begin(); it != lod.modified_blocks.end(); ++it) {
			SaveBlockDataTask *task = save_block(*it, lod_index, tracker, with_flush, false);
			if (task != nullptr) {
				tasks.push_io_task(task);
			}
		}
		lod.modified_blocks.clear();
	}
}

void VoxelInstancer::remove_instances_in_sphere(const Vector3 p_center, const float p_radius) {
	VOXEL_PROFILE_MESSAGE("RemoveInSphere");

	class RemoveInSphere : public IAreaOperation {
	private:
		VoxelInstancer &_instancer;
		const Vector3 _center;
		const float _radius_squared;
		MMRemovalAction _mm_removal_action;
		uint32_t _mm_removal_action_item_id;

	public:
		RemoveInSphere(VoxelInstancer &pp_instancer, const Vector3 pp_center, const float pp_radius) :
				_instancer(pp_instancer),
				_center(pp_center),
				_radius_squared(pp_radius * pp_radius),
				_mm_removal_action_item_id(VoxelInstanceLibrary::MAX_ID) //
		{}

		Result execute(Block &block) override {
			VOXEL_PROFILE_SCOPE();

			const unsigned int base_block_size = 1 << _instancer._parent_mesh_block_size_po2;
			const Vector3 block_origin = Vector3i(block.grid_position * (base_block_size << block.lod_index));
			const Vector3f center_local = to_vec3f(_center - block_origin);

			// TODO 临时分配器的候选
			// TODO 如果我们有自己的缓存，可能就完全不需要分配了
			StdVector<Vector3f> instance_positions;
			get_instance_positions_local(block, base_block_size, instance_positions, nullptr);

			// TODO 临时分配器的候选
			StdVector<uint32_t> instances_to_remove;
			unsigned int instance_index = 0;
			for (const Vector3f &instance_pos : instance_positions) {
				const float ds = math::distance_squared(instance_pos, center_local);
				if (ds < _radius_squared) {
					instances_to_remove.push_back(instance_index);
				}
				++instance_index;
			}

			if (instances_to_remove.size() == 0) {
				return { false };
			}

			try_update_item_cache(block.layer_id);

			remove_instances_by_index(block, base_block_size, to_span(instances_to_remove), _mm_removal_action);

			return { true };
		}

	private:
		void try_update_item_cache(const uint32_t item_id) {
			if (item_id == _mm_removal_action_item_id) {
				return;
			}

			// 重新缓存项信息

			_mm_removal_action = MMRemovalAction();
			_mm_removal_action_item_id = item_id;

			if (_instancer._library.is_null()) {
				return;
			}
			VoxelInstanceLibraryItem *item = _instancer._library->get_item(item_id);
			if (item == nullptr) {
				return;
			}
			VoxelInstanceLibraryMultiMeshItem *mm_item = Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(item);
			if (mm_item == nullptr) {
				return;
			}
			_mm_removal_action = get_mm_removal_action(&_instancer, mm_item);
		}
	};

	RemoveInSphere op(*this, p_center, p_radius);

	const Vector3 rv(p_radius, p_radius, p_radius);
	do_area_operation(AABB(p_center - rv, 2 * rv), op);
}

VoxelInstancer::SceneInstance VoxelInstancer::create_scene_instance(
		const VoxelInstanceLibrarySceneItem &scene_item,
		int instance_index,
		unsigned int block_index,
		Transform3D transform,
		int data_block_size_po2
) {
	SceneInstance instance;
	ERR_FAIL_COND_V_MSG(
			scene_item.get_scene().is_null(),
			instance,
			String("{0} ({1}) is missing an attached scene in {2} ({3})")
					.format(
							varray(VoxelInstanceLibrarySceneItem::get_class_static(),
								   scene_item.get_item_name(),
								   VoxelInstancer::get_class_static(),
								   get_path())
					)
	);
	Node *root = scene_item.get_scene()->instantiate();
	ERR_FAIL_COND_V(root == nullptr, instance);
	instance.root = Object::cast_to<Node3D>(root);
	ERR_FAIL_COND_V_MSG(instance.root == nullptr, instance, "Root of scene instance must be derived from Spatial");

	instance.component = VoxelInstanceComponent::find_in(instance.root);
	if (instance.component == nullptr) {
		instance.component = memnew(VoxelInstanceComponent);
		instance.root->add_child(instance.component);
	}

	instance.component->attach(this);
	instance.component->set_instance_index(instance_index);
	instance.component->set_render_block_index(block_index);
	instance.component->set_data_block_position(math::floor_to_int(transform.origin) >> data_block_size_po2);

	instance.root->set_transform(transform);

	// 这是最慢的部分，因为 Godot 会触发各种回调
	add_child(instance.root);

	return instance;
}

unsigned int VoxelInstancer::create_block(
		Layer &layer,
		const uint16_t layer_id,
		const Vector3i grid_position,
		const bool pending_instances
) {
	UniquePtr<Block> block = make_unique_instance<Block>();
	block->layer_id = layer_id;
	block->current_mesh_lod = 0;
	block->lod_index = layer.lod_index;
	block->grid_position = grid_position;
	block->pending_instances = pending_instances;
	const unsigned int block_index = _blocks.size();
	_blocks.push_back(std::move(block));
#ifdef DEBUG_ENABLED
	// 数据块必须尚不存在
	CRASH_COND(layer.blocks.find(grid_position) != layer.blocks.end());
#endif
	layer.blocks.insert({ grid_position, block_index });
	return block_index;
}

void VoxelInstancer::update_block_from_transforms(
		int block_index,
		Span<const Transform3f> transforms,
		const Vector3i grid_position,
		Layer &layer,
		const VoxelInstanceLibraryItem &item_base,
		const uint16_t layer_id,
		World3D &world,
		const Transform3D &block_global_transform,
		const Vector3 block_local_position
) {
	VOXEL_PROFILE_SCOPE();

	// 获取或创建数据块
	if (block_index == -1) {
		block_index = create_block(layer, layer_id, grid_position, false);
	}
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(block_index < 0 || block_index >= static_cast<int>(_blocks.size()));
	ERR_FAIL_COND(_blocks[block_index] == nullptr);
#endif
	Block &block = *_blocks[block_index];

	// 更新 multimesh
	const VoxelInstanceLibraryMultiMeshItem *item = Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(&item_base);
	if (item != nullptr) {
		update_multimesh_block_from_transforms(
				block, block_index, block_global_transform, block_local_position, transforms, *item, world
		);
		return;
	}

	// 更新场景实例
	const VoxelInstanceLibrarySceneItem *scene_item = Object::cast_to<VoxelInstanceLibrarySceneItem>(&item_base);
	if (scene_item != nullptr) {
		update_scene_block_from_transforms(block, block_index, block_local_position, transforms, *scene_item);
	}
}

void VoxelInstancer::update_multimesh_block_from_transforms(
		Block &block,
		const unsigned int block_index,
		const Transform3D &block_global_transform,
		const Vector3 block_local_position,
		Span<const Transform3f> transforms,
		const VoxelInstanceLibraryMultiMeshItem &item,
		World3D &world
) {
	VOXEL_PROFILE_SCOPE();

	const VoxelInstanceLibraryMultiMeshItem::Settings &settings = item.get_multimesh_settings();

	if (transforms.size() == 0) {
		if (block.multimesh_instance.is_valid()) {
			block.multimesh_instance.set_multimesh(Ref<MultiMesh>());
			block.multimesh_instance.destroy();
		}

	} else {
		Ref<MultiMesh> multimesh = block.multimesh_instance.get_multimesh();
		if (multimesh.is_null()) {
			multimesh.instantiate();
			multimesh->set_transform_format(MultiMesh::TRANSFORM_3D);
			multimesh->set_use_colors(false);
			multimesh->set_use_custom_data(false);
		} else {
			multimesh->set_visible_instance_count(-1);
		}
		PackedFloat32Array bulk_array;
		voxel::godot::DirectMultiMeshInstance::make_transform_3d_bulk_array(transforms, bulk_array);
		multimesh->set_instance_count(transforms.size());

		// 在 `multimesh_set_buffer` 之前设置网格，因为否则 Godot 会在 `multimesh_set_buffer` 内部
		// 通过从显卡下载回缓冲区来计算 AABB，这会带来非常严重的性能惩罚
		// TODO 如果我们能使用自定义 AABB，就不需要这种重新排序
		if (settings.mesh_lod_count > 0) {
			if (block.current_mesh_lod < settings.mesh_lod_count) {
				multimesh->set_mesh(settings.mesh_lods[block.current_mesh_lod]);
			}
		}

		// TODO 等待 Godot 在资源对象上公开该方法
		// multimesh->set_as_bulk_array(bulk_array);
		RenderingServer::get_singleton()->multimesh_set_buffer(multimesh->get_rid(), bulk_array);

		if (!block.multimesh_instance.is_valid()) {
			block.multimesh_instance.create();
			block.multimesh_instance.set_interpolated(false);
			block.multimesh_instance.set_visible(
					is_visible() &&
					!(item.get_hide_beyond_max_lod() && block.current_mesh_lod == settings.mesh_lod_count)
			);
		}
		block.multimesh_instance.set_multimesh(multimesh);
		block.multimesh_instance.set_render_layer(settings.render_layer);
		block.multimesh_instance.set_world(&world);
		block.multimesh_instance.set_transform(block_global_transform);
		block.multimesh_instance.set_material_override(settings.material_override);
		block.multimesh_instance.set_cast_shadows_setting(settings.shadow_casting_setting);
		block.multimesh_instance.set_gi_mode(settings.gi_mode);

		if (settings.mesh_lod_count > 1 || (settings.mesh_lod_count == 1 && item.get_hide_beyond_max_lod())) {
			// 暂时隐藏，让 LOD 系统运行时负责显示/隐藏并分配正确的网格。我们这样做是因为
			// LOD 系统不一定每帧更新每个数据块，否则在生成时它们会以其完整
			// LOD 闪烁
			block.current_mesh_lod = settings.mesh_lod_count;
			block.multimesh_instance.set_visible(false);
		}
	}

	if (item.get_collision_distance() < 0.f) {
		// 碰撞体始终存在，立即全部创建
		update_multimesh_block_colliders(block, block_index, settings, transforms, block_local_position);
	}
}

void VoxelInstancer::update_multimesh_block_colliders(
		Block &block,
		const uint32_t block_index,
		const InstanceLibraryMultiMeshItemSettings &settings,
		Span<const Transform3f> transforms,
		const Vector3 block_local_position
) {
	// 更新刚体
	Span<const CollisionShapeInfo> collision_shapes = to_span(settings.collision_shapes);
	if (collision_shapes.size() == 0) {
		return;
	}

	VOXEL_PROFILE_SCOPE();

	const int data_block_size_po2 = _parent_data_block_size_po2;

	// 添加新刚体
	for (unsigned int instance_index = 0; instance_index < transforms.size(); ++instance_index) {
		const Transform3D local_transform = to_transform3(transforms[instance_index]);
		// 刚体是实例化器的子节点，因此我们使用本地数据块坐标
		const Transform3D body_transform(local_transform.basis, local_transform.origin + block_local_position);

		VoxelInstancerRigidBody *body;

		if (instance_index < block.bodies.size()) {
			// 刚体已存在，我们仅更新其属性
			body = block.bodies[instance_index];

		} else {
			// 创建刚体

			// TODO 性能：从场景树中移除节点很慢，会导致帧率卡顿。
			// 参见 https://github.com/godotengine/godot/issues/61929
			// 带碰撞体的实例可能导致创建数千个节点。虽然这在实践中可行，
			// 但移除被证明非常慢。不是因为物理引擎，而是因为节点系统本身的问题。
			// 一种可能的解决方法是直接使用服务器，或者将节点作为更多充当桶的节点的子节点。
			body = memnew(VoxelInstancerRigidBody);
			body->attach(this);
			body->set_instance_index(instance_index);
			body->set_render_block_index(block_index);
			body->set_data_block_position(math::floor_to_int(body_transform.origin) >> data_block_size_po2);
			body->set_collision_layer(settings.collision_layer);
			body->set_collision_mask(settings.collision_mask);

			for (unsigned int i = 0; i < collision_shapes.size(); ++i) {
				const CollisionShapeInfo &shape_info = collision_shapes[i];
				CollisionShape3D *cs = memnew(CollisionShape3D);
				cs->set_shape(shape_info.shape);
				cs->set_transform(shape_info.transform);
				body->add_child(cs);
			}

			for (const StringName &group_name : settings.group_names) {
				body->add_to_group(group_name);
			}

			add_child(body);
			block.bodies.push_back(body);
		}

		body->set_transform(body_transform);
	}

	// 移除多余的刚体
	for (unsigned int instance_index = transforms.size(); instance_index < block.bodies.size(); ++instance_index) {
		VoxelInstancerRigidBody *body = block.bodies[instance_index];
		body->detach_and_destroy();
	}

	block.bodies.resize(transforms.size());
}

void VoxelInstancer::update_scene_block_from_transforms(
		Block &block,
		const unsigned int block_index,
		const Vector3 block_local_position,
		Span<const Transform3f> transforms,
		const VoxelInstanceLibrarySceneItem &scene_item
) {
	VOXEL_PROFILE_SCOPE();

	ERR_FAIL_COND_MSG(
			scene_item.get_scene().is_null(),
			String("{0} ({1}) is missing an attached scene in {2} ({3})")
					.format(
							varray(VoxelInstanceLibrarySceneItem::get_class_static(),
								   scene_item.get_item_name(),
								   VoxelInstancer::get_class_static(),
								   get_path())
					)
	);

	const int data_block_size_po2 = _parent_data_block_size_po2;

	// 添加新实例
	for (unsigned int instance_index = 0; instance_index < transforms.size(); ++instance_index) {
		const Transform3D local_transform = to_transform3(transforms[instance_index]);
		const Transform3D body_transform(local_transform.basis, local_transform.origin + block_local_position);

		SceneInstance instance;

		if (instance_index < static_cast<unsigned int>(block.bodies.size())) {
			instance = block.scene_instances[instance_index];
			instance.root->set_transform(body_transform);

		} else {
			instance =
					create_scene_instance(scene_item, instance_index, block_index, body_transform, data_block_size_po2);
			ERR_CONTINUE(instance.root == nullptr);
			block.scene_instances.push_back(instance);
		}

		// TODO 反序列化状态
	}

	// 移除旧实例
	for (unsigned int instance_index = transforms.size(); instance_index < block.scene_instances.size();
		 ++instance_index) {
		SceneInstance instance = block.scene_instances[instance_index];
		ERR_CONTINUE(instance.component == nullptr);
		instance.component->detach();
		ERR_CONTINUE(instance.root == nullptr);
		instance.root->queue_free();
	}

	block.scene_instances.resize(transforms.size());
}

void VoxelInstancer::create_render_blocks(
		const Vector3i render_grid_position,
		const int lod_index,
		Array surface_arrays,
		const int32_t vertex_range_end,
		const int32_t index_range_end
) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(_library.is_valid());
	VOXEL_ASSERT_RETURN(_parent != nullptr);
	Ref<VoxelStream> stream = _parent->get_stream();
	Ref<VoxelGenerator> generator = _parent->get_generator();

	const unsigned int render_block_size = 1 << _parent_mesh_block_size_po2;
	const unsigned int data_block_size = 1 << _parent_data_block_size_po2;

	Lod &lod = _lods[lod_index];

	// 创建处于挂起状态的空数据块
	for (auto layer_it = lod.layers.begin(); layer_it != lod.layers.end(); ++layer_it) {
		const int layer_id = *layer_it;

		Layer &layer = get_layer(layer_id);

		if (layer.blocks.find(render_grid_position) != layer.blocks.end()) {
			// 数据块已经创建过了？
			continue;
		}

		create_block(layer, layer_id, render_grid_position, true);
	}

	LoadInstanceChunkTask *task = VOXEL_NEW(LoadInstanceChunkTask(
			_loading_results,
			stream,
			generator,
			lod.quick_reload_cache,
			_library,
			surface_arrays,
			vertex_range_end,
			index_range_end,
			render_grid_position,
			lod_index,
			render_block_size,
			data_block_size,
			_up_mode
	));

	VoxelEngine::get_singleton().push_async_io_task(task);
}

SaveBlockDataTask *VoxelInstancer::save_block(
		Vector3i data_grid_pos,
		int lod_index,
		std::shared_ptr<AsyncDependencyTracker> tracker,
		bool with_flush,
		bool cache_while_saving
) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND_V(_library.is_null(), nullptr);
	ERR_FAIL_COND_V(_parent == nullptr, nullptr);

	VOXEL_PRINT_VERBOSE(format("Requesting save of instance block {} lod {}", data_grid_pos, lod_index));

	const Lod &lod = _lods[lod_index];

	UniquePtr<InstanceBlockData> block_data = make_unique_instance<InstanceBlockData>();
	const int data_block_size = (1 << _parent_data_block_size_po2) << lod_index;
	block_data->position_range = data_block_size;

	const int render_to_data_factor = (1 << _parent_mesh_block_size_po2) / (1 << _parent_data_block_size_po2);
	ERR_FAIL_COND_V_MSG(render_to_data_factor < 1 || render_to_data_factor > 2, nullptr, "Unsupported block size");

	const int render_block_size_base = (1 << _parent_mesh_block_size_po2);
	const int render_block_size = render_block_size_base << lod_index;
	const int half_render_block_size = render_block_size / 2;
	const Vector3i render_block_pos = math::floordiv(data_grid_pos, render_to_data_factor);

	const int octant_index =
			VoxelInstanceGenerator::get_octant_index(data_grid_pos.x & 1, data_grid_pos.y & 1, data_grid_pos.z & 1);

	for (auto it = lod.layers.begin(); it != lod.layers.end(); ++it) {
		const int layer_id = *it;

		const VoxelInstanceLibraryItem *item = _library->get_item_const(layer_id);
		CRASH_COND(item == nullptr);
		if (!item->is_persistent()) {
			continue;
		}

		const Layer &layer = get_layer_const(layer_id);

		ERR_FAIL_COND_V(layer_id < 0, nullptr);

		const auto render_block_it = layer.blocks.find(render_block_pos);
		if (render_block_it == layer.blocks.end()) {
			continue;
		}

		const unsigned int render_block_index = render_block_it->second;
#ifdef DEBUG_ENABLED
		CRASH_COND(render_block_index >= _blocks.size());
#endif
		Block &render_block = *_blocks[render_block_index];

		block_data->layers.push_back(InstanceBlockData::LayerData());
		InstanceBlockData::LayerData &layer_data = block_data->layers.back();

		layer_data.instances.clear();
		layer_data.id = layer_id;

		if (item->get_generator().is_valid()) {
			layer_data.scale_min = item->get_generator()->get_min_scale();
			layer_data.scale_max = item->get_generator()->get_max_scale();
		} else {
			// TODO 在序列化器中自动计算缩放范围
			layer_data.scale_min = 0.1f;
			layer_data.scale_max = 10.f;
		}

		if (render_block.multimesh_instance.is_valid()) {
			// Multimesh 网格

			Ref<MultiMesh> multimesh = render_block.multimesh_instance.get_multimesh();
			CRASH_COND(multimesh.is_null());

			VOXEL_PROFILE_SCOPE();

			const int instance_count = voxel::godot::get_visible_instance_count(**multimesh);

			if (render_to_data_factor == 1) {
				layer_data.instances.resize(instance_count);

				// TODO 优化：如果能一次性获取整个数组就好了
				for (int instance_index = 0; instance_index < instance_count; ++instance_index) {
					// TODO 优化：首次执行非常慢，之后仍有开销。
					layer_data.instances[instance_index].transform =
							to_transform3f(multimesh->get_instance_transform(instance_index));
				}

			} else if (render_to_data_factor == 2) {
				for (int instance_index = 0; instance_index < instance_count; ++instance_index) {
					// TODO 优化：在多线程模式下这很糟糕！考虑保留一份本地副本...
					const Transform3D rendered_instance_transform = multimesh->get_instance_transform(instance_index);
					const int instance_octant_index = VoxelInstanceGenerator::get_octant_index(
							to_vec3f(rendered_instance_transform.origin), half_render_block_size
					);
					if (instance_octant_index == octant_index) {
						InstanceBlockData::InstanceData d;
						d.transform = to_transform3f(rendered_instance_transform);
						layer_data.instances.push_back(d);
					}
				}
			}

		} else if (render_block.scene_instances.size() > 0) {
			// 场景

			VOXEL_PROFILE_SCOPE();
			const unsigned int instance_count = render_block.scene_instances.size();

			const Vector3 render_block_origin = render_block_pos * render_block_size;

			if (render_to_data_factor == 1) {
				layer_data.instances.resize(instance_count);

				for (unsigned int instance_index = 0; instance_index < instance_count; ++instance_index) {
					const SceneInstance instance = render_block.scene_instances[instance_index];
					ERR_CONTINUE(instance.root == nullptr);
					layer_data.instances[instance_index].transform =
							to_transform3f(instance.root->get_transform().translated(-render_block_origin));
				}

			} else if (render_to_data_factor == 2) {
				for (unsigned int instance_index = 0; instance_index < instance_count; ++instance_index) {
					const SceneInstance instance = render_block.scene_instances[instance_index];
					ERR_CONTINUE(instance.root == nullptr);
					Transform3D t = instance.root->get_transform();
					t.origin -= render_block_origin;
					const int instance_octant_index =
							VoxelInstanceGenerator::get_octant_index(to_vec3f(t.origin), half_render_block_size);
					if (instance_octant_index == octant_index) {
						InstanceBlockData::InstanceData d;
						d.transform = to_transform3f(t);
						layer_data.instances.push_back(d);
					}
					// TODO 序列化状态？
				}
			}

			// 使场景变换相对于渲染数据块
			// for (InstanceBlockData::InstanceData &d : layer_data.instances) {
			// 	d.transform.origin -= render_block_origin;
			// }
		}

		if (render_to_data_factor == 2) {
			// 数据块所在的网格比渲染数据块更小，因此我们可以转换实例的相对位置
			const Vector3f rel = to_vec3f(data_block_size * (data_grid_pos - render_block_pos * render_to_data_factor));
			for (InstanceBlockData::InstanceData &d : layer_data.instances) {
				d.transform.origin -= rel;
			}
		}
	}

	const VolumeID volume_id = _parent->get_volume_id();

	std::shared_ptr<StreamingDependency> stream_dependency = _parent->get_streaming_dependency();
	VOXEL_ASSERT(stream_dependency != nullptr);

	if (cache_while_saving) {
		Lod &lod_mutable = _lods[lod_index];
		// 将数据保留在内存中，以防其很快被重新加载
		// TODO 预先复制副本效率不高，我们是否可以改用 shared_ptr？
		UniquePtr<InstanceBlockData> saving_cache = make_unique_instance<InstanceBlockData>();
		block_data->copy_to(*saving_cache);
		if (lod_mutable.quick_reload_cache == nullptr) {
			lod_mutable.quick_reload_cache = make_shared_instance<InstancerQuickReloadingCache>();
		}
		{
			MutexLock mlock(lod_mutable.quick_reload_cache->mutex);
			lod_mutable.quick_reload_cache->map[data_grid_pos] = std::move(saving_cache);
		}
	}

	SaveBlockDataTask *task = VOXEL_NEW(SaveBlockDataTask(
			volume_id, data_grid_pos, lod_index, std::move(block_data), stream_dependency, tracker, with_flush
	));

	return task;
}

inline bool detect_ground(
		const Vector3 instance_position_local,
		const Vector3 instance_up,
		const Vector3 block_origin,
		const float sd_threshold,
		const float sd_offset,
		const bool bidirectional,
		const VoxelTool &voxel_tool
) {
	const Vector3 normal_offset = sd_offset * instance_up;
	const Vector3 instance_pos_terrain = instance_position_local + block_origin;
	const Vector3 instance_pos_terrain_below = instance_pos_terrain + normal_offset;

	// TODO 优化：使用事务而不是随机单次查询
	const float sdf_below = voxel_tool.get_voxel_f_interpolated(instance_pos_terrain_below);
	if (sdf_below <= sd_threshold) {
		// 仍留有足够的支撑地面
		return true;
	}

	if (bidirectional) {
		// 改为尝试在上方采样，以防实例翻转
		const Vector3 instance_pos_terrain_above = instance_pos_terrain - normal_offset;
		const float sdf_above = voxel_tool.get_voxel_f_interpolated(instance_pos_terrain_above);
		if (sdf_above <= sd_threshold) {
			return true;
		}
	}

	return false;
}

VoxelInstancer::MMRemovalAction VoxelInstancer::get_mm_removal_action(
		VoxelInstancer *instancer,
		VoxelInstanceLibraryMultiMeshItem *mm_item
) {
	if (mm_item == nullptr) {
		VOXEL_PRINT_ERROR_ONCE("Didn't expect multimesh item to be null, bug?");
		return MMRemovalAction();
	}

	switch (mm_item->get_removal_behavior()) {
		case VoxelInstanceLibraryMultiMeshItem::REMOVAL_BEHAVIOR_NONE:
			break;

		case VoxelInstanceLibraryMultiMeshItem::REMOVAL_BEHAVIOR_INSTANTIATE: {
			{
				Ref<PackedScene> scene = mm_item->get_removal_scene();
				if (scene.is_null()) {
#ifdef TOOLS_ENABLED
					Ref<VoxelInstanceLibrary> lib = instancer->get_library();
					const int item_id = lib->get_item_id(mm_item);
					VOXEL_PRINT_ERROR_ONCE(format(
							"Removal behavior of item {} is set to instantiate a scene, but the scene is null.", item_id
					));
					return MMRemovalAction();
#endif
				}
			}
			MMRemovalAction action;
			action.context = { instancer, mm_item };
			action.callback = [](MMRemovalAction::Context ctx, const Transform3D &trans) {
				Ref<PackedScene> scene = ctx.item->get_removal_scene();
				Node *root = scene->instantiate();
				Node3D *root_3d = Object::cast_to<Node3D>(root);
				if (root_3d != nullptr) {
					root_3d->set_transform(trans);
					// 当回调发生在移除刚体的过程中时，我们不能调用 add_child，因为 Godot
					// 在该过程中会锁定 VoxelInstancer 的子节点，从而阻止添加节点...
					// ctx.instancer->add_child(root);
					ctx.instancer->call_deferred(VoxelStringNames::get_singleton().add_child, root);
				} else {
#ifdef TOOLS_ENABLED
					Ref<VoxelInstanceLibrary> lib = ctx.instancer->get_library();
					const int item_id = lib->get_item_id(ctx.item);
					VOXEL_PRINT_ERROR_ONCE(
							format("Removal behavior of item {} is set to instantiate a scene, but its root is not "
								   "a Node3D.",
								   item_id)
					);
#endif
				}
			};
			return action;
		}

		case VoxelInstanceLibraryMultiMeshItem::REMOVAL_BEHAVIOR_CALLBACK: {
			MMRemovalAction action;
			action.context = { instancer, mm_item };
			action.callback = [](MMRemovalAction::Context ctx, const Transform3D &trans) {
				ctx.item->trigger_removal_callback(ctx.instancer, trans);
			};
			return action;
		}

		default:
			VOXEL_PRINT_ERROR("Unknown removal mode");
			break;
	}

	return MMRemovalAction();
}

void VoxelInstancer::get_instance_positions_local(
		const Block &block,
		const unsigned int base_block_size,
		StdVector<Vector3f> &dst_positions,
		StdVector<Vector3f> *dst_normals
) {
	VOXEL_PROFILE_SCOPE();

	dst_positions.clear();
	if (dst_normals != nullptr) {
		dst_normals->clear();
	}

	if (block.scene_instances.size() > 0) {
		const Vector3 block_origin(block.grid_position * (base_block_size << block.lod_index));

		dst_positions.reserve(block.scene_instances.size());

		for (const SceneInstance &si : block.scene_instances) {
			const Vector3 pos_terrain = si.root->get_position();
			dst_positions.push_back(to_vec3f(pos_terrain - block_origin));
		}

		if (dst_normals != nullptr) {
			dst_normals->reserve(dst_positions.size());

			for (const SceneInstance &si : block.scene_instances) {
				const Vector3 normal = voxel::godot::BasisUtility::get_up(si.root->get_basis());
				dst_positions.push_back(to_vec3f(normal));
			}
		}

	} else {
		if (!block.multimesh_instance.is_valid()) {
			// 空数据块
			return;
		}

		Ref<MultiMesh> multimesh = block.multimesh_instance.get_multimesh();
		VOXEL_ASSERT_RETURN(multimesh.is_valid());

		const unsigned int instance_count = voxel::godot::get_visible_instance_count(**multimesh);
		{
			VOXEL_PROFILE_SCOPE_NAMED("Alloc P");
			dst_positions.reserve(instance_count);
		}

		if (dst_normals != nullptr) {
			{
				VOXEL_PROFILE_SCOPE_NAMED("Alloc N");
				dst_normals->reserve(instance_count);
			}

			for (unsigned int instance_index = 0; instance_index < instance_count; ++instance_index) {
				// TODO 优化：首次执行非常慢，之后仍有开销。
				//      使用 `multimesh_get_buffer` 会更好吗？不幸的是它总是分配内存（它
				//      甚至不利用 CoW），而且也不缓存，所以为了强制它我们就必须做一次
				//      对 `get_instance_transform` 的伪调用。使用我们自己的缓存并小心避免 Godot
				//      填充它自己的缓存仍然更好...
				const Transform3D instance_transform = multimesh->get_instance_transform(instance_index);
				dst_positions.push_back(to_vec3f(instance_transform.origin));
				dst_normals->push_back(to_vec3f(voxel::godot::BasisUtility::get_up(instance_transform.basis)));
			}
		} else {
			for (unsigned int instance_index = 0; instance_index < instance_count; ++instance_index) {
				// TODO 优化：首次执行非常慢，之后仍有开销。
				const Transform3D instance_transform = multimesh->get_instance_transform(instance_index);
				dst_positions.push_back(to_vec3f(instance_transform.origin));
			}
		}
	}
}

void VoxelInstancer::get_instance_transforms_local(const Block &block, StdVector<Transform3f> &dst) {
	VOXEL_PROFILE_SCOPE();

	Ref<MultiMesh> multimesh = block.multimesh_instance.get_multimesh();
	VOXEL_ASSERT_RETURN(multimesh.is_valid());
	const unsigned int instance_count = voxel::godot::get_visible_instance_count(**multimesh);

	dst.resize(instance_count);

	for (unsigned int instance_index = 0; instance_index < instance_count; ++instance_index) {
		// TODO 优化：首次执行非常慢，之后仍有开销。
		const Transform3D trans = multimesh->get_instance_transform(instance_index);
		dst[instance_index] = to_transform3f(trans);
	}
}

void VoxelInstancer::remove_instances_by_index(
		Block &block,
		const uint32_t base_block_size,
		Span<const uint32_t> ascending_indices,
		const MMRemovalAction mm_removal_action
) {
#ifdef DEV_ENABLED
	for (unsigned int i = 1; i < ascending_indices.size(); ++i) {
		VOXEL_ASSERT(ascending_indices[i] > ascending_indices[i - 1]);
	}
#endif

	if (block.scene_instances.size() > 0) {
		remove_scene_instances_by_index(block, ascending_indices);

	} else {
		remove_multimesh_instances_by_index(block, base_block_size, ascending_indices, mm_removal_action);
	}
}

void VoxelInstancer::remove_scene_instances_by_index(Block &block, Span<const uint32_t> ascending_indices) {
	VOXEL_PROFILE_SCOPE();

	const unsigned int initial_instance_count = block.scene_instances.size();
	unsigned int instance_count = initial_instance_count;

	unsigned int removal_list_index = ascending_indices.size();
	while (removal_list_index > 0) {
		--removal_list_index;

		const unsigned int instance_index = ascending_indices[removal_list_index];

		SceneInstance instance = block.scene_instances[instance_index];
		ERR_CONTINUE(instance.root == nullptr);

		const unsigned int last_instance_index = --instance_count;

		// TODO 对于场景实例的情况，我们可以使用重叠检查或信号。
		// 分离，这样它就不会尝试更新我们的实例，我们已经在这里做了
		ERR_CONTINUE(instance.component == nullptr);
		// 不使用 detach_as_removed()，
		// 该函数不会将数据块标记为已修改。这可以由调用方完成。
		instance.component->detach();
		instance.root->queue_free();

		SceneInstance moved_instance = block.scene_instances[last_instance_index];
		if (moved_instance.root != instance.root) {
			if (moved_instance.component == nullptr) {
				ERR_PRINT("Instance component should not be null");
			} else {
				moved_instance.component->set_instance_index(instance_index);
			}
			block.scene_instances[instance_index] = moved_instance;
		}
	}

	if (instance_count < initial_instance_count) {
		if (block.scene_instances.size() > 0) {
			block.scene_instances.resize(instance_count);
		}
	}
}

void VoxelInstancer::remove_multimesh_instances_by_index(
		Block &block,
		const uint32_t base_block_size,
		Span<const uint32_t> ascending_indices,
		const MMRemovalAction removal_action
) {
	VOXEL_PROFILE_SCOPE();

	Ref<MultiMesh> multimesh = block.multimesh_instance.get_multimesh();
	VOXEL_ASSERT_RETURN(multimesh.is_valid());

	const int initial_instance_count = voxel::godot::get_visible_instance_count(**multimesh);
	int instance_count = initial_instance_count;

	const int block_size = base_block_size << block.lod_index;

	unsigned int removal_list_index = ascending_indices.size();
	while (removal_list_index > 0) {
		--removal_list_index;

		const unsigned int instance_index = ascending_indices[removal_list_index];

		Transform3D instance_transform;
		if (removal_action.is_valid()) {
			instance_transform = multimesh->get_instance_transform(instance_index);
		}

		// 移除 MultiMesh 实例
		const int last_instance_index = --instance_count;
		// TODO 在多线程模式下这很糟糕！考虑保留一份本地副本...
		const Transform3D last_trans = multimesh->get_instance_transform(last_instance_index);
		// TODO 另外，设置变换会在内部将缓冲区下载回内存（如果尚未这样做），
		// Godot 大概用其按区域更新 VRAM 缓冲区。但它使用的区域是 512 个项目宽，
		// 因此考虑到我们的地形数据块大小，我们通常拥有的项目更少，
		// 与上传整个缓冲区相比几乎没有好处。因此即使我们有自己的缓存来提高我们这边的性能，
		// 同时避免 Godot *需要* 自己的缓存，我们从 Godot 这边也得不到什么好处。
		multimesh->set_instance_transform(instance_index, last_trans);

		// 如果该数据块有刚体，则移除它
		// TODO 对于刚体的情形，我们可以使用重叠检查
		if (block.bodies.size() > 0) {
			VoxelInstancerRigidBody *rb = block.bodies[instance_index];
			// 分离，这样它就不会尝试更新我们的实例，我们已经在这里做了
			rb->detach_and_destroy();

			// 由于我们做了交换移除，更新最后一个刚体索引
			VoxelInstancerRigidBody *moved_rb = block.bodies[last_instance_index];
			if (moved_rb != rb) {
				moved_rb->set_instance_index(instance_index);
				block.bodies[instance_index] = moved_rb;
			}
		}

		if (removal_action.is_valid()) {
			const Transform3D trans(
					instance_transform.basis, instance_transform.origin + Vector3(block.grid_position * block_size)
			);
			removal_action.call(trans);
		}
	}

	if (instance_count < initial_instance_count) {
		// 根据文档，set_instance_count() 会重置数组，因此我们改为只隐藏它们
		multimesh->set_visible_instance_count(instance_count);

		if (block.bodies.size() > 0) {
			block.bodies.resize(instance_count);
		}
	}
}

void VoxelInstancer::do_area_operation(const AABB p_aabb, IAreaOperation &op) {
	do_area_operation(
			Box3i::from_min_max(Vector3i(p_aabb.position.floor()), Vector3i((p_aabb.position + p_aabb.size).ceil())), op
	);
}

void VoxelInstancer::do_area_operation(const Box3i p_voxel_box, IAreaOperation &op) {
	VOXEL_PROFILE_SCOPE();
	ERR_FAIL_COND(_parent == nullptr);
	const int render_block_size = 1 << _parent_mesh_block_size_po2;
	const int data_block_size = 1 << _parent_data_block_size_po2;

	for (unsigned int lod_index = 0; lod_index < _lods.size(); ++lod_index) {
		Lod &lod = _lods[lod_index];

		if (lod.layers.size() == 0) {
			continue;
		}

		const Box3i render_blocks_box = p_voxel_box.downscaled(render_block_size << lod_index);

		bool modified = false;

		for (const int layer_id : lod.layers) {
			const Layer &layer = get_layer(layer_id);
			const StdVector<UniquePtr<Block>> &blocks = _blocks;

			// 迭代与该区域相交的数据块
			const Vector3i bmax = render_blocks_box.position + render_blocks_box.size;
			Vector3i block_pos;
			for (block_pos.z = render_blocks_box.position.z; block_pos.z < bmax.z; ++block_pos.z) {
				for (block_pos.x = render_blocks_box.position.x; block_pos.x < bmax.x; ++block_pos.x) {
					for (block_pos.y = render_blocks_box.position.y; block_pos.y < bmax.y; ++block_pos.y) {
						//
						const auto block_it = layer.blocks.find(block_pos);
						if (block_it == layer.blocks.end()) {
							// 这里没有实例化数据块
							continue;
						}

						Block &block = *blocks[block_it->second];
						const IAreaOperation::Result result = op.execute(block);
						modified = modified | result.modified;
					}
				}
			}
		}

		if (modified) {
			const Box3i data_blocks_box = p_voxel_box.downscaled(data_block_size << lod_index);

			// 所有实例都必须被冻结为已编辑状态。
			// TODO 优化：如有必要，也许可以按项 ID 缩小范围
			data_blocks_box.for_each_cell([&lod](Vector3i data_block_pos) { //
				lod.modified_blocks.insert(data_block_pos);
			});
		}
	}
}

void VoxelInstancer::on_area_edited(Box3i p_voxel_box) {
	remove_floating_instances(p_voxel_box);
}

#ifdef VOXEL_INSTANCER_USE_SPECIALIZED_FLOATING_INSTANCE_REMOVAL_IMPLEMENTATION

void VoxelInstancer::remove_floating_instances(const Box3i p_voxel_box) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_PROFILE_MESSAGE("RemoveFloatingInstances");

	ERR_FAIL_COND(_parent == nullptr);
	const int render_block_size = 1 << _parent_mesh_block_size_po2;
	const int data_block_size = 1 << _parent_data_block_size_po2;

	Ref<VoxelTool> maybe_voxel_tool = _parent->get_voxel_tool();
	ERR_FAIL_COND(maybe_voxel_tool.is_null());
	VoxelTool &voxel_tool = **maybe_voxel_tool;
	voxel_tool.set_channel(VoxelBuffer::CHANNEL_SDF);

	const Transform3D parent_transform = get_global_transform();
	const int base_block_size_po2 = _parent_mesh_block_size_po2;

	for (unsigned int lod_index = 0; lod_index < _lods.size(); ++lod_index) {
		Lod &lod = _lods[lod_index];

		if (lod.layers.size() == 0) {
			continue;
		}

		const Box3i render_blocks_box = p_voxel_box.downscaled(render_block_size << lod_index);

		// 移除悬空实例
		for (const int layer_id : lod.layers) {
			const Layer &layer = get_layer(layer_id);
			const StdVector<UniquePtr<Block>> &blocks = _blocks;
			const int block_size_po2 = base_block_size_po2 + layer.lod_index;

			VoxelInstanceLibraryItem *item = nullptr;
			VoxelInstanceLibraryMultiMeshItem *mm_item = nullptr;
			bool bidirectional = false;
			float sd_threshold = 0.f;
			float sd_offset = 0.f;

			const Vector3i bmax = render_blocks_box.position + render_blocks_box.size;
			Vector3i block_pos;
			for (block_pos.z = render_blocks_box.position.z; block_pos.z < bmax.z; ++block_pos.z) {
				for (block_pos.x = render_blocks_box.position.x; block_pos.x < bmax.x; ++block_pos.x) {
					for (block_pos.y = render_blocks_box.position.y; block_pos.y < bmax.y; ++block_pos.y) {
						//
						const auto block_it = layer.blocks.find(block_pos);
						if (block_it == layer.blocks.end()) {
							// 这里没有实例化数据块
							continue;
						}

						VOXEL_PROFILE_SCOPE_NAMED("Block");

						Block &block = *blocks[block_it->second];

						if (item == nullptr) {
							item = _library->get_item(layer_id);
							sd_threshold = item->get_floating_sdf_threshold();
							sd_offset = item->get_floating_sdf_offset_along_normal();
							Ref<VoxelInstanceGenerator> generator = item->get_generator();
							if (generator.is_valid()) {
								bidirectional = generator->get_random_vertical_flip();
							}
						}

						if (block.scene_instances.size() > 0) {
							remove_floating_scene_instances(
									block,
									parent_transform,
									p_voxel_box,
									voxel_tool,
									block_size_po2,
									sd_threshold,
									sd_offset,
									bidirectional
							);

						} else {
							if (mm_item == nullptr) {
								mm_item = Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(item);
							}

							MMRemovalAction action = get_mm_removal_action(this, mm_item);

							remove_floating_multimesh_instances(
									block,
									parent_transform,
									p_voxel_box,
									voxel_tool,
									block_size_po2,
									sd_threshold,
									sd_offset,
									bidirectional,
									action
							);
						}
					}
				}
			}
		}

		const Box3i data_blocks_box = p_voxel_box.downscaled(data_block_size << lod_index);

		// 所有实例都必须被冻结为已编辑状态。
		// 因为即使它们中没有一个被移除或添加，它们可能生成的地面也
		// 发生了变化，而目前我们不希望在重新加载该区域时生成意外的实例。
		data_blocks_box.for_each_cell([&lod](Vector3i data_block_pos) { //
			lod.modified_blocks.insert(data_block_pos);
		});
	}
}

void VoxelInstancer::remove_floating_multimesh_instances(
		Block &block,
		const Transform3D &parent_transform,
		const Box3i p_voxel_box,
		const VoxelTool &voxel_tool,
		const int block_size_po2,
		const float sd_threshold,
		const float sd_offset,
		const bool bidirectional,
		const MMRemovalAction removal_action
) {
	if (!block.multimesh_instance.is_valid()) {
		// 空数据块
		return;
	}

	Ref<MultiMesh> multimesh = block.multimesh_instance.get_multimesh();
	ERR_FAIL_COND(multimesh.is_null());

	const int initial_instance_count = voxel::godot::get_visible_instance_count(**multimesh);
	int instance_count = initial_instance_count;

	// const Transform3D block_global_transform =
	// 		Transform3D(parent_transform.basis, parent_transform.xform(block.grid_position << block_size_po2));
	const Vector3i block_origin_in_voxels = block.grid_position << block_size_po2;

	// 让我们逐个检查所有实例
	// 注意：我们必须反复查询 VisualServer 这一点相当糟糕。
	// - 在多线程模式下，我们可能必须与其线程同步
	// - 执行哈希映射 RID 查找来检查 `RID_Owner::id_map`
	for (int instance_index = 0; instance_index < instance_count; ++instance_index) {
		// TODO 优化：在多线程模式下这很糟糕！考虑保留一份本地副本...
		const Transform3D instance_transform = multimesh->get_instance_transform(instance_index);
		const Vector3i voxel_pos(math::floor_to_int(instance_transform.origin) + block_origin_in_voxels);

		if (!p_voxel_box.contains(voxel_pos)) {
			continue;
		}

		if (detect_ground(
					instance_transform.origin,
					voxel::godot::BasisUtility::get_up(instance_transform.basis),
					Vector3(block_origin_in_voxels),
					sd_threshold,
					sd_offset,
					bidirectional,
					voxel_tool
			)) {
			continue;
		}

		// 移除 MultiMesh 实例
		const int last_instance_index = --instance_count;
		// TODO 优化：首次执行非常慢，之后仍有开销。
		const Transform3D last_trans = multimesh->get_instance_transform(last_instance_index);
		// TODO 另外，设置变换会在内部将缓冲区下载回内存（如果尚未这样做），
		// Godot 大概用其按区域更新 VRAM 缓冲区。但它使用的区域是 512 个项目宽，
		// 因此考虑到我们的地形数据块大小，我们通常拥有的项目更少，
		// 与上传整个缓冲区相比几乎没有好处。因此即使我们有自己的缓存来提高我们这边的性能，
		// 同时避免 Godot *需要* 自己的缓存，我们从 Godot 这边也得不到什么好处。
		multimesh->set_instance_transform(instance_index, last_trans);

		// 如果该数据块有刚体，则移除它
		// TODO 对于刚体的情形，我们可以使用重叠检查
		if (block.bodies.size() > 0) {
			VoxelInstancerRigidBody *rb = block.bodies[instance_index];
			// 分离，这样它就不会尝试更新我们的实例，我们已经在这里做了
			rb->detach_and_destroy();

			// 由于我们做了交换移除，更新最后一个刚体索引
			VoxelInstancerRigidBody *moved_rb = block.bodies[last_instance_index];
			if (moved_rb != rb) {
				moved_rb->set_instance_index(instance_index);
				block.bodies[instance_index] = moved_rb;
			}
		}

		--instance_index;

		if (removal_action.is_valid()) {
			const Transform3D trans(
					instance_transform.basis, instance_transform.origin + Vector3(block_origin_in_voxels)
			);
			removal_action.call(trans);
		}

		// DEBUG
		// Ref<CubeMesh> cm;
		// cm.instance();
		// cm->set_size(Vector3(0.5, 0.5, 0.5));
		// MeshInstance *mi = memnew(MeshInstance);
		// mi->set_mesh(cm);
		// mi->set_transform(get_global_transform() *
		// 				  (Transform(Basis(), (block_pos << layer->lod_index).to_vec3()) * t));
		// add_child(mi);
	}

	if (instance_count < initial_instance_count) {
		// 根据文档，set_instance_count() 会重置数组，因此我们改为只隐藏它们
		multimesh->set_visible_instance_count(instance_count);

		if (block.bodies.size() > 0) {
			block.bodies.resize(instance_count);
		}

		// Array args;
		// args.push_back(instance_count);
		// args.push_back(initial_instance_count);
		// args.push_back(block_pos.to_vec3());
		// args.push_back(layer->lod_index);
		// args.push_back(multimesh->get_instance_count());
		// print_line(
		// 		String("Hiding instances from {0} to {1}. P: {2}, lod: {3}, total: {4}").format(args));
	}
}

void VoxelInstancer::remove_floating_scene_instances(
		Block &block,
		const Transform3D &parent_transform,
		const Box3i p_voxel_box,
		const VoxelTool &voxel_tool,
		const int block_size_po2,
		const float sd_threshold,
		const float sd_offset,
		const bool bidirectional
) {
	const unsigned int initial_instance_count = block.scene_instances.size();
	unsigned int instance_count = initial_instance_count;

	const Transform3D block_global_transform =
			Transform3D(parent_transform.basis, parent_transform.xform(block.grid_position << block_size_po2));

	// 让我们逐个检查所有实例
	// 注意：我们必须反复查询 VisualServer 这一点相当糟糕。
	// - 在多线程模式下，我们可能必须与其线程同步
	// - 执行哈希映射 RID 查找来检查 `RID_Owner::id_map`
	for (unsigned int instance_index = 0; instance_index < instance_count; ++instance_index) {
		SceneInstance instance = block.scene_instances[instance_index];
		ERR_CONTINUE(instance.root == nullptr);
		const Transform3D scene_transform = instance.root->get_transform();
		const Vector3i voxel_pos(math::floor_to_int(scene_transform.origin + block_global_transform.origin));

		if (!p_voxel_box.contains(voxel_pos)) {
			continue;
		}

		if (detect_ground(
					scene_transform.origin,
					voxel::godot::BasisUtility::get_up(scene_transform.basis),
					Vector3(), // 小技巧，场景已经在地形空间中
					sd_threshold,
					sd_offset,
					bidirectional,
					voxel_tool
			)) {
			continue;
		}

		// 移除 MultiMesh 实例
		const unsigned int last_instance_index = --instance_count;

		// TODO 对于场景实例的情况，我们可以使用重叠检查或信号。
		// 分离，这样它就不会尝试更新我们的实例，我们已经在这里做了
		ERR_CONTINUE(instance.component == nullptr);
		// 不使用 detach_as_removed()，
		// 该函数不会将数据块标记为已修改。这可以由调用方完成。
		instance.component->detach();
		instance.root->queue_free();

		SceneInstance moved_instance = block.scene_instances[last_instance_index];
		if (moved_instance.root != instance.root) {
			if (moved_instance.component == nullptr) {
				ERR_PRINT("Instance component should not be null");
			} else {
				moved_instance.component->set_instance_index(instance_index);
			}
			block.scene_instances[instance_index] = moved_instance;
		}

		--instance_index;
	}

	if (instance_count < initial_instance_count) {
		if (block.scene_instances.size() > 0) {
			block.scene_instances.resize(instance_count);
		}
	}
}

#else // VOXEL_INSTANCER_USE_SPECIALIZED_FLOATING_INSTANCE_REMOVAL_IMPLEMENTATION

void VoxelInstancer::remove_floating_instances(const Box3i voxel_box) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_PROFILE_MESSAGE("RemoveFloatingInstances");

	class RemoveFloatingInstances : public IAreaOperation {
	private:
		VoxelInstancer &_instancer;
		const unsigned int _base_block_size;
		VoxelTool &_voxel_tool;
		Box3i _voxel_box;

		MMRemovalAction _mm_removal_action;
		uint32_t _mm_removal_action_item_id = VoxelInstanceLibrary::MAX_ID;
		float _sd_threshold = 0.f;
		float _offset_along_normal = 0.f;
		bool _bidirectional = false;

	public:
		RemoveFloatingInstances(
				VoxelInstancer &instancer,
				const unsigned int base_block_size,
				VoxelTool &p_voxel_tool,
				const Box3i p_voxel_box
		) :
				_instancer(instancer),
				_base_block_size(base_block_size),
				_voxel_tool(p_voxel_tool),
				_voxel_box(p_voxel_box) //
		{}

		Result execute(Block &block) override {
			VOXEL_PROFILE_SCOPE();

			// TODO 临时分配器的候选
			// TODO 如果我们有自己的缓存，可能就完全不需要分配了
			StdVector<Vector3f> instance_positions;
			StdVector<Vector3f> instance_normals;
			get_instance_positions_local(block, _base_block_size, instance_positions, &instance_normals);

			try_update_item_cache(block.layer_id);

			// TODO 如果我们使用比逐个体素查询更优化的方式，就可以只使用本地位置
			const Vector3i block_origin_i = block.grid_position * (_base_block_size << block.lod_index);
			const Vector3 block_origin(block_origin_i);

			const Box3f box_local = Box3f::from_min_max(
					to_vec3f(_voxel_box.position - block_origin_i),
					to_vec3f(_voxel_box.position - block_origin_i + _voxel_box.size)
			);

			// TODO 临时分配器的候选
			StdVector<uint32_t> instances_to_remove;
			for (unsigned int instance_index = 0; instance_index < instance_positions.size(); ++instance_index) {
				const Vector3f instance_pos = instance_positions[instance_index];
				if (!box_local.contains(instance_pos)) {
					continue;
				}

				if (!detect_ground(
							to_vec3(instance_positions[instance_index]),
							to_vec3(instance_normals[instance_index]),
							block_origin,
							_sd_threshold,
							_offset_along_normal,
							_bidirectional,
							_voxel_tool
					)) {
					instances_to_remove.push_back(instance_index);
				}
			}

			if (instances_to_remove.size() == 0) {
				return { false };
			}

			remove_instances_by_index(block, _base_block_size, to_span(instances_to_remove), _mm_removal_action);

			return { true };
		}

	private:
		void try_update_item_cache(const uint32_t item_id) {
			if (item_id == _mm_removal_action_item_id) {
				return;
			}

			// 重新缓存项信息

			_mm_removal_action = MMRemovalAction();
			_mm_removal_action_item_id = item_id;
			_sd_threshold = 0.f;
			_offset_along_normal = 0.f;

			if (_instancer._library.is_null()) {
				return;
			}
			VoxelInstanceLibraryItem *item = _instancer._library->get_item(item_id);
			if (item == nullptr) {
				return;
			}
			_sd_threshold = item->get_floating_sdf_threshold();
			_offset_along_normal = item->get_floating_sdf_offset_along_normal();

			Ref<VoxelInstanceGenerator> instance_generator = item->get_generator();
			_bidirectional = instance_generator.is_valid() ? instance_generator->get_random_vertical_flip() : false;

			VoxelInstanceLibraryMultiMeshItem *mm_item = Object::cast_to<VoxelInstanceLibraryMultiMeshItem>(item);
			if (mm_item == nullptr) {
				return;
			}
			_mm_removal_action = get_mm_removal_action(&_instancer, mm_item);
		}
	};

	Ref<VoxelTool> maybe_voxel_tool = _parent->get_voxel_tool();
	VOXEL_ASSERT_RETURN(maybe_voxel_tool.is_valid());

	RemoveFloatingInstances op(*this, 1 << _parent_mesh_block_size_po2, **maybe_voxel_tool, voxel_box);

	do_area_operation(voxel_box, op);
}

#endif

// 当用户在其仍附着在地面上时销毁或移除刚体节点，会调用此函数
void VoxelInstancer::on_body_removed(
		Vector3i data_block_position,
		unsigned int render_block_index,
		unsigned int instance_index
) {
	VOXEL_PRINT_VERBOSE(format("on_body_removed from block {}", render_block_index));

	Block &block = *_blocks[render_block_index];

	if (instance_index >= block.bodies.size()) {
		int instance_count = -1;
		if (block.multimesh_instance.is_valid()) {
			Ref<MultiMesh> multimesh = block.multimesh_instance.get_multimesh();
			instance_count = voxel::godot::get_visible_instance_count(**multimesh);
		}
		VOXEL_PRINT_ERROR(
				format("Can't remove instance with index {} (bodies: {}, instances: {})",
					   instance_index,
					   block.bodies.size(),
					   instance_count)
		);
		return;
	}

	if (block.multimesh_instance.is_valid()) {
		// 移除 multimesh 实例

		Ref<MultiMesh> multimesh = block.multimesh_instance.get_multimesh();
		ERR_FAIL_COND(multimesh.is_null());

		{
			Ref<VoxelInstanceLibraryItem> item = _library->get_item(block.layer_id);
			Ref<VoxelInstanceLibraryMultiMeshItem> mm_item = item;
			MMRemovalAction action = get_mm_removal_action(this, mm_item.ptr());
			if (action.is_valid()) {
				// TODO 优化：首次执行非常慢，之后仍有开销。
				const Transform3D ltrans = multimesh->get_instance_transform(instance_index);
				const Vector3i block_origin_in_voxels = data_block_position
						<< (_parent_mesh_block_size_po2 + block.lod_index);
				const Transform3D trans(ltrans.basis, ltrans.origin + Vector3(block_origin_in_voxels));
				action.call(trans);
			}
		}

		int visible_count = voxel::godot::get_visible_instance_count(**multimesh);
		ERR_FAIL_COND(static_cast<int>(instance_index) >= visible_count);

		--visible_count;
		// 交换移除
		// TODO 优化：首次执行非常慢，之后仍有开销。
		const Transform3D last_trans = multimesh->get_instance_transform(visible_count);
		multimesh->set_instance_transform(instance_index, last_trans);
		multimesh->set_visible_instance_count(visible_count);
	}

	// 注销该刚体
	unsigned int body_count = block.bodies.size();
	const unsigned int last_instance_index = --body_count;
	VoxelInstancerRigidBody *moved_body = block.bodies[last_instance_index];
	if (instance_index != last_instance_index) {
		// 由于我们做了交换移除，更新最后一个刚体索引
		moved_body->set_instance_index(instance_index);
		block.bodies[instance_index] = moved_body;
	}
	block.bodies.resize(body_count);

	// 将数据块标记为已修改
	const Layer &layer = get_layer(block.layer_id);
	Lod &lod = _lods[layer.lod_index];
	lod.modified_blocks.insert(data_block_position);
}

void VoxelInstancer::on_scene_instance_removed(
		Vector3i data_block_position,
		unsigned int render_block_index,
		unsigned int instance_index
) {
	Block &block = *_blocks[render_block_index];
	VOXEL_ASSERT_RETURN(instance_index < block.scene_instances.size());

	// 注销该场景实例
	unsigned int instance_count = block.scene_instances.size();
	const unsigned int last_instance_index = --instance_count;
	SceneInstance moved_instance = block.scene_instances[last_instance_index];
	if (instance_index != last_instance_index) {
		// 由于我们做了交换移除，更新最后一个实例索引
		ERR_FAIL_COND(moved_instance.component == nullptr);
		moved_instance.component->set_instance_index(instance_index);
		block.scene_instances[instance_index] = moved_instance;
	}
	block.scene_instances.resize(instance_count);

	// 将数据块标记为已修改
	const Layer &layer = get_layer(block.layer_id);
	Lod &lod = _lods[layer.lod_index];
	lod.modified_blocks.insert(data_block_position);
}

void VoxelInstancer::on_scene_instance_modified(Vector3i data_block_position, unsigned int render_block_index) {
	Block &block = *_blocks[render_block_index];

	// 将数据块标记为已修改
	const Layer &layer = get_layer(block.layer_id);
	Lod &lod = _lods[layer.lod_index];
	lod.modified_blocks.insert(data_block_position);
}

void VoxelInstancer::on_data_block_saved(Vector3i data_grid_position, unsigned int lod_index) {
	if (lod_index >= _lods.size()) {
		return;
	}
	Lod &lod = _lods[lod_index];
	if (lod.quick_reload_cache != nullptr) {
		MutexLock mlock(lod.quick_reload_cache->mutex);
		lod.quick_reload_cache->map.erase(data_grid_position);
	}
}

void VoxelInstancer::set_mesh_block_size_po2(unsigned int p_mesh_block_size_po2) {
	_parent_mesh_block_size_po2 = p_mesh_block_size_po2;
}

void VoxelInstancer::set_data_block_size_po2(unsigned int p_data_block_size_po2) {
	_parent_data_block_size_po2 = p_data_block_size_po2;
}

int VoxelInstancer::get_library_item_id_from_render_block_index(unsigned int render_block_index) const {
	VOXEL_ASSERT_RETURN_V(render_block_index < _blocks.size(), -1);
	Block &block = *_blocks[render_block_index];
	return block.layer_id;
}

// 调试相关

int VoxelInstancer::debug_get_block_count() const {
	return _blocks.size();
}

void VoxelInstancer::debug_get_instance_counts(StdUnorderedMap<uint32_t, uint32_t> &counts_per_layer) const {
	VOXEL_PROFILE_SCOPE();

	counts_per_layer.clear();

	for (auto it = _blocks.begin(); it != _blocks.end(); ++it) {
		const Block &block = **it;

		uint32_t count = block.scene_instances.size();

		if (block.multimesh_instance.is_valid()) {
			Ref<MultiMesh> multimesh = block.multimesh_instance.get_multimesh();
			VOXEL_ASSERT_CONTINUE(multimesh.is_valid());

			count += voxel::godot::get_visible_instance_count(**multimesh);
		}

		counts_per_layer[block.layer_id] += count;
	}
}

Dictionary VoxelInstancer::_b_debug_get_instance_counts() const {
	Dictionary d;
	StdUnorderedMap<uint32_t, uint32_t> map;
	debug_get_instance_counts(map);
	for (auto it = map.begin(); it != map.end(); ++it) {
		d[it->first] = it->second;
	}
	return d;
}

void VoxelInstancer::debug_dump_as_scene(String fpath) const {
	Node *root = debug_dump_as_nodes();
	ERR_FAIL_COND(root == nullptr);

	voxel::godot::set_nodes_owner_except_root(root, root);

	Ref<PackedScene> packed_scene;
	packed_scene.instantiate();
	const Error pack_result = packed_scene->pack(root);
	memdelete(root);
	ERR_FAIL_COND(pack_result != OK);

	const Error save_result = voxel::godot::save_resource(packed_scene, fpath, ResourceSaver::FLAG_BUNDLE_RESOURCES);
	ERR_FAIL_COND(save_result != OK);
}

Node *VoxelInstancer::debug_dump_as_nodes() const {
	return convert_to_nodes(NODE_CONVERSION_DUPLICATE_MESHES | NODE_CONVERSION_DUPLICATE_MULTIMESHES);
}

Node3D *VoxelInstancer::convert_to_nodes(const uint32_t flags) const {
	VOXEL_ASSERT_RETURN_V(_library.is_valid(), nullptr);

	const unsigned int mesh_block_size = 1 << _parent_mesh_block_size_po2;

	Node3D *root = memnew(Node3D);
	root->set_transform(get_transform());
	root->set_name("VoxelInstancerRoot");

	StdUnorderedMap<Ref<Mesh>, Ref<Mesh>> mesh_copies;

	// 遍历每一层
	for (auto layer_it = _layers.begin(); layer_it != _layers.end(); ++layer_it) {
		const Layer &layer = layer_it->second;
		const int lod_block_size = mesh_block_size << layer.lod_index;

		Node3D *layer_node = memnew(Node3D);
		layer_node->set_name(String("Layer{0}").format(varray(layer_it->first)));
		root->add_child(layer_node);

		Ref<Material> mat_override;
		const VoxelInstanceLibraryItem *item_base = _library->get_item_const(layer_it->first);
		const VoxelInstanceLibraryMultiMeshItem *mm_item =
				Object::cast_to<const VoxelInstanceLibraryMultiMeshItem>(item_base);

		if (mm_item != nullptr) {
			if ((flags & NODE_CONVERSION_INCLUDE_MATERIAL_OVERRIDES) != 0) {
				mat_override = mm_item->get_material_override();
			}
		}

		// 遍历层中的每个数据块
		for (auto block_it = layer.blocks.begin(); block_it != layer.blocks.end(); ++block_it) {
			const unsigned int block_index = block_it->second;
			CRASH_COND(block_index >= _blocks.size());
			const Block &block = *_blocks[block_index];

			if (block.multimesh_instance.is_valid()) {
				const Transform3D block_local_transform(Basis(), Vector3(block.grid_position * lod_block_size));

				Ref<MultiMesh> src_multimesh = block.multimesh_instance.get_multimesh();
				ERR_CONTINUE(src_multimesh.is_null());
				Ref<Mesh> src_mesh = src_multimesh->get_mesh();
				ERR_CONTINUE(src_mesh.is_null());

				Ref<Mesh> mesh;
				if ((flags & NODE_CONVERSION_DUPLICATE_MESHES) != 0) {
					// 复制网格可能是必要的，因为即使使用 `FLAG_BUNDLE_RESOURCES`，
					// 保存到 PackedScene 时它们也常常不会被保存
					auto mesh_copy_it = mesh_copies.find(src_mesh);
					Ref<Mesh> mesh_copy;
					if (mesh_copy_it == mesh_copies.end()) {
						mesh_copy = src_mesh->duplicate();
						mesh_copies.insert({ src_mesh, mesh_copy });
					} else {
						mesh_copy = mesh_copy_it->second;
					}
					mesh = mesh_copy;
				} else {
					mesh = src_mesh;
				}

				Ref<MultiMesh> multimesh;
				if ((flags & NODE_CONVERSION_DUPLICATE_MULTIMESHES) != 0) {
					Ref<MultiMesh> multimesh_copy = src_multimesh->duplicate();
					multimesh_copy->set_mesh(mesh);
				} else {
					multimesh = src_multimesh;
				}

				MultiMeshInstance3D *mmi = memnew(MultiMeshInstance3D);
				mmi->set_multimesh(multimesh);
				mmi->set_transform(block_local_transform);
				mmi->set_material_override(mat_override);

				if (mm_item != nullptr) {
					mmi->set_cast_shadows_setting(
							GeometryInstance3D::ShadowCastingSetting(mm_item->get_cast_shadows_setting())
					);
					mmi->set_layer_mask(mm_item->get_render_layer());
					mmi->set_gi_mode(mm_item->get_gi_mode());
				}

				layer_node->add_child(mmi);
			}

			// TODO 也导出场景实例
		}
	}

	return root;
}

void VoxelInstancer::debug_set_draw_enabled(bool enabled) {
#ifdef TOOLS_ENABLED
	_gizmos_enabled = enabled;
	if (_gizmos_enabled) {
		if (is_inside_tree()) {
			_debug_renderer.set_world(is_visible_in_tree() ? *get_world_3d() : nullptr);
		}
	} else {
		_debug_renderer.clear();
	}
#endif
}

bool VoxelInstancer::debug_is_draw_enabled() const {
#ifdef TOOLS_ENABLED
	return _gizmos_enabled;
#else
	return false;
#endif
}

void VoxelInstancer::debug_set_draw_flag(DebugDrawFlag flag_index, bool enabled) {
#ifdef TOOLS_ENABLED
	ERR_FAIL_INDEX(flag_index, DEBUG_DRAW_FLAGS_COUNT);
	if (enabled) {
		_debug_draw_flags |= (1 << flag_index);
	} else {
		_debug_draw_flags &= ~(1 << flag_index);
	}
#endif
}

bool VoxelInstancer::debug_get_draw_flag(DebugDrawFlag flag_index) const {
#ifdef TOOLS_ENABLED
	ERR_FAIL_INDEX_V(flag_index, DEBUG_DRAW_FLAGS_COUNT, false);
	return (_debug_draw_flags & (1 << flag_index)) != 0;
#else
	return false;
#endif
}

Dictionary VoxelInstancer::debug_get_block_infos(const Vector3 world_position, const int item_id) {
#ifndef TOOLS_ENABLED
	return Dictionary();
#else
	Dictionary dict;

	auto layer_it = _layers.find(item_id);
	if (layer_it == _layers.end()) {
		VOXEL_PRINT_ERROR("Invalid item id");
		return Dictionary();
	}
	const Layer &layer = layer_it->second;

	dict["lod_index"] = layer.lod_index;

	const int block_shift = _parent_mesh_block_size_po2 + layer.lod_index;
	const Vector3i bpos = Vector3i(world_position.floor()) >> block_shift;
	const Vector3i block_origin_in_voxels = bpos << block_shift;
	const int block_size_in_voxels = 1 << block_shift;

	dict["aabb"] =
			AABB(block_origin_in_voxels, Vector3(block_size_in_voxels, block_size_in_voxels, block_size_in_voxels));

	dict["grid_position"] = bpos;

	auto block_it = layer.blocks.find(bpos);
	const bool allocated = (block_it != layer.blocks.end());
	dict["allocated"] = allocated;
	if (!allocated) {
		return dict;
	}

	const unsigned int block_index = block_it->second;
	const Block *block = _blocks[block_index].get();
	VOXEL_ASSERT(block != nullptr);

	dict["mesh_lod"] = block->current_mesh_lod;

	Array instances_array;
	Array bodies_array;
	Array scenes_array;

	if (block->multimesh_instance.is_valid()) {
		Ref<MultiMesh> mm = block->multimesh_instance.get_multimesh();

		if (mm.is_valid()) {
			const unsigned int count = voxel::godot::get_visible_instance_count(**mm);
			instances_array.resize(count);

			for (unsigned int instance_index = 0; instance_index < count; ++instance_index) {
				// TODO 优化：首次执行非常慢，之后仍有开销。
				const Transform3D instance_transform = mm->get_instance_transform(instance_index);
				instances_array[instance_index] = instance_transform;
			}
		}
	}

	for (const VoxelInstancerRigidBody *body : block->bodies) {
		bodies_array.push_back(body);
	}

	for (const SceneInstance &scene : block->scene_instances) {
		scenes_array.push_back(scene.root);
	}

	dict["instances"] = instances_array;
	dict["bodies"] = bodies_array;
	dict["scenes"] = scenes_array;

	return dict;
#endif
}

#ifdef TOOLS_ENABLED

#if defined(VOXEL_GODOT)
PackedStringArray VoxelInstancer::get_configuration_warnings() const {
	PackedStringArray warnings;
	get_configuration_warnings(warnings);
	return warnings;
}
#endif

void VoxelInstancer::get_configuration_warnings(PackedStringArray &warnings) const {
	if (_parent == nullptr) {
		warnings.append(
				VOXEL_TTR("This node must be child of a {0}.").format(varray(VoxelLodTerrain::get_class_static()))
		);
	}
	if (_library.is_null()) {
		warnings.append(VOXEL_TTR("No library is assigned. A {0} is needed to spawn items.")
								.format(varray(VoxelInstanceLibrary::get_class_static())));
	} else if (_library->get_item_count() == 0) {
		warnings.append(VOXEL_TTR("The assigned library is empty. Add items to it so they can be spawned."));

	} else {
		voxel::godot::get_resource_configuration_warnings(**_library, warnings, []() { return "library: "; });

		VoxelTerrain *vt = Object::cast_to<VoxelTerrain>(_parent);
		if (vt != nullptr) {
			_library->for_each_item([&warnings](int id, const VoxelInstanceLibraryItem &item) {
				const int lod_index = item.get_lod_index();
				if (lod_index > 0) {
					warnings.append(
							String(VOXEL_TTR("library: item {0}: LOD index is set to higher than 0 ({1}), but the parent "
										  "terrain doesn't have LOD support. Instances will not be generated."))
									.format(varray(id, lod_index))
					);
				}
			});
		}

		if (_parent != nullptr) {
			Ref<VoxelStream> stream = _parent->get_stream();
			if (stream.is_valid() && !stream->supports_instance_blocks()) {
				const int persistent_id = _library->find_item([](const VoxelInstanceLibraryItem &item) { //
					return item.is_persistent();
				});
				if (persistent_id != -1) {
					warnings.append(String(VOXEL_TTR("Library contains at least one persistent item (ID {0}), but the "
												  "current stream ({1}) does not support saving instances."))
											.format(varray(persistent_id, stream->get_class())));
				}
			}
		}
	}
}

#endif // TOOLS_ENABLED

void VoxelInstancer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_library", "library"), &VoxelInstancer::set_library);
	ClassDB::bind_method(D_METHOD("get_library"), &VoxelInstancer::get_library);

	ClassDB::bind_method(D_METHOD("set_up_mode", "mode"), &VoxelInstancer::set_up_mode);
	ClassDB::bind_method(D_METHOD("get_up_mode"), &VoxelInstancer::get_up_mode);

	ClassDB::bind_method(
			D_METHOD("remove_instances_in_sphere", "center", "radius"), &VoxelInstancer::remove_instances_in_sphere
	);

	ClassDB::bind_method(D_METHOD("debug_get_block_count"), &VoxelInstancer::debug_get_block_count);
	ClassDB::bind_method(D_METHOD("debug_get_instance_counts"), &VoxelInstancer::_b_debug_get_instance_counts);
	ClassDB::bind_method(D_METHOD("debug_dump_as_scene", "fpath"), &VoxelInstancer::debug_dump_as_scene);
	ClassDB::bind_method(D_METHOD("debug_set_draw_enabled", "enabled"), &VoxelInstancer::debug_set_draw_enabled);
	ClassDB::bind_method(D_METHOD("debug_is_draw_enabled"), &VoxelInstancer::debug_is_draw_enabled);
	ClassDB::bind_method(D_METHOD("debug_set_draw_flag", "flag", "enabled"), &VoxelInstancer::debug_set_draw_flag);
	ClassDB::bind_method(D_METHOD("debug_get_draw_flag", "flag"), &VoxelInstancer::debug_get_draw_flag);
	ClassDB::bind_method(
			D_METHOD("debug_get_block_infos", "world_position", "item_id"), &VoxelInstancer::debug_get_block_infos
	);

	ClassDB::bind_method(
			D_METHOD("get_mesh_lod_update_budget_microseconds"),
			&VoxelInstancer::get_mesh_lod_update_budget_microseconds
	);
	ClassDB::bind_method(
			D_METHOD("set_mesh_lod_update_budget_microseconds", "micros"),
			&VoxelInstancer::set_mesh_lod_update_budget_microseconds
	);

	ClassDB::bind_method(
			D_METHOD("get_collision_update_budget_microseconds"),
			&VoxelInstancer::get_collision_update_budget_microseconds
	);
	ClassDB::bind_method(
			D_METHOD("set_collision_update_budget_microseconds", "micros"),
			&VoxelInstancer::set_collision_update_budget_microseconds
	);

	ClassDB::bind_method(D_METHOD("set_fading_enabled", "enabled"), &VoxelInstancer::set_fading_enabled);
	ClassDB::bind_method(D_METHOD("get_fading_enabled"), &VoxelInstancer::get_fading_enabled);

	ClassDB::bind_method(D_METHOD("set_fading_duration", "duration"), &VoxelInstancer::set_fading_duration);
	ClassDB::bind_method(D_METHOD("get_fading_duration"), &VoxelInstancer::get_fading_duration);

	ADD_PROPERTY(
			PropertyInfo(
					Variant::OBJECT, "library", PROPERTY_HINT_RESOURCE_TYPE, VoxelInstanceLibrary::get_class_static()
			),
			"set_library",
			"get_library"
	);
	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "up_mode", PROPERTY_HINT_ENUM, "PositiveY,Sphere"), "set_up_mode", "get_up_mode"
	);

	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "mesh_lod_update_budget_microseconds"),
			"set_mesh_lod_update_budget_microseconds",
			"get_mesh_lod_update_budget_microseconds"
	);

	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "collision_update_budget_microseconds"),
			"set_collision_update_budget_microseconds",
			"get_collision_update_budget_microseconds"
	);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fading_enabled"), "set_fading_enabled", "get_fading_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fading_duration"), "set_fading_duration", "get_fading_duration");

	BIND_CONSTANT(MAX_LOD);

	BIND_ENUM_CONSTANT(UP_MODE_POSITIVE_Y);
	BIND_ENUM_CONSTANT(UP_MODE_SPHERE);

	BIND_ENUM_CONSTANT(DEBUG_DRAW_ALL_BLOCKS);
	BIND_ENUM_CONSTANT(DEBUG_DRAW_EDITED_BLOCKS);
	BIND_ENUM_CONSTANT(DEBUG_DRAW_FLAGS_COUNT);
}

} // namespace voxel
