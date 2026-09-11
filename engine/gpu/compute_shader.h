#include <core/templates/rid.h>
#ifndef VOXEL_COMPUTE_SHADER_H
#define VOXEL_COMPUTE_SHADER_H

#include <core/templates/rid.h>
#include "../../util/godot/core/string.h"
#include "../../util/memory/memory.h"

class RenderingDevice;

namespace voxel {

struct ComputeShaderInternal {
	RID rid;
#if DEBUG_ENABLED
	String debug_name;
#endif

	void clear(RenderingDevice &rd);
	void load_from_glsl(RenderingDevice &rd, String source_text, String name);

	// 无效实例意味着着色器编译失败
	inline bool is_valid() const {
		return rid.is_valid();
	}
};

class ComputeShader;

// 参见 ComputeShaderResourceFactory
struct ComputeShaderFactory {
	ComputeShaderFactory() = delete;

	[[nodiscard]]
	static std::shared_ptr<ComputeShader> create_from_glsl(String source_text, String name);

	[[nodiscard]]
	static std::shared_ptr<ComputeShader> create_invalid();
};

// 对使用 `VoxelEngine` 持有的 `RenderingDevice` 创建的计算着色器的轻量 RAII 封装。
// 如果源码在运行时可能改变，可以通过共享指针传递，并创建新实例，
// 而不是随时清空旧着色器，以保证线程安全。只要该着色器的一次派发
// 仍在显卡上运行，就应保持其引用。
class ComputeShader {
public:
	friend struct ComputeShaderFactory;

	~ComputeShader();

	// 仅在 GPU 任务线程上使用
	RID get_rid() const;

private:
	ComputeShaderInternal _internal;
};

} // namespace voxel

#endif // VOXEL_COMPUTE_SHADER_H
