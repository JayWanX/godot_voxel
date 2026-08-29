#ifndef VOXEL_GPU_TASK_RUNNER_H
#define VOXEL_GPU_TASK_RUNNER_H

#include "../../util/containers/span.h"
#include "../../util/containers/std_vector.h"
#include "../../util/godot/core/packed_byte_array.h"
#include "../../util/godot/core/rid.h"
#include "../../util/godot/macros.h"
#include "../../util/macros.h"
#include "../../util/thread/mutex.h"
#include "../../util/thread/semaphore.h"
#include "../../util/thread/thread.h"

#include "compute_shader.h"
#include "gpu_storage_buffer_pool.h"

#include <atomic>

VOXEL_GODOT_FORWARD_DECLARE(class RenderingDevice)

namespace voxel {

class GPUStorageBufferPool;

struct BaseGPUResources {
	ComputeShaderInternal dilate_normalmap_shader;
	ComputeShaderInternal detail_gather_hits_shader;
	ComputeShaderInternal detail_normalmap_shader;
	ComputeShaderInternal detail_modifier_sphere_shader;
	ComputeShaderInternal detail_modifier_mesh_shader;
	ComputeShaderInternal block_modifier_sphere_shader;
	ComputeShaderInternal block_modifier_mesh_shader;

	RID filtering_sampler_rid;

	void load(RenderingDevice &rd);
	void clear(RenderingDevice &rd);
};

struct GPUTaskContext {
	RenderingDevice &rendering_device;
	GPUStorageBufferPool &storage_buffer_pool;
	const BaseGPUResources &base_resources;

	// 由当前批次中的多个任务共享的缓冲区。
	// 它会在收集前一次性下载，这比分别下载多个单独的缓冲区更快，
	// 因为 Godot 的 API 只提供阻塞调用。
	unsigned int shared_output_buffer_begin = 0; // 以字节为单位
	unsigned int shared_output_buffer_size = 0; // 以字节为单位
	RID shared_output_buffer_rid;
	PackedByteArray downloaded_shared_output_data;

	GPUTaskContext(RenderingDevice &rd, GPUStorageBufferPool &sb_pool, const BaseGPUResources &br) :
			rendering_device(rd), storage_buffer_pool(sb_pool), base_resources(br) {}
};

class IGPUTask {
public:
	virtual ~IGPUTask() {}

	virtual unsigned int get_required_shared_output_buffer_size() const {
		return 0;
	}

	virtual void prepare(GPUTaskContext &ctx) = 0;
	virtual void collect(GPUTaskContext &ctx) = 0;
};

// 运行调度计算着色器并收集其结果的任务。
class GPUTaskRunner {
public:
	GPUTaskRunner();
	~GPUTaskRunner();

	void start();
	void stop();
	void push(IGPUTask *task);
	unsigned int get_pending_task_count() const;
	bool is_running() const;

private:
	void thread_func();

	RenderingDevice *_rendering_device = nullptr;
	// mutable Mutex _rendering_device_ptr_mutex;

	GPUStorageBufferPool _storage_buffer_pool;
	BaseGPUResources _base_resources;

	// 要运行的任务队列。它们将按提交顺序运行。
	StdVector<IGPUTask *> _shared_tasks;
	Mutex _mutex;
	Semaphore _semaphore;
	// 之所以使用线程，是因为到目前为止，使用 RenderingDevice 提交和接收数据的唯一方式似乎是
	// 阻塞调用线程并等待显卡……
	// 由于我们已经有一个线程池，这个线程大部分时间应该处于休眠或等待状态。
	Thread _thread;
	bool _running = false;
	std::atomic_uint32_t _pending_count = 0;
};

} // namespace voxel

#endif // VOXEL_GPU_TASK_RUNNER_H
