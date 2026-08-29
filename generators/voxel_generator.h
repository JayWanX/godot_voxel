#ifndef VOXEL_GENERATOR_H
#define VOXEL_GENERATOR_H

#include "../engine/ids.h"
#include "../engine/priority_dependency.h"
#include "../storage/voxel_format.h"
#include "../util/containers/span.h"
#include "../util/containers/std_vector.h"
#include "../util/godot/classes/resource.h"
#include "../util/math/box3i.h"
#include "../util/math/vector3f.h"
#include "../util/tasks/cancellation_token.h"
#include "../util/thread/mutex.h"

#ifdef VOXEL_ENABLE_GPU
#include "../engine/gpu/compute_shader_resource.h"
#endif

#include <memory>

namespace voxel {

class IThreadedTask;
class AsyncDependencyTracker;

class VoxelBuffer;
struct StreamingDependency;
class VoxelData;

#ifdef VOXEL_ENABLE_GPU
class ComputeShader;
struct ComputeShaderParameters;
#endif

namespace godot {
class VoxelBuffer;
}

// 未编码的通用体素值。
// （存储在 VoxelBuffer 中的体素会被编码以占用更少空间）
union VoxelSingleValue {
	uint64_t i;
	float f;
};

// 提供对只读生成的体素的访问。
// 必须以多线程安全的方式实现。
class VoxelGenerator : public Resource {
	GDCLASS(VoxelGenerator, Resource)
public:
	VoxelGenerator();

	struct Result {
		// 用于在使用 LOD 时对数据块进行优化。
		// 如果为 `false`，请求更低的 LOD 索引时可能会找到更精确的数据。
		// 如果为 `true`，低于该 LOD 的任何数据块都被认为不会带来更多细节，或者与当前相同。
		// 这有助于在使用 LOD 时减少需要加载的数据块数量。
		bool max_lod_hint = false;
	};

	struct VoxelQueryData {
		VoxelBuffer &voxel_buffer;
		Vector3i origin_in_voxels;
		uint32_t lod;
	};

	virtual Result generate_block(VoxelQueryData input);

	struct BlockTaskParams {
		Vector3i block_position;
		VoxelFormat format;
		VolumeID volume_id;
		uint8_t lod_index = 0;
		uint8_t block_size = 0;
		bool drop_beyond_max_distance = true;
#ifdef VOXEL_ENABLE_GPU
		bool use_gpu = false; // 这是一个提示，若不支持则继续使用 CPU
#endif
		PriorityDependency priority_dependency;
		std::shared_ptr<StreamingDependency> stream_dependency; // 用于保存生成器输出
		std::shared_ptr<VoxelData> data; // 仅用于 modifiers
		std::shared_ptr<AsyncDependencyTracker> tracker; // 用于异步编辑
		std::shared_ptr<VoxelBuffer> voxels; // 可选，为结果复用一个体素缓冲区
		TaskCancellationToken cancellation_token; // 用于显式取消
	};

	// 创建一个线程任务，该任务将异步使用生成器来生成一个数据块，并把结果返回给
	// 请求它的 volume。
	virtual IThreadedTask *create_block_task(const BlockTaskParams &params) const;

	virtual bool supports_single_generation() const {
		return false;
	}

	virtual bool supports_series_generation() const {
		return false;
	}

	virtual bool supports_lod() const {
		return true;
	}

	// TODO 不确定这个 API 在性能方面是否合适
	virtual VoxelSingleValue generate_single(Vector3i pos, unsigned int channel);

	virtual void generate_series(
			Span<const float> positions_x,
			Span<const float> positions_y,
			Span<const float> positions_z,
			unsigned int channel,
			Span<float> out_values,
			Vector3f min_pos,
			Vector3f max_pos
	);

	// 声明该生成器将使用的通道
	virtual int get_used_channels_mask() const;

#ifdef VOXEL_ENABLE_GPU
	// GPU 支持
	// 该支持的工作方式是提供着色器及参数，使其能够产生与 CPU 版本生成器相同的结果。

