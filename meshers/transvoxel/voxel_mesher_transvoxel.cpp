#include "voxel_mesher_transvoxel.h"
#include "../../engine/voxel_engine.h"
#include "../../generators/voxel_generator.h"
#include "../../shaders/transvoxel_minimal_shader.h"
#include "../../storage/voxel_buffer_gd.h"
#include "../../storage/voxel_data.h"
#include "../../thirdparty/meshoptimizer/meshoptimizer.h"
#include "../../util/godot/classes/array_mesh.h"
#include "../../util/godot/classes/rendering_server.h"
#include "../../util/godot/classes/shader.h"
#include "../../util/godot/classes/shader_material.h"
#include "../../util/godot/core/packed_arrays.h"
#include "../../util/math/conv.h"
#include "../../util/profiling.h"
#include "transvoxel_tables.cpp"
#ifdef TOOLS_ENABLED
#include "../../util/string/format.h"
#endif

using namespace voxel::godot;

namespace voxel {

namespace {
Ref<ShaderMaterial> g_minimal_shader_material;
} // namespace

namespace transvoxel {
// 将 thread-local 包装在函数中，使它们在首次需要时才初始化，而不是在应用程序启动时初始化。
// 这可以规避静态调试 MSVC 运行时（/MTd）中的一个 bug。在撰写本文时，即使在调试构建中 Godot 也
// 使用 /MT，这导致无法获得标准库提供的安全检查。
// 参见 https://github.com/baldurk/renderdoc/issues/1743
// 以及 https://developercommunity.visualstudio.com/t/race-condition-on-g-tss-mutex-with-static-crt/672664
MeshArrays &get_tls_mesh_arrays() {
	thread_local MeshArrays tls_mesh_arrays;
	return tls_mesh_arrays;
}
StdVector<CellInfo> &get_tls_cell_infos() {
	thread_local StdVector<CellInfo> tls_cell_infos;
	return tls_cell_infos;
}
} // namespace transvoxel

const transvoxel::MeshArrays &VoxelMesherTransvoxel::get_mesh_cache_from_current_thread() {
	return transvoxel::get_tls_mesh_arrays();
}

Span<const transvoxel::CellInfo> VoxelMesherTransvoxel::get_cell_info_from_current_thread() {
	return to_span(transvoxel::get_tls_cell_infos());
}

void VoxelMesherTransvoxel::load_static_resources() {
	Ref<Shader> shader;
	shader.instantiate();
	shader->set_code(g_transvoxel_minimal_shader);
	g_minimal_shader_material.instantiate();
	g_minimal_shader_material->set_shader(shader);
}

void VoxelMesherTransvoxel::free_static_resources() {
	g_minimal_shader_material.unref();
}

VoxelMesherTransvoxel::VoxelMesherTransvoxel() {
	set_padding(transvoxel::MIN_PADDING, transvoxel::MAX_PADDING);
}

VoxelMesherTransvoxel::~VoxelMesherTransvoxel() {}

int VoxelMesherTransvoxel::get_used_channels_mask() const {
	uint32_t mask = 1 << VoxelBuffer::CHANNEL_SDF;

	switch (_texture_mode) {
		case TEXTURES_NONE:
			break;
		case TEXTURES_BLEND_4_OVER_16:
			mask |= (1 << VoxelBuffer::CHANNEL_INDICES) | (1 << VoxelBuffer::CHANNEL_WEIGHTS);
			break;
		case TEXTURES_SINGLE_S4:
			mask |= (1 << VoxelBuffer::CHANNEL_INDICES);
			break;
		default:
			VOXEL_PRINT_ERROR("Unhandled texture mode");
			break;
	}

	return mask;
}

bool VoxelMesherTransvoxel::is_generating_collision_surface() const {
	// 通过子网格索引
	return true;
}

namespace {

void fill_surface_arrays(Array &arrays, const transvoxel::MeshArrays &src) {
	PackedVector3Array vertices;
	PackedVector3Array normals;
	PackedFloat32Array lod_data; // 4*float32
	PackedFloat32Array texturing_data; // 2*4*uint8 作为 2*float32，或 3*uint8 作为 1*float32
	PackedInt32Array indices;

	copy_to(vertices, to_span_const(src.vertices));

	// raw_copy_to(lod_data, src.lod_data);
	lod_data.resize(src.lod_data.size() * 4);
	// 根据布局，前 3 个 float 是位置，第 4 个 float 实际上是一个位掩码
	static_assert(sizeof(transvoxel::LodAttrib) == 16);
	memcpy(lod_data.ptrw(), src.lod_data.data(), lod_data.size() * sizeof(float));

	copy_to(indices, to_span_const(src.indices));

	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	if (src.normals.size() != 0) {
		copy_to(normals, to_span_const(src.normals));
		arrays[Mesh::ARRAY_NORMAL] = normals;
	}

	if (src.texturing_data_1f32.size() != 0) {
		texturing_data.resize(src.texturing_data_1f32.size());
		memcpy(texturing_data.ptrw(), src.texturing_data_1f32.data(), texturing_data.size() * sizeof(float));
		arrays[Mesh::ARRAY_CUSTOM1] = texturing_data;

	} else if (src.texturing_data_2f32.size() != 0) {
		texturing_data.resize(src.texturing_data_2f32.size() * 2);
		memcpy(texturing_data.ptrw(), src.texturing_data_2f32.data(), texturing_data.size() * sizeof(float));
		arrays[Mesh::ARRAY_CUSTOM1] = texturing_data;
	}

	arrays[Mesh::ARRAY_CUSTOM0] = lod_data;
	arrays[Mesh::ARRAY_INDEX] = indices;
}

template <typename T>
void remap_vertex_array(
		const StdVector<T> &src_data,
		StdVector<T> &dst_data,
		const StdVector<unsigned int> &remap_indices,
		const unsigned int unique_vertex_count
) {
	if (src_data.size() == 0) {
		dst_data.clear();
		return;
	}
	dst_data.resize(unique_vertex_count);
	voxelmeshopt::meshopt_remapVertexBuffer(
			&dst_data[0], &src_data[0], src_data.size(), sizeof(T), remap_indices.data()
	);
}

void simplify(
		const transvoxel::MeshArrays &src_mesh,
		transvoxel::MeshArrays &dst_mesh,
		float p_target_ratio,
		float p_error_threshold
) {
	VOXEL_PROFILE_SCOPE();

	// 收集并检查输入

	ERR_FAIL_COND(p_target_ratio < 0.f || p_target_ratio > 1.f);
	ERR_FAIL_COND(p_error_threshold < 0.f || p_error_threshold > 1.f);
	ERR_FAIL_COND(src_mesh.vertices.size() < 3);
	ERR_FAIL_COND(src_mesh.indices.size() < 3);

	const unsigned int target_index_count = p_target_ratio * src_mesh.indices.size();

	static thread_local StdVector<unsigned int> lod_indices;
	lod_indices.clear();
	lod_indices.resize(src_mesh.indices.size());

	float lod_error = 0.f;

	// 简化
	{
		VOXEL_PROFILE_SCOPE_NAMED("meshopt_simplify");

		// TODO 关于 `voxelmeshopt::` 命名空间，请参见构建脚本
		const unsigned int lod_index_count = voxelmeshopt::meshopt_simplify(
				&lod_indices[0],
				reinterpret_cast<const unsigned int *>(src_mesh.indices.data()),
				src_mesh.indices.size(),
				&src_mesh.vertices[0].x,
				src_mesh.vertices.size(),
				sizeof(Vector3f),
				target_index_count,
				p_error_threshold,
				// 对数据块边界至关重要，参见 https://github.com/zeux/meshoptimizer/issues/311
				voxelmeshopt::meshopt_SimplifyLockBorder,
				&lod_error
		);

		lod_indices.resize(lod_index_count);
	}

	// 生成输出

	Array surface;
	surface.resize(Mesh::ARRAY_MAX);

	static thread_local StdVector<unsigned int> remap_indices;
	remap_indices.clear();
	remap_indices.resize(src_mesh.vertices.size());

	const unsigned int unique_vertex_count = voxelmeshopt::meshopt_optimizeVertexFetchRemap(
			&remap_indices[0], lod_indices.data(), lod_indices.size(), src_mesh.vertices.size()
	);

	remap_vertex_array(src_mesh.vertices, dst_mesh.vertices, remap_indices, unique_vertex_count);
	remap_vertex_array(src_mesh.normals, dst_mesh.normals, remap_indices, unique_vertex_count);
	remap_vertex_array(src_mesh.lod_data, dst_mesh.lod_data, remap_indices, unique_vertex_count);
	remap_vertex_array(src_mesh.texturing_data_1f32, dst_mesh.texturing_data_1f32, remap_indices, unique_vertex_count);
	remap_vertex_array(src_mesh.texturing_data_2f32, dst_mesh.texturing_data_2f32, remap_indices, unique_vertex_count);

	dst_mesh.indices.resize(lod_indices.size());
	// TODO 不确定参数是否正确
	voxelmeshopt::meshopt_remapIndexBuffer(
			reinterpret_cast<unsigned int *>(dst_mesh.indices.data()),
			lod_indices.data(),
			lod_indices.size(),
			remap_indices.data()
	);
}

} // namespace

// TODO 也许我们可以自动检测？不过可能会变得有歧义
static VoxelMesherTransvoxel::TexturingMode check_texturing_mode(
		const VoxelMesherTransvoxel::TexturingMode expected_tex_mode,
		const VoxelBuffer &vb
) {
#ifdef TOOLS_ENABLED
	// 在开发版本中做更完善的错误报告
	switch (expected_tex_mode) {
		case VoxelMesherTransvoxel::TEXTURES_MIXEL4_S4: {
			const VoxelBuffer::Depth indices_depth = vb.get_channel_depth(VoxelBuffer::CHANNEL_INDICES);
			if (indices_depth != VoxelBuffer::DEPTH_16_BIT) {
				VOXEL_PRINT_ERROR_ONCE(format(
						"The Indices channel is set to {} bits, but 16 bits are necessary to use the Mixel4 texturing "
						"mode.",
						VoxelBuffer::get_depth_byte_count(indices_depth)
				));
				return VoxelMesherTransvoxel::TEXTURES_NONE;
			}
			const VoxelBuffer::Depth weights_depth = vb.get_channel_depth(VoxelBuffer::CHANNEL_WEIGHTS);
			if (weights_depth != VoxelBuffer::DEPTH_16_BIT) {
				VOXEL_PRINT_ERROR_ONCE(format(
						"The Weights channel is set to {} bits, but 16 bits are necessary to use the Mixel4 texturing "
						"mode.",
						VoxelBuffer::get_depth_byte_count(weights_depth)
				));
				return VoxelMesherTransvoxel::TEXTURES_NONE;
			}
		} break;

		case VoxelMesherTransvoxel::TEXTURES_SINGLE_S4: {
			const VoxelBuffer::Depth indices_depth = vb.get_channel_depth(VoxelBuffer::CHANNEL_INDICES);
			if (indices_depth != VoxelBuffer::DEPTH_8_BIT) {
				VOXEL_PRINT_WARNING_ONCE(
						format("The Indices channel is set to {} bits, but only 8 bits are required to use the Single "
							   "texturing mode.",
							   VoxelBuffer::get_depth_byte_count(indices_depth))
				);
				return VoxelMesherTransvoxel::TEXTURES_NONE;
			}
		} break;

		case VoxelMesherTransvoxel::TEXTURES_NONE:
			break;

		default:
			VOXEL_PRINT_ERROR_ONCE("Unknown texture mode");
			break;
	}
#endif
	return expected_tex_mode;
}

void VoxelMesherTransvoxel::build(VoxelMesher::Output &output, const VoxelMesher::Input &input) {
	VOXEL_PROFILE_SCOPE();

	static thread_local transvoxel::Cache tls_cache;
	// static thread_local FixedArray<transvoxel::MeshArrays, Cube::SIDE_COUNT> tls_transition_mesh_arrays;
	static thread_local transvoxel::MeshArrays tls_simplified_mesh_arrays;

	const VoxelBuffer::ChannelId sdf_channel = VoxelBuffer::CHANNEL_SDF;

	// 初始化动态内存：
	// 这些向量会被复用。
	// 我们事先不知道将生成多少几何体。
	// 一旦容量足够大，就不应再分配内存
	transvoxel::MeshArrays &mesh_arrays = transvoxel::get_tls_mesh_arrays();
	mesh_arrays.clear();

	const VoxelBuffer &voxels = input.voxels;
	if (voxels.is_uniform(sdf_channel)) {
		// SDF 没有变化，因此不会与等值面相交，也就没有可多边形化的内容
		return;
	}

	// const uint64_t time_before = Time::get_singleton()->get_ticks_usec();

	transvoxel::DefaultTextureIndicesData default_texture_indices_data;
	StdVector<transvoxel::CellInfo> *cell_infos = nullptr;
	if (input.detail_texture_hint) {
		transvoxel::get_tls_cell_infos().clear();
		cell_infos = &transvoxel::get_tls_cell_infos();
	}

	const TexturingMode texture_mode = check_texturing_mode(_texture_mode, voxels);

	default_texture_indices_data = transvoxel::build_regular_mesh(
			voxels,
			sdf_channel,
			input.lod_index,
			static_cast<transvoxel::TexturingMode>(texture_mode),
			tls_cache,
			mesh_arrays,
			cell_infos,
			_edge_clamp_margin,
			_textures_ignore_air_voxels
	);

	if (mesh_arrays.vertices.size() == 0) {
		// 网格可能为空
		return;
	}
	if (mesh_arrays.indices.size() == 0) {
		// 网格可能有顶点但仍然为空，例如因为三角形全是退化的
		return;
	}

	transvoxel::MeshArrays *combined_mesh_arrays = &mesh_arrays;
	if (_mesh_optimization_params.enabled) {
		// TODO 启用体素纹理时，这会大幅降低质量。
		// 目前简化时还不支持考虑纹理。
		// 参见 https://github.com/zeux/meshoptimizer/issues/158
		simplify(
				mesh_arrays,
				tls_simplified_mesh_arrays,
				_mesh_optimization_params.target_ratio,
				_mesh_optimization_params.error_threshold
		);

		combined_mesh_arrays = &tls_simplified_mesh_arrays;
	}

	output.collision_surface.submesh_vertex_end = combined_mesh_arrays->vertices.size();
	output.collision_surface.submesh_index_end = combined_mesh_arrays->indices.size();

	if (_transitions_enabled && input.lod_hint) {
		// 我们将过渡网格与常规网格合并，因为这样比分开时产生的绘制调用更少。
		// 这只需要一个顶点着色器技巧，在邻居变化时丢弃它们。
		VOXEL_ASSERT(combined_mesh_arrays != nullptr);

		for (int dir = 0; dir < Cube::SIDE_COUNT; ++dir) {
			VOXEL_PROFILE_SCOPE();

			transvoxel::build_transition_mesh(
					voxels,
					sdf_channel,
					dir,
					input.lod_index,
					static_cast<transvoxel::TexturingMode>(texture_mode),
					tls_cache,
					*combined_mesh_arrays,
					default_texture_indices_data,
					_edge_clamp_margin,
					_textures_ignore_air_voxels
			);
		}
	}

	Array gd_arrays;
	fill_surface_arrays(gd_arrays, *combined_mesh_arrays);
	output.surfaces.push_back({ gd_arrays, 0 });

	// const uint64_t time_spent = Time::get_singleton()->get_ticks_usec() - time_before;
	// print_line(String("VoxelMesherTransvoxel spent {0} us").format(varray(time_spent)));

	output.primitive_type = Mesh::PRIMITIVE_TRIANGLES;

	// Transvoxel 过渡数据
	output.mesh_flags = (RenderingServerEnums::ARRAY_CUSTOM_RGBA_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT);

	// 纹理数据
	switch (texture_mode) {
		case TEXTURES_NONE:
			break;
		case TEXTURES_MIXEL4_S4:
		case TEXTURES_SINGLE_S4:
			output.mesh_flags |= (RenderingServerEnums::ARRAY_CUSTOM_RG_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM1_SHIFT);
			break;
		default:
			VOXEL_PRINT_ERROR("Unhandled texture mode");
			break;
	}
}

// 仅用于测试
Ref<ArrayMesh> VoxelMesherTransvoxel::build_transition_mesh(Ref<godot::VoxelBuffer> voxels, int direction) {
	static thread_local transvoxel::Cache s_cache;
	static thread_local transvoxel::MeshArrays s_mesh_arrays;

	s_mesh_arrays.clear();

	ERR_FAIL_COND_V(voxels.is_null(), Ref<ArrayMesh>());

	if (voxels->is_uniform(VoxelBuffer::CHANNEL_SDF)) {
		// 均匀的 SDF 不会产生任何表面
		return Ref<ArrayMesh>();
	}

	// TODO 我们需要通过通用接口输出过渡网格，它们是结果的一部分
	// 目前在这种特定情况下我们还无法支持正确的纹理索引
	transvoxel::DefaultTextureIndicesData default_texture_indices_data;
	default_texture_indices_data.use = false;
	transvoxel::build_transition_mesh(
			voxels->get_buffer(),
			VoxelBuffer::CHANNEL_SDF,
			direction,
			0,
			static_cast<transvoxel::TexturingMode>(_texture_mode),
			s_cache,
			s_mesh_arrays,
			default_texture_indices_data,
			_edge_clamp_margin,
			_textures_ignore_air_voxels
	);

	Ref<ArrayMesh> mesh;

	if (s_mesh_arrays.vertices.size() == 0) {
		return mesh;
	}

	Array arrays;
	fill_surface_arrays(arrays, s_mesh_arrays);
	mesh.instantiate();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	return mesh;
}

void VoxelMesherTransvoxel::set_texturing_mode(TexturingMode mode) {
	VOXEL_ASSERT_RETURN(mode >= 0 && mode < TEXTURES_MODE_COUNT);
	if (mode != _texture_mode) {
		_texture_mode = mode;
		emit_changed();
	}
}

VoxelMesherTransvoxel::TexturingMode VoxelMesherTransvoxel::get_texturing_mode() const {
	return _texture_mode;
}

void VoxelMesherTransvoxel::set_textures_ignore_air_voxels(const bool enable) {
	_textures_ignore_air_voxels = enable;
}

bool VoxelMesherTransvoxel::get_textures_ignore_air_voxels() const {
	return _textures_ignore_air_voxels;
}

void VoxelMesherTransvoxel::set_mesh_optimization_enabled(bool enabled) {
	_mesh_optimization_params.enabled = enabled;
}

bool VoxelMesherTransvoxel::is_mesh_optimization_enabled() const {
	return _mesh_optimization_params.enabled;
}

void VoxelMesherTransvoxel::set_mesh_optimization_error_threshold(float threshold) {
	_mesh_optimization_params.error_threshold = math::clamp(threshold, 0.f, 1.f);
}

float VoxelMesherTransvoxel::get_mesh_optimization_error_threshold() const {
	return _mesh_optimization_params.error_threshold;
}

void VoxelMesherTransvoxel::set_mesh_optimization_target_ratio(float ratio) {
	_mesh_optimization_params.target_ratio = math::clamp(ratio, 0.f, 1.f);
}

float VoxelMesherTransvoxel::get_mesh_optimization_target_ratio() const {
	return _mesh_optimization_params.target_ratio;
}

void VoxelMesherTransvoxel::set_transitions_enabled(bool enable) {
	_transitions_enabled = enable;
}

bool VoxelMesherTransvoxel::get_transitions_enabled() const {
	return _transitions_enabled;
}

Ref<ShaderMaterial> VoxelMesherTransvoxel::get_default_lod_material() const {
	return g_minimal_shader_material;
}

void VoxelMesherTransvoxel::set_edge_clamp_margin(float margin) {
	_edge_clamp_margin = math::clamp(margin, 0.f, 0.5f);
}

float VoxelMesherTransvoxel::get_edge_clamp_margin() const {
	return _edge_clamp_margin;
}

void VoxelMesherTransvoxel::_bind_methods() {
	using Self = VoxelMesherTransvoxel;

	ClassDB::bind_method(D_METHOD("build_transition_mesh", "voxel_buffer", "direction"), &Self::build_transition_mesh);

	ClassDB::bind_method(D_METHOD("set_texturing_mode", "mode"), &Self::set_texturing_mode);
	ClassDB::bind_method(D_METHOD("get_texturing_mode"), &Self::get_texturing_mode);

	ClassDB::bind_method(D_METHOD("set_textures_ignore_air_voxels", "enabled"), &Self::set_textures_ignore_air_voxels);
	ClassDB::bind_method(D_METHOD("get_textures_ignore_air_voxels"), &Self::get_textures_ignore_air_voxels);

	ClassDB::bind_method(D_METHOD("set_mesh_optimization_enabled", "enabled"), &Self::set_mesh_optimization_enabled);
	ClassDB::bind_method(D_METHOD("is_mesh_optimization_enabled"), &Self::is_mesh_optimization_enabled);

	ClassDB::bind_method(
			D_METHOD("set_mesh_optimization_error_threshold", "threshold"), &Self::set_mesh_optimization_error_threshold
	);
	ClassDB::bind_method(
			D_METHOD("get_mesh_optimization_error_threshold"), &Self::get_mesh_optimization_error_threshold
	);

	ClassDB::bind_method(
			D_METHOD("set_mesh_optimization_target_ratio", "ratio"), &Self::set_mesh_optimization_target_ratio
	);
	ClassDB::bind_method(D_METHOD("get_mesh_optimization_target_ratio"), &Self::get_mesh_optimization_target_ratio);

	ClassDB::bind_method(D_METHOD("set_transitions_enabled", "enabled"), &Self::set_transitions_enabled);
	ClassDB::bind_method(D_METHOD("get_transitions_enabled"), &Self::get_transitions_enabled);

	ClassDB::bind_method(D_METHOD("get_edge_clamp_margin"), &Self::get_edge_clamp_margin);
	ClassDB::bind_method(D_METHOD("set_edge_clamp_margin", "margin"), &Self::set_edge_clamp_margin);

	ADD_GROUP("Materials", "");

	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "texturing_mode", PROPERTY_HINT_ENUM, "None,Mixel4,Single"),
			"set_texturing_mode",
			"get_texturing_mode"
	);

	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "textures_ignore_air_voxels"),
			"set_textures_ignore_air_voxels",
			"get_textures_ignore_air_voxels"
	);

	ADD_GROUP("Mesh optimization", "mesh_optimization_");

	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "mesh_optimization_enabled"),
			"set_mesh_optimization_enabled",
			"is_mesh_optimization_enabled"
	);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "mesh_optimization_error_threshold"),
			"set_mesh_optimization_error_threshold",
			"get_mesh_optimization_error_threshold"
	);
	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "mesh_optimization_target_ratio"),
			"set_mesh_optimization_target_ratio",
			"get_mesh_optimization_target_ratio"
	);

	ADD_GROUP("Advanced", "");

	ADD_PROPERTY(
			PropertyInfo(Variant::BOOL, "transitions_enabled"), "set_transitions_enabled", "get_transitions_enabled"
	);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "edge_clamp_margin"), "set_edge_clamp_margin", "get_edge_clamp_margin");

	BIND_ENUM_CONSTANT(TEXTURES_NONE);
	BIND_ENUM_CONSTANT(TEXTURES_MIXEL4_S4);
	BIND_ENUM_CONSTANT(TEXTURES_SINGLE_S4);

	// MIXEL4_S4 的旧别名
	BIND_CONSTANT(TEXTURES_BLEND_4_OVER_16);
}

} // namespace voxel
