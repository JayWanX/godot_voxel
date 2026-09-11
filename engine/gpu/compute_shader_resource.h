#include <core/templates/rid.h>
#include <core/math/vector3i.h>
#ifndef VOXEL_COMPUTE_SHADER_RESOURCE_H
#define VOXEL_COMPUTE_SHADER_RESOURCE_H

#include "../../util/containers/span.h"
#include "../../util/godot/classes/ref_counted.h"
#include "../../util/godot/core/packed_byte_array.h"
#include <core/templates/rid.h>
#include "../../util/godot/core/transform_3d.h"
#include <core/math/vector3i.h>
#include "../../util/godot/macros.h"
#include "../../util/macros.h"
#include <memory>

class Image;
class Curve;
class RenderingDevice;

namespace voxel {

// 这是我们自己为使用 RenderingDevice 创建的资源提供的轻量封装。我们不能使用 Godot 的常规资源，
// 因为它们要么不存在，要么假设我们使用主 RenderingDevice（用于渲染的那个）。
// 我们使用自己的 RenderingDevice，因为我们希望运行非基于帧的重型计算着色器，
// 并且我们在专用线程中使用它，因为 Godot 没有异步上传/下载数据的 API。
// RD 函数是阻塞的（等待同步），所以我们不想在做我们的事情时阻塞主线程或渲染线程。
//
// 除此之外，Godot 强制我们在创建 RenderingDevice 的同一线程上使用它。
// 这意味着任何源自主线程的资源都必须将操作延迟到我们自己的线程。尽管实际上允许
// 从不同线程调用资源创建，我们仍然必须延迟它：因为创建 RenderingDevice 的线程
// 可能在主线程开始创建资源时还没有创建它……
//
// 我们必须选择一种策略来确保一致性，因此所有资源都是引用计数的，所有方法都是延迟执行的。
// 使用这些资源的 GPU 任务必须在使用期间持有对它们的引用，而不仅仅是它们的 RID。
// 注意：类似的做法可能是使用更高级别资源（生成器？）的引用计数来实现这一点，
// 而不是使用这些封装？

struct ComputeShaderResourceInternal {
	enum Type {
		TYPE_TEXTURE_2D,
		TYPE_TEXTURE_3D,
		TYPE_STORAGE_BUFFER,
		TYPE_UNINITIALIZED,
		TYPE_DEINITIALIZED //
	};

	// ComputeShaderResource();
	// ComputeShaderResource(ComputeShaderResource &&other);
	// ~ComputeShaderResource();

	void clear(RenderingDevice &rd);
	bool is_valid() const;
	Type get_type() const;
	void create_texture_2d(RenderingDevice &rd, const Image &image);
	void create_texture_2d(RenderingDevice &rd, const Curve &curve);
	void create_texture_3d_float32(RenderingDevice &rd, const PackedByteArray &data, const Vector3i size);
	void create_storage_buffer(RenderingDevice &rd, const PackedByteArray &data);
	void update_storage_buffer(RenderingDevice &rd, const PackedByteArray &data);

	// void operator=(ComputeShaderResource &&src);

	Type type = TYPE_UNINITIALIZED;
	RID rid;
};

class ComputeShaderResource;

// 通常我会把这些函数做成静态方法，但 C++ 允许在对象实例上调用静态方法，
// 这显然是误用。而且 MSVC 无法在通过 shared_ptr 调用时报告 [[nodiscard]] 误用。
// 所以为了绕过这些问题，我们不得不把函数移出来……
struct ComputeShaderResourceFactory {
	ComputeShaderResourceFactory() = delete;

	[[nodiscard]]
	static std::shared_ptr<ComputeShaderResource> create_texture_2d(const Ref<Image> &image);

	[[nodiscard]]
	static std::shared_ptr<ComputeShaderResource> create_texture_2d(const Ref<Curve> &curve);

	[[nodiscard]]
	static std::shared_ptr<ComputeShaderResource> create_texture_3d_zxy(
			Span<const float> fdata_zxy,
			const Vector3i size
	);

	[[nodiscard]]
	static std::shared_ptr<ComputeShaderResource> create_storage_buffer(const PackedByteArray &data);
};

// 必须通过 `ComputeShaderResourceFactory` 创建，并使用 `shared_ptr` 传递
class ComputeShaderResource {
public:
	friend struct ComputeShaderResourceFactory;

	static void update_storage_buffer(const std::shared_ptr<ComputeShaderResource> &res, const PackedByteArray &data);

	ComputeShaderResourceInternal::Type get_type() const;

	~ComputeShaderResource();

	// 仅在 GPU 任务线程上使用
	RID get_rid() const;

private:
	ComputeShaderResourceInternal::Type _type = ComputeShaderResourceInternal::TYPE_UNINITIALIZED;
	ComputeShaderResourceInternal _internal;
};

// 将 3D 变换转换为可用于 GLSL 的 4x4 矩阵布局。
void transform3d_to_mat4(const Transform3D &t, Span<float> dst);

} // namespace voxel

#endif // VOXEL_COMPUTE_SHADER_RESOURCE_H