	virtual bool supports_shaders() const {
		return false;
	}

	struct ShaderParameter {
		String name;
		std::shared_ptr<ComputeShaderResource> resource;
	};

	struct ShaderOutput {
		enum Type { TYPE_SDF, TYPE_SINGLE_TEXTURE, TYPE_TYPE };
		Type type;
	};

	struct ShaderSourceData {
		// 仅与生成器相关的源代码。不包含接口块（uniforms），因为它们可能会
		// 根据代码集成的位置而生成。
		String glsl;
		// 关联的资源
		StdVector<ShaderParameter> parameters;

		// 生成的源码将包含一个以 `vec3 position` 参数开头的 `generate` 函数，
		// 其后是诸如 `out float out_sd, ...` 之类的输出，这决定了计算着色器
		// 将返回什么。
		StdVector<ShaderOutput> outputs;
	};

	struct ShaderOutputs {
		StdVector<ShaderOutput> outputs;
	};

	virtual bool get_shader_source(ShaderSourceData &out_data) const;
	std::shared_ptr<ComputeShader> get_detail_rendering_shader();
	std::shared_ptr<ComputeShaderParameters> get_detail_rendering_shader_parameters();
	std::shared_ptr<ComputeShader> get_block_rendering_shader();
	// TODO 这些参数难道不应该按每种着色器类型分别共享吗？
	std::shared_ptr<ComputeShaderParameters> get_block_rendering_shader_parameters();
	std::shared_ptr<ShaderOutputs> get_block_rendering_shader_outputs();
	void compile_shaders();
	// 若有已编译的着色器则将其丢弃，以便在再次需要时重新编译
	void invalidate_shaders();
#endif

	// 请求生成一个宽阶段（broad）结果，它应比完整生成更快获得。
	// 如果返回 true，返回的数据块可以当作 `generate_block` 的结果来使用。
	// 如果返回 false，则不返回任何数据块，应使用完整生成。
	// 通常 `generate_block` 内部也能做到这一点，但在某些情况下（如 GPU 生成）它可能
	// 被用来避免向显卡发送工作。
	virtual bool generate_broad_block(VoxelQueryData input);

	// 缓存 API
	//
	// 某些生成器可能使用内部缓存来优化性能。以下方法为生成器管理缓存的
	// 生命周期提供一些信息。
	// 目前仅用于 VoxelTerrain。

	// 当观察者与地形配对、移动或取消配对时，必须调用此方法。
	// 配对时应发送一个空的先前 box。
	// 移动时应发送先前的 box 和新的 box。
	// 取消配对时应发送一个空 box 作为当前 box。
	virtual void process_viewer_diff(ViewerID viewer_id, Box3i p_requested_box, Box3i p_prev_requested_box);

	virtual void clear_cache();

	// 提示流（stream）的函数是否可以调用。主要用于脚本实现的情况，以避免
	// 错误刷屏。
	virtual bool is_runnable() const;

	// 编辑器

#ifdef TOOLS_ENABLED
	virtual void get_configuration_warnings(PackedStringArray &out_warnings) const {}
#endif

protected:
	static void _bind_methods();

	void _b_generate_block(Ref<godot::VoxelBuffer> out_buffer, Vector3 origin_in_voxels, int lod);

#ifdef VOXEL_ENABLE_GPU
	std::shared_ptr<ComputeShader> _detail_rendering_shader;
	std::shared_ptr<ComputeShaderParameters> _detail_rendering_shader_parameters;
	std::shared_ptr<ComputeShader> _block_rendering_shader;
	std::shared_ptr<ComputeShaderParameters> _block_rendering_shader_parameters;
	std::shared_ptr<ShaderOutputs> _block_rendering_shader_outputs;
#endif
	Mutex _shader_mutex;
};

} // namespace voxel

#endif // VOXEL_GENERATOR_H
