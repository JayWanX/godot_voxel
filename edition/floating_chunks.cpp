#include "floating_chunks.h"
#include "../constants/voxel_string_names.h"
#include "../storage/voxel_buffer.h"
#include "../util/godot/classes/array_mesh.h"
#include "../util/godot/classes/collision_shape_3d.h"
#include "../util/godot/classes/convex_polygon_shape_3d.h"
#include "../util/godot/classes/mesh_instance_3d.h"
#include "../util/godot/classes/rendering_server.h"
#include "../util/godot/classes/rigid_body_3d.h"
#include "../util/godot/classes/shader.h"
#include "../util/godot/classes/shader_material.h"
#include "../util/godot/classes/timer.h"
#include "../util/island_finder.h"
#include "../util/profiling.h"
#include "voxel_tool.h"

#ifdef VOXEL_GODOT
#include "../util/godot/core/callable_mp.h"
#endif

namespace voxel {

void box_propagate_ccl(Span<uint8_t> cells, const Vector3i size) {
	VOXEL_PROFILE_SCOPE();

	// 以 3x3x3 的模式将非零格子向零格子传播。
	// 用于由连通分量标记（Connected-Component-Labelling）产生的网格。

	// Z
	{
		VOXEL_PROFILE_SCOPE_NAMED("Z");
		Vector3i pos;
		const int dz = size.x * size.y;
		unsigned int i = 0;
		for (pos.x = 0; pos.x < size.x; ++pos.x) {
			for (pos.y = 0; pos.y < size.y; ++pos.y) {
				// 注意，边界格子不处理。不仅因为工作量更大，还因为那样可能会
				// 让标签触及边缘，而这之后会被解释为不是岛屿。
				pos.z = 2;
				i = Vector3iUtil::get_zxy_index(pos, size);
				for (; pos.z < size.z - 2; ++pos.z, i += dz) {
					const uint8_t c = cells[i];
					if (c != 0) {
						if (cells[i - dz] == 0) {
							cells[i - dz] = c;
						}
						if (cells[i + dz] == 0) {
							cells[i + dz] = c;
							// 跳过下一个格子，否则会导致无限传播
							i += dz;
							++pos.z;
						}
					}
				}
			}
		}
	}

	// X
	{
		VOXEL_PROFILE_SCOPE_NAMED("X");
		Vector3i pos;
		const int dx = size.y;
		unsigned int i = 0;
		for (pos.z = 0; pos.z < size.z; ++pos.z) {
			for (pos.y = 0; pos.y < size.y; ++pos.y) {
				pos.x = 2;
				i = Vector3iUtil::get_zxy_index(pos, size);
				for (; pos.x < size.x - 2; ++pos.x, i += dx) {
					const uint8_t c = cells[i];
					if (c != 0) {
						if (cells[i - dx] == 0) {
							cells[i - dx] = c;
						}
						if (cells[i + dx] == 0) {
							cells[i + dx] = c;
							i += dx;
							++pos.x;
						}
					}
				}
			}
		}
	}

	// Y
	{
		VOXEL_PROFILE_SCOPE_NAMED("Y");
		Vector3i pos;
		const int dy = 1;
		unsigned int i = 0;
		for (pos.z = 0; pos.z < size.z; ++pos.z) {
			for (pos.x = 0; pos.x < size.x; ++pos.x) {
				pos.y = 2;
				i = Vector3iUtil::get_zxy_index(pos, size);
				for (; pos.y < size.y - 2; ++pos.y, i += dy) {
					const uint8_t c = cells[i];
					if (c != 0) {
						if (cells[i - dy] == 0) {
							cells[i - dy] = c;
						}
						if (cells[i + dy] == 0) {
							cells[i + dy] = c;
							i += dy;
							++pos.y;
						}
					}
				}
			}
		}
	}
}

// 将漂浮的体素块转换为刚体：
// 检测盒子内相互连接的不同体素组。完全包含在盒子内的每个组都会从
// 源体积中移除，并转换为刚体。
// 这只是一种实现方式，我不知道它是否是最好的（很少存在最好的方式）
// 所以未来或许可以探索其它性能更好的方法
Array separate_floating_chunks(
		VoxelTool &voxel_tool,
		Box3i world_box,
		Node *parent_node,
		Transform3D terrain_transform,
		Ref<VoxelMesher> mesher,
		Array materials
) {
	VOXEL_PROFILE_SCOPE();

	// 检查
	ERR_FAIL_COND_V(mesher.is_null(), Array());
	ERR_FAIL_COND_V(parent_node == nullptr, Array());

	// 复制源数据

	// TODO 不要假定通道，目前它是为平滑地形硬编码的
	static const int channels_mask = (1 << VoxelBuffer::CHANNEL_SDF);
	static const VoxelBuffer::ChannelId main_channel = VoxelBuffer::CHANNEL_SDF;

	VoxelBuffer source_copy_buffer(VoxelBuffer::ALLOCATOR_POOL);
	{
		VOXEL_PROFILE_SCOPE_NAMED("Copy");
		source_copy_buffer.create(world_box.size);
		voxel_tool.copy(world_box.position, source_copy_buffer, channels_mask, false);
	}

	// 标记不同的体素组

	// TODO 可作为临时分配器的候选
	static thread_local StdVector<uint8_t> ccl_output;
	ccl_output.resize(Vector3iUtil::get_volume_u64(world_box.size));

	unsigned int label_count = 0;

	{
		// TODO 允许在不同 LOD 下运行该算法，以精度换取速度
		VOXEL_PROFILE_SCOPE_NAMED("CCL scan");
		IslandFinder island_finder;
		island_finder.scan_3d(
				Box3i(Vector3i(), world_box.size),
				[&source_copy_buffer](Vector3i pos) {
					// TODO 可以通过直接访问进一步优化
					return source_copy_buffer.get_voxel_f(pos.x, pos.y, pos.z, main_channel) < 0.f;
				},
				to_span(ccl_output),
				&label_count
		);
	}

	struct Bounds {
		Vector3i min_pos;
		Vector3i max_pos; // 包含
		bool valid = false;
	};

	if (main_channel == VoxelBuffer::CHANNEL_SDF) {
		// 传播标签以改善 SDF 质量，否则分离块的梯度会突然截断。
		// 局限：如果两个岛屿距离太近，其中一个会覆盖另一个。
		// 另一种方案是在单个块上执行此操作？
		box_propagate_ccl(to_span(ccl_output), world_box.size);
	}

	// 计算每个组的边界

	StdVector<Bounds> bounds_per_label;
	{
		VOXEL_PROFILE_SCOPE_NAMED("Bounds calculation");

		// 加 1，因为标签 0 是“无标签”的索引
		bounds_per_label.resize(label_count + 1);

		unsigned int ccl_index = 0;
		for (int z = 0; z < world_box.size.z; ++z) {
			for (int x = 0; x < world_box.size.x; ++x) {
				for (int y = 0; y < world_box.size.y; ++y) {
					CRASH_COND(ccl_index >= ccl_output.size());
					const uint8_t label = ccl_output[ccl_index];
					++ccl_index;

					if (label == 0) {
						continue;
					}

					CRASH_COND(label >= bounds_per_label.size());
					Bounds &bounds = bounds_per_label[label];

					if (bounds.valid == false) {
						bounds.min_pos = Vector3i(x, y, z);
						bounds.max_pos = bounds.min_pos;
						bounds.valid = true;

					} else {
						if (x < bounds.min_pos.x) {
							bounds.min_pos.x = x;
						} else if (x > bounds.max_pos.x) {
							bounds.max_pos.x = x;
						}

						if (y < bounds.min_pos.y) {
							bounds.min_pos.y = y;
						} else if (y > bounds.max_pos.y) {
							bounds.max_pos.y = y;
						}

						if (z < bounds.min_pos.z) {
							bounds.min_pos.z = z;
						} else if (z > bounds.max_pos.z) {
							bounds.max_pos.z = z;
						}
					}
				}
			}
		}
	}

	// 排除触及盒子边界的组，
	// 因为那意味着我们无法判断它们是真正悬空，还是连接到更远处的陆地上

	const Vector3i lbmax = world_box.size - Vector3i(1, 1, 1);
	for (unsigned int label = 1; label < bounds_per_label.size(); ++label) {
		CRASH_COND(label >= bounds_per_label.size());
		Bounds &local_bounds = bounds_per_label[label];
		ERR_CONTINUE(!local_bounds.valid);

		if ( //
				local_bounds.min_pos.x == 0 //
				|| local_bounds.min_pos.y == 0 //
				|| local_bounds.min_pos.z == 0 //
				|| local_bounds.max_pos.x == lbmax.x //
				|| local_bounds.max_pos.y == lbmax.y //
				|| local_bounds.max_pos.z == lbmax.z) {
			//
			local_bounds.valid = false;
		}
	}

	// 为每个组创建体素缓冲区

	struct InstanceInfo {
		VoxelBuffer voxels;
		Vector3i world_pos;
		unsigned int label;
	};
	StdVector<InstanceInfo> instances_info;

	const int min_padding = 2; // mesher->get_minimum_padding();
	const int max_padding = 2; // mesher->get_maximum_padding();

	{
		VOXEL_PROFILE_SCOPE_NAMED("Extraction");

		for (unsigned int label = 1; label < bounds_per_label.size(); ++label) {
			CRASH_COND(label >= bounds_per_label.size());
			const Bounds local_bounds = bounds_per_label[label];

			if (!local_bounds.valid) {
				continue;
			}

			const Vector3i world_pos = world_box.position + local_bounds.min_pos - Vector3iUtil::create(min_padding);
			const Vector3i size =
					local_bounds.max_pos - local_bounds.min_pos + Vector3iUtil::create(1 + max_padding + min_padding);

			instances_info.push_back(InstanceInfo{ VoxelBuffer(VoxelBuffer::ALLOCATOR_POOL), world_pos, label });

			VoxelBuffer &buffer = instances_info.back().voxels;
			buffer.create(size.x, size.y, size.z);

			// 从源体积读取体素
			voxel_tool.copy(world_pos, buffer, channels_mask, false);

			// 清理内边距边界
			const Box3i inner_box(
					Vector3iUtil::create(min_padding),
					buffer.get_size() - Vector3iUtil::create(min_padding + max_padding)
			);
			Box3i(Vector3i(), buffer.get_size()).difference(inner_box, [&buffer](Box3i box) {
				buffer.fill_area_f(constants::SDF_FAR_OUTSIDE, box.position, box.position + box.size, main_channel);
			});

			// 过滤掉不属于该标签的体素
			for (int z = local_bounds.min_pos.z; z <= local_bounds.max_pos.z; ++z) {
				for (int x = local_bounds.min_pos.x; x <= local_bounds.max_pos.x; ++x) {
					for (int y = local_bounds.min_pos.y; y <= local_bounds.max_pos.y; ++y) {
						const unsigned int ccl_index = Vector3iUtil::get_zxy_index(Vector3i(x, y, z), world_box.size);
						CRASH_COND(ccl_index >= ccl_output.size());
						const uint8_t label2 = ccl_output[ccl_index];

						if (label2 != 0 && label != label2) {
							buffer.set_voxel_f(
									constants::SDF_FAR_OUTSIDE,
									min_padding + x - local_bounds.min_pos.x,
									min_padding + y - local_bounds.min_pos.y,
									min_padding + z - local_bounds.min_pos.z,
									main_channel
							);
						}
					}
				}
			}
		}
	}

	// 从源体积中擦除体素。
	// 必须在从源体积复制体素之后执行。

	{
		VOXEL_PROFILE_SCOPE_NAMED("Erasing");

		voxel_tool.set_channel(main_channel);

		for (unsigned int instance_index = 0; instance_index < instances_info.size(); ++instance_index) {
			CRASH_COND(instance_index >= instances_info.size());
			const InstanceInfo &info = instances_info[instance_index];
			voxel_tool.sdf_stamp_erase(info.voxels, info.world_pos);
		}
	}

	// 找出哪些材质包含需要实例化的参数。
	//
	// 自提交 7dbc458bb4f3e0cc94e5070bd33bde41d214c98d 起，已无法再通过 Shader 的参数缓存
	// 快速检查某个 shader 是否按名称拥有某个 uniform。现在唯一的方法似乎是获取完整的
	// 参数列表并在其中查找，这既慢又写起来繁琐。

	uint32_t materials_to_instance_mask = 0;
	{
		StdVector<voxel::godot::ShaderParameterInfo> params;
		const String u_block_local_transform = VoxelStringNames::get_singleton().u_block_local_transform;

		VOXEL_ASSERT_RETURN_V_MSG(
				materials.size() < 32,
				Array(),
				"Too many materials. If you need more, make a request or change the code."
		);

		for (int material_index = 0; material_index < materials.size(); ++material_index) {
			Ref<ShaderMaterial> sm = materials[material_index];
			if (sm.is_null()) {
				continue;
			}

			Ref<Shader> shader = sm->get_shader();
			if (shader.is_null()) {
				continue;
			}

			params.clear();
			voxel::godot::get_shader_parameter_list(shader->get_rid(), params);

			for (const voxel::godot::ShaderParameterInfo &param_info : params) {
				if (param_info.name == u_block_local_transform) {
					materials_to_instance_mask |= (1 << material_index);
					break;
				}
			}
		}
	}

	// 创建实例

	Array nodes;

	{
		VOXEL_PROFILE_SCOPE_NAMED("Remeshing and instancing");

		for (unsigned int instance_index = 0; instance_index < instances_info.size(); ++instance_index) {
			CRASH_COND(instance_index >= instances_info.size());
			const InstanceInfo &info = instances_info[instance_index];

			CRASH_COND(info.label >= bounds_per_label.size());
			const Bounds local_bounds = bounds_per_label[info.label];
			ERR_CONTINUE(!local_bounds.valid);

			// DEBUG
			// print_line(String("--- Instance {0}").format(varray(instance_index)));
			// for (int z = 0; z < info.voxels->get_size().z; ++z) {
			// 	for (int x = 0; x < info.voxels->get_size().x; ++x) {
			// 		String s;
			// 		for (int y = 0; y < info.voxels->get_size().y; ++y) {
			// 			float sdf = info.voxels->get_voxel_f(x, y, z, VoxelBuffer::CHANNEL_SDF);
			// 			if (sdf < -0.1f) {
			// 				s += "X ";
			// 			} else if (sdf < 0.f) {
			// 				s += "x ";
			// 			} else {
			// 				s += "- ";
			// 			}
			// 		}
			// 		print_line(s);
			// 	}
			// 	print_line("//");
			// }

			const Transform3D local_transform(
					Basis(),
					info.world_pos
							// 撤销最小内边距
							+ Vector3i(1, 1, 1)
			);

			for (int i = 0; i < materials.size(); ++i) {
				if ((materials_to_instance_mask & (1 << i)) != 0) {
					Ref<ShaderMaterial> sm = materials[i];
					VOXEL_ASSERT_CONTINUE(sm.is_valid());
					sm = sm->duplicate(false);
					// 该参数应当有一个有效的默认值，与相对于体积的局部变换相匹配，
					// 这通常是按实例区分的，但在 Godot 3 中没有这样的功能，所以不得不
					// 进行复制。
					// TODO 尝试对标量 uniform 使用按实例的参数（Godot 4 不支持纹理）
					sm->set_shader_parameter(
							VoxelStringNames::get_singleton().u_block_local_transform, local_transform
					);
					materials[i] = sm;
				}
			}

			// TODO 如果此处与 Transvoxel 网格化器一起使用法线贴图，我们需要要么仅针对
			// 此调用关闭它，要么传入正确的选项
			Ref<ArrayMesh> mesh = mesher->build_mesh(info.voxels, materials, Dictionary());
			// 网格不应为空，
			// 因为我们是从具有负 SDF 的连通组构建这些缓冲区的。
			ERR_CONTINUE(mesh.is_null());

			if (voxel::godot::is_mesh_empty(**mesh)) {
				continue;
			}

			// DEBUG
			// {
			// 	Ref<VoxelBlockSerializer> serializer;
			// 	serializer.instance();
			// 	Ref<StreamPeerBuffer> peer;
			// 	peer.instance();
			// 	serializer->serialize(peer, info.voxels, false);
			// 	String fpath = String("debug_data/split_dump_{0}.bin").format(varray(instance_index));
			// 	FileAccess *f = FileAccess::open(fpath, FileAccess::WRITE);
			// 	PoolByteArray bytes = peer->get_data_array();
			// 	PoolByteArray::Read bytes_read = bytes.read();
			// 	f->store_buffer(bytes_read.ptr(), bytes.size());
			// 	f->close();
			// 	memdelete(f);
			// }

			// TODO 提供生成多个凸形状的选项
			// TODO 使用快速方式。由于内部的 TriangleMesh 和网格数据查询，这很慢。
			// TODO 如果网格没有三角形，则不创建刚体
			Ref<Shape3D> shape = mesh->create_convex_shape();
			ERR_CONTINUE(shape.is_null());
			CollisionShape3D *collision_shape = memnew(CollisionShape3D);
			collision_shape->set_shape(shape);
			// 将形状稍微居中，因为 Godot 把节点原点与质心混淆了
			const Vector3i size =
					local_bounds.max_pos - local_bounds.min_pos + Vector3iUtil::create(1 + max_padding + min_padding);
			const Vector3 offset = -Vector3(size) * 0.5f;
			collision_shape->set_position(offset);

			RigidBody3D *rigid_body = memnew(RigidBody3D);
			rigid_body->set_transform(terrain_transform * local_transform.translated_local(-offset));
			rigid_body->add_child(collision_shape);
			rigid_body->set_freeze_mode(RigidBody3D::FREEZE_MODE_KINEMATIC);
			rigid_body->set_freeze_enabled(true);

			// 短暂时间后切换为刚体模式，以解决与地形的穿插问题，
			// 因为碰撞体是异步更新的
			Timer *timer = memnew(Timer);
			timer->set_wait_time(0.2);
			timer->set_one_shot(true);
			timer->connect("timeout", callable_mp(rigid_body, &RigidBody3D::set_freeze_enabled).bind(false));
			// 这里不能使用 start()，因为它要求位于 SceneTree 内，
			// 而我们在添加到父节点之前不知道它是否会处于其中。
			timer->set_autostart(true);
			rigid_body->add_child(timer);

			MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
			mesh_instance->set_mesh(mesh);
			mesh_instance->set_position(offset);
			rigid_body->add_child(mesh_instance);

			parent_node->add_child(rigid_body);

			nodes.append(rigid_body);
		}
	}

	return nodes;
}

} // namespace voxel
