#include "funcs.h"
#include "../meshers/blocky/voxel_blocky_library_base.h"
#include "../storage/voxel_data.h"
#include "../util/containers/dynamic_bitset.h"
#include "../util/containers/span.h"
#include "../util/containers/std_vector.h"
#include <core/math/random_pcg.h>
#include "../util/profiling.h"
#include "../util/string/format.h"
#include <core/math/random_pcg.h>


namespace voxel {

void copy_from_chunked_storage(
		VoxelBuffer &dst_buffer,
		const Vector3i min_pos,
		const unsigned int block_size_po2,
		const uint32_t channels_mask,
		const VoxelBuffer *(*get_block_func)(void *, Vector3i),
		void *get_block_func_ctx,
		const bool with_metadata
) {
	VOXEL_ASSERT_RETURN_MSG(Vector3iUtil::get_volume_u64(dst_buffer.get_size()) > 0, "The area to copy is empty");
	VOXEL_ASSERT_RETURN(get_block_func != nullptr);

	const Vector3i max_pos = min_pos + dst_buffer.get_size();

	const Vector3i min_block_pos = min_pos >> block_size_po2;
	const Vector3i max_block_pos = ((max_pos - Vector3i(1, 1, 1)) >> block_size_po2) + Vector3i(1, 1, 1);

	const Vector3i block_size_v = Vector3iUtil::create(1 << block_size_po2);

	const SmallVector<uint8_t, VoxelBuffer::MAX_CHANNELS> channels = VoxelBuffer::mask_to_channels_list(channels_mask);

	Vector3i bpos;
	for (bpos.z = min_block_pos.z; bpos.z < max_block_pos.z; ++bpos.z) {
		for (bpos.x = min_block_pos.x; bpos.x < max_block_pos.x; ++bpos.x) {
			for (bpos.y = min_block_pos.y; bpos.y < max_block_pos.y; ++bpos.y) {
				const VoxelBuffer *src_buffer = (*get_block_func)(get_block_func_ctx, bpos);
				const Vector3i src_block_origin = bpos << block_size_po2;

				if (src_buffer != nullptr) {
					for (const uint8_t channel : channels) {
						dst_buffer.set_channel_depth(channel, src_buffer->get_channel_depth(channel));
						// 注意：copy_from 会处理区域位于边缘时的钳制
						dst_buffer.copy_channel_from(
								*src_buffer, min_pos - src_block_origin, src_buffer->get_size(), Vector3i(), channel
						);
					}

					if (with_metadata) {
						dst_buffer.copy_voxel_metadata_in_area(
								*src_buffer,
								Box3i::from_min_max(min_pos - src_block_origin, src_buffer->get_size()),
								Vector3i()
						);
					}

				} else {
					for (const uint8_t channel : channels) {
						// 目前，不存在的块默认使用硬编码的默认值，对应“空空间”。
						// 如果我们要改变这一点，可能需要为此添加一个 API。
						dst_buffer.fill_area(
								VoxelBuffer::get_default_raw_value(
										static_cast<VoxelBuffer::ChannelId>(channel),
										dst_buffer.get_channel_depth(channel)
								),
								src_block_origin - min_pos,
								src_block_origin - min_pos + block_size_v,
								channel
						);
					}
				}
			}
		}
	}
}

void paste_to_chunked_storage(
		const VoxelBuffer &src_buffer,
		Vector3i min_pos,
		unsigned int block_size_po2,
		unsigned int channels_mask,
		bool use_mask,
		uint8_t mask_channel,
		uint64_t mask_value,
		VoxelBuffer *(*get_block_func)(void *, Vector3i),
		void *get_block_func_ctx
) {
	VOXEL_ASSERT_RETURN(get_block_func != nullptr);
	const Vector3i max_pos = min_pos + src_buffer.get_size();

	const Vector3i min_block_pos = min_pos >> block_size_po2;
	const Vector3i max_block_pos = ((max_pos - Vector3i(1, 1, 1)) >> block_size_po2) + Vector3i(1, 1, 1);

	const SmallVector<uint8_t, VoxelBuffer::MAX_CHANNELS> channels = VoxelBuffer::mask_to_channels_list(channels_mask);

	Vector3i bpos;
	for (bpos.z = min_block_pos.z; bpos.z < max_block_pos.z; ++bpos.z) {
		for (bpos.x = min_block_pos.x; bpos.x < max_block_pos.x; ++bpos.x) {
			for (bpos.y = min_block_pos.y; bpos.y < max_block_pos.y; ++bpos.y) {
				VoxelBuffer *dst_buffer = (*get_block_func)(get_block_func_ctx, bpos);

				if (dst_buffer == nullptr) {
					continue;
				}

				const Vector3i dst_block_origin = bpos << block_size_po2;
				const Vector3i dst_base_pos = min_pos - dst_block_origin;

				if (use_mask) {
					paste_src_masked(
							to_span(channels), src_buffer, mask_channel, mask_value, *dst_buffer, dst_base_pos, true
					);

				} else {
					paste(to_span(channels), src_buffer, *dst_buffer, dst_base_pos, true);
				}
			}
		}
	}
}

AABB get_path_aabb(Span<const Vector3> positions, Span<const float> radii) {
	AABB aabb(positions[0], Vector3());

	for (unsigned int i = 0; i < positions.size(); ++i) {
		const Vector3 pos = positions[i];
		const float r = radii[i];
		const Vector3 extentv(r, r, r);
		aabb = aabb.expand(pos - extentv);
		aabb = aabb.expand(pos + extentv);
	}

	return aabb;
}

void run_blocky_random_tick(
		VoxelData &data,
		const Box3i voxel_box,
		const VoxelBlockyLibraryBase &lib,
		RandomPCG &random,
		const int voxel_count,
		const int batch_count,
		const uint32_t tags_mask,
		void *callback_data,
		bool (*callback)(void *, Vector3i, int64_t)
) {
	ERR_FAIL_COND(batch_count <= 0);
	ERR_FAIL_COND(voxel_count < 0);
	ERR_FAIL_COND(!math::is_valid_size(voxel_box.size));
	ERR_FAIL_COND(callback == nullptr);

	constexpr unsigned int lod_index = 0;

	const unsigned int block_size = data.get_block_size();
	const Box3i block_box = voxel_box.downscaled(block_size);

	const int block_count = math::ceildiv(voxel_count, batch_count);

	// 处理体素数量不是批次数量倍数时的余数
	const int batch_rem = voxel_count % batch_count;
	const int last_batch_count = batch_rem > 0 ? batch_rem : batch_count;

	// const int bs_mask = map.get_block_size_mask();
	const VoxelBuffer::ChannelId channel = VoxelBuffer::CHANNEL_TYPE;

	struct Pick {
		uint64_t value;
		Vector3i rpos;
	};
	// TODO 可作为临时分配器的候选
	static thread_local StdVector<Pick> picks;
	picks.reserve(batch_count);

	const float block_volume = math::cubed(block_size);
	CRASH_COND(block_volume < 0.1f);

	struct L {
		static inline int urand(RandomPCG &random, uint32_t max_value) {
			return random.rand() % max_value;
		}
		static inline Vector3i urand_vec3i(RandomPCG &random, Vector3i s) {
#ifdef DEBUG_ENABLED
			CRASH_COND(s.x <= 0 || s.y <= 0 || s.z <= 0);
#endif
			return Vector3i(urand(random, s.x), urand(random, s.y), urand(random, s.z));
		}
	};

	const blocky::BakedLibrary &lib_data = lib.get_baked_data();

	// 随机选择块
	for (int block_index = 0; block_index < block_count; ++block_index) {
		const Vector3i block_pos = block_box.position + L::urand_vec3i(random, block_box.size);

		const Vector3i block_origin = data.block_to_voxel(block_pos);

		picks.clear();

		{
			SpatialLock3D &spatial_lock = data.get_spatial_lock(lod_index);
			SpatialLock3D::Read srlock(spatial_lock, BoxBounds3i::from_position(block_pos));

			std::shared_ptr<VoxelBuffer> voxels_ptr = data.try_get_block_voxels(block_pos);

			if (voxels_ptr != nullptr) {
				// 这里只进行读取。
				const VoxelBuffer &voxels = *voxels_ptr;

				if (voxels.get_channel_compression(channel) == VoxelBuffer::COMPRESSION_UNIFORM) {
					const uint64_t v = voxels.get_voxel(0, 0, 0, channel);
					if (lib_data.has_model(v)) {
						const blocky::BakedModel &vt = lib_data.models[v];
						if (vt.is_random_tickable == false || (vt.tags_mask & tags_mask) == 0) {
							// 跳过整个块
							continue;
						}
					}
				}

				const Box3i block_voxel_box(block_origin, Vector3iUtil::create(block_size));
				Box3i local_voxel_box = voxel_box.clipped(block_voxel_box);
				local_voxel_box.position -= block_origin;
				const float volume_ratio = Vector3iUtil::get_volume_u64(local_voxel_box.size) / block_volume;
				const int local_batch_count_full_block =
						(block_index == block_count - 1 ? last_batch_count : batch_count);
				const int local_batch_count = Math::ceil(local_batch_count_full_block * volume_ratio);

				// 在块内随机选择一批体素。
				// 这样分批处理通过减少块查找次数，略微提升性能。
				for (int vi = 0; vi < local_batch_count; ++vi) {
					const Vector3i rpos = local_voxel_box.position + L::urand_vec3i(random, local_voxel_box.size);

					const uint64_t v = voxels.get_voxel(rpos, channel);
					picks.push_back(Pick{ v, rpos });
				}
			}
		}

		// 由于向脚本开放，以下操作可能随机读取和写入体素。
		// 不过我们不直接发送缓冲区，而是通过负责加锁的 API。
		// 所以我们在这里不需要（也不应该）加锁。
		for (size_t i = 0; i < picks.size(); ++i) {
			const Pick pick = picks[i];

			if (lib_data.has_model(pick.value)) {
				const blocky::BakedModel &vt = lib_data.models[pick.value];

				if (vt.is_random_tickable && (vt.tags_mask & tags_mask) != 0) {
					ERR_FAIL_COND(!callback(callback_data, pick.rpos + block_origin, pick.value));
				}
			}
		}
	}
}

void run_blocky_random_tick(
		VoxelData &data,
		const AABB voxel_box_f,
		const VoxelBlockyLibraryBase &lib,
		RandomPCG &random,
		const int voxel_count,
		const int batch_count,
		const uint32_t tags_mask,
		const Callable &callback
) {
	struct CallbackData {
		const Callable &callable;
	};
	CallbackData cb_self{ callback };

	const Box3i voxel_box(math::floor_to_int(voxel_box_f.position), math::floor_to_int(voxel_box_f.size));

	voxel::run_blocky_random_tick(
			data,
			voxel_box,
			lib,
			random,
			voxel_count,
			batch_count,
			tags_mask,
			&cb_self,
			[](void *self, Vector3i pos, int64_t val) {
				const CallbackData *cd = reinterpret_cast<const CallbackData *>(self);
				const Variant vpos = pos;
				const Variant vv = val;
				const Variant *args[2];
				args[0] = &vpos;
				args[1] = &vv;
				Callable::CallError error;
				Variant retval; // 我们不关心返回值，Callable API 需要它
				cd->callable.callp(args, 2, retval, error);
				// TODO 我很想知道报告这类错误的正确方式……
				// 我在引擎中找到的示例并不一致
				ERR_FAIL_COND_V(error.error != Callable::CallError::CALL_OK, false);
		// 失败时返回，我们不想看到错误刷屏
				return true;
			}
	);
}

bool indices_to_bitarray_u16(Span<const int32_t> indices, DynamicBitset &bitarray) {
#ifdef DEBUG_ENABLED
	const int32_t max_supported_value = 65535;
	// 校验
	for (const int32_t i : indices) {
		VOXEL_ASSERT_RETURN_V_MSG(
				i >= 0 && i <= max_supported_value,
				false,
				format("Index {} is out of supported range 0..{}", i, max_supported_value)
		);
	}
#endif

	int32_t max_value = -1;
	for (const int32_t i : indices) {
		max_value = math::max(i, max_value);
	}

	bitarray.resize_no_init(max_value + 1);

	for (const int32_t i : indices) {
		bitarray.set(i);
	}

	return true;
}

void indices_to_bitarray(Span<const uint8_t> indices, DynamicBitset &bitarray) {
	if (indices.size() == 0) {
		bitarray.clear();
	}

	uint8_t max_value = 0;
	for (const uint8_t i : indices) {
		max_value = math::max(i, max_value);
	}

	bitarray.resize_no_init(max_value + 1);

	for (const uint8_t i : indices) {
		bitarray.set(i);
	}
}

} // namespace voxel

namespace voxel::ops {

Box3i get_round_cone_int_bounds(Vector3f p0, Vector3f p1, float r0, float r1) {
	const Vector3f minp(
			math::min(p0.x - r0, p1.x - r1), //
			math::min(p0.y - r0, p1.y - r1), //
			math::min(p0.z - r0, p1.z - r1)
	);

	const Vector3f maxp(
			math::max(p0.x + r0, p1.x + r1), //
			math::max(p0.y + r0, p1.y + r1), //
			math::max(p0.z + r0, p1.z + r1)
	);

	return Box3i::from_min_max(to_vec3i(math::floor(minp)), to_vec3i(math::ceil(maxp)));
}

#if defined(DEBUG_ENABLED) || defined(VOXEL_TESTS)

// 参考实现。正确但非常慢。
void box_blur_slow_ref(const VoxelBuffer &src, VoxelBuffer &dst, int radius, Vector3f sphere_pos, float sphere_radius) {
	VOXEL_PROFILE_SCOPE();

	const Vector3i dst_size = src.get_size() - Vector3i(radius, radius, radius) * 2;

	VOXEL_ASSERT_RETURN(dst_size.x >= 0);
	VOXEL_ASSERT_RETURN(dst_size.y >= 0);
	VOXEL_ASSERT_RETURN(dst_size.z >= 0);

	dst.create(dst_size);

	const int box_size = radius * 2 + 1;
	const float box_volume = Vector3iUtil::get_volume_u64(Vector3i(box_size, box_size, box_size));

	const float sphere_radius_s = sphere_radius * sphere_radius;

	Vector3i dst_pos;
	for (dst_pos.z = 0; dst_pos.z < dst.get_size().z; ++dst_pos.z) {
		for (dst_pos.x = 0; dst_pos.x < dst.get_size().x; ++dst_pos.x) {
			for (dst_pos.y = 0; dst_pos.y < dst.get_size().y; ++dst_pos.y) {
				const float sd_src =
						src.get_voxel_f(dst_pos + Vector3i(radius, radius, radius), VoxelBuffer::CHANNEL_SDF);

				const float sphere_ds = math::distance_squared(sphere_pos, to_vec3f(dst_pos));
				if (sphere_ds > sphere_radius_s) {
					// 超出笔刷范围
					dst.set_voxel_f(sd_src, dst_pos, VoxelBuffer::CHANNEL_SDF);
					continue;
				}
				// 笔刷系数
				const float factor = math::clamp(1.f - sphere_ds / sphere_radius_s, 0.f, 1.f);

				const Vector3i src_min = dst_pos; // - Vector3i(radius, radius, radius);
				const Vector3i src_max = src_min + Vector3i(box_size, box_size, box_size);

				Vector3i src_pos;
				float sd_sum = 0.f;
				for (src_pos.z = src_min.z; src_pos.z < src_max.z; ++src_pos.z) {
					for (src_pos.x = src_min.x; src_pos.x < src_max.x; ++src_pos.x) {
						for (src_pos.y = src_min.y; src_pos.y < src_max.y; ++src_pos.y) {
							// 这是热点代码。可以通过分离 XYZ 模糊并将读取结果缓存在
							// 环形缓冲区中来优化
							sd_sum += src.get_voxel_f(src_pos, VoxelBuffer::CHANNEL_SDF);
						}
					}
				}

				const float sd_avg = sd_sum / box_volume;
				const float sd = Math::lerp(sd_src, sd_avg, factor);

				dst.set_voxel_f(sd, dst_pos, VoxelBuffer::CHANNEL_SDF);
			}
		}
	}
}

#endif

void box_blur(const VoxelBuffer &src, VoxelBuffer &dst, int radius, Vector3f sphere_pos, float sphere_radius) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(radius >= 1);

	const Vector3i dst_size = src.get_size() - Vector3i(radius, radius, radius) * 2;

	VOXEL_ASSERT_RETURN(dst_size.x >= 0);
	VOXEL_ASSERT_RETURN(dst_size.y >= 0);
	VOXEL_ASSERT_RETURN(dst_size.z >= 0);

	dst.create(dst_size);

	const int box_size = radius * 2 + 1;
	const float box_size_f = box_size;
	// const float box_volume = Vector3iUtil::get_volume(Vector3i(box_size, box_size, box_size));

	const float sphere_radius_s = sphere_radius * sphere_radius;

	// 盒式模糊是可分离的：我们可以先沿第一个轴做一维模糊，然后是第二个轴，再是第三个
	// 轴。这减少了内存访问次数，并将算法简化为 1-D。

	// 由于分离后的模糊是一维的，我们可以使用环形缓冲区来优化读取/累加，因为每次
	// 迭代只是将平均窗口移动 1 个体素。因此无需在每次迭代时收集所有值来求平均，
	// 我们只需加入一个并移除一个。
	StdVector<float> ring_buffer;
	const unsigned int rb_power = math::get_next_power_of_two_32_shift(box_size);
	const unsigned int rb_len = 1 << rb_power;
	ring_buffer.resize(rb_len);
	const unsigned int rb_mask = rb_len - 1;
	VOXEL_ASSERT(static_cast<int>(ring_buffer.size()) >= box_size);

	// 在两个轴上带额外长度的临时缓冲区
	StdVector<float> tmp;
	const Vector3i tmp_size(dst_size.x + 2 * radius, dst_size.y, dst_size.z + 2 * radius);
	tmp.resize(Vector3iUtil::get_volume_u64(tmp_size));

	// Y 轴模糊
	Vector3i dst_pos;
	unsigned int tmp_stride = 1;
	unsigned int tmp_i = 0;
	{
		VOXEL_PROFILE_SCOPE_NAMED("Y blur");
		for (dst_pos.z = 0; dst_pos.z < tmp_size.z; ++dst_pos.z) {
			for (dst_pos.x = 0; dst_pos.x < tmp_size.x; ++dst_pos.x) {
				float sd_sum = 0.f;
				// 用初始样本填充窗口
				for (int y = 0; y < box_size; ++y) {
					// TODO 第一个轴这样采样的方式使其比其他轴慢得多。
					// 是否将 tmp 放大以容纳整个尺寸并先转换进去？
					const float sd = src.get_voxel_f(Vector3i(dst_pos.x, y, dst_pos.z), VoxelBuffer::CHANNEL_SDF);
					ring_buffer[y] = sd;
					sd_sum += sd;
				}

				tmp[tmp_i] = sd_sum / box_size_f;
				tmp_i += tmp_stride;

				// 环形缓冲区中的读/写游标：
				// 假定窗口“从左向右”移动
				int rbr = 0; // 窗口中最左侧的样本
				int rbw = box_size & rb_mask; // 窗口中最右侧样本的下一个位置

				for (dst_pos.y = 1; dst_pos.y < tmp_size.y; ++dst_pos.y) {
					// 提前 2*radius 查看，因为我们从一个在 Y 方向上也比 tmp 大的缓冲区采样
					const float sd = src.get_voxel_f(
							Vector3i(dst_pos.x, dst_pos.y + radius * 2, dst_pos.z), VoxelBuffer::CHANNEL_SDF
					);
					// 移除离开窗口的样本
					sd_sum -= ring_buffer[rbr];
					// 添加进入窗口的样本
					sd_sum += sd;
					ring_buffer[rbw] = sd;
					// 将读和写游标向前推进 1。掩码处理环形缓冲区的环绕（比取模更快）
					rbr = (rbr + 1) & rb_mask;
					rbw = (rbw + 1) & rb_mask;

					tmp[tmp_i] = sd_sum / box_size_f;
					tmp_i += tmp_stride;
				}
			}
		}
	}

	// X 轴模糊
	{
		VOXEL_PROFILE_SCOPE_NAMED("X blur");
		tmp_stride = tmp_size.y;
		for (dst_pos.z = 0; dst_pos.z < tmp_size.z; ++dst_pos.z) {
			for (dst_pos.y = 0; dst_pos.y < tmp_size.y; ++dst_pos.y) {
				// 在每一行上初始化，因为这次我们不做完全规则的访问
				tmp_i = Vector3iUtil::get_zxy_index(Vector3i(0, dst_pos.y, dst_pos.z), tmp_size);

				float sd_sum = 0.f;
				for (int x = 0; x < box_size; ++x) {
					// 这次我们直接从临时缓冲区本身读取样本。
					// 我们在读取缓冲区的同时进行写入，但这应该没问题，因为我们只读取
					// 在它们被修改之前的值，并且当我们到达这些值时不会追溯性地影响结果，因为
					// 环形缓冲区充当这些值的副本。
					const float sd = tmp[tmp_i + x * tmp_stride];
					ring_buffer[x] = sd;
					sd_sum += sd;
				}

				// 仅对最终区域计算 X 轴模糊（我们最初保留邻居是为了捕获沿
				// Y 轴方向的模糊样本，X 和 Z 将使用这些样本）

				tmp_i += radius * tmp_stride; // 按 +(radius, 0, 0) 跳跃
				tmp[tmp_i] = sd_sum / box_size_f;

				int rbr = 0;
				int rbw = box_size & rb_mask;

				for (dst_pos.x = radius + 1; dst_pos.x < tmp_size.x - radius; ++dst_pos.x) {
					tmp_i += tmp_stride;

					const float sd = tmp[tmp_i + radius * tmp_stride];
					sd_sum -= ring_buffer[rbr];
					sd_sum += sd;
					ring_buffer[rbw] = sd;
					// 将读和写游标向前推进 1
					rbr = (rbr + 1) & rb_mask;
					rbw = (rbw + 1) & rb_mask;

					tmp[tmp_i] = sd_sum / box_size_f;
				}
			}
		}
	}

	// Z 轴模糊
	{
		VOXEL_PROFILE_SCOPE_NAMED("Z blur");
		tmp_stride = tmp_size.y * tmp_size.x;
		for (dst_pos.x = radius; dst_pos.x < tmp_size.x - radius; ++dst_pos.x) {
			for (dst_pos.y = 0; dst_pos.y < tmp_size.y; ++dst_pos.y) {
				tmp_i = Vector3iUtil::get_zxy_index(Vector3i(dst_pos.x, dst_pos.y, 0), tmp_size);

				float sd_sum = 0.f;
				for (int z = 0; z < box_size; ++z) {
					const float sd = tmp[tmp_i + z * tmp_stride];
					ring_buffer[z] = sd;
					sd_sum += sd;
				}

				tmp_i += radius * tmp_stride; // 按 +(0, 0, radius) 跳跃
				tmp[tmp_i] = sd_sum / box_size_f;

				int rbr = 0;
				int rbw = box_size & rb_mask;

				for (dst_pos.z = radius + 1; dst_pos.z < tmp_size.z - radius; ++dst_pos.z) {
					tmp_i += tmp_stride;

					const float sd = tmp[tmp_i + radius * tmp_stride];
					sd_sum -= ring_buffer[rbr];
					sd_sum += sd;
					ring_buffer[rbw] = sd;
					// 将读和写游标向前推进 1
					rbr = (rbr + 1) & rb_mask;
					rbw = (rbw + 1) & rb_mask;

					tmp[tmp_i] = sd_sum / box_size_f;
				}
			}
		}
	}

	// 使用形状进行混合

	{
		VOXEL_PROFILE_SCOPE_NAMED("Blend");
		for (dst_pos.z = 0; dst_pos.z < dst_size.z; ++dst_pos.z) {
			for (dst_pos.x = 0; dst_pos.x < dst_size.x; ++dst_pos.x) {
				for (dst_pos.y = 0; dst_pos.y < dst_size.y; ++dst_pos.y) {
					//
					const Vector3i src_pos = dst_pos + Vector3i(radius, radius, radius);
					// TODO 也许可以优化这次读取
					const float src_sd = src.get_voxel_f(src_pos, VoxelBuffer::CHANNEL_SDF);

					const float sphere_ds = math::distance_squared(sphere_pos, to_vec3f(dst_pos));
					if (sphere_ds > sphere_radius_s) {
						// 超出笔刷范围
						dst.set_voxel_f(src_sd, dst_pos, VoxelBuffer::CHANNEL_SDF);
						continue;
					}

					// 笔刷系数
					const float factor = math::clamp(1.f - sphere_ds / sphere_radius_s, 0.f, 1.f);

					const Vector3i tmp_pos(dst_pos.x + radius, dst_pos.y, dst_pos.z + radius);
					const unsigned int tmp_loc = Vector3iUtil::get_zxy_index(tmp_pos, tmp_size);
					const float tmp_sd = tmp[tmp_loc];

					const float sd = Math::lerp(src_sd, tmp_sd, factor);
					dst.set_voxel_f(sd, dst_pos, VoxelBuffer::CHANNEL_SDF);
				}
			}
		}
	}
}

void grow_sphere(VoxelBuffer &src, float strength, Vector3f sphere_pos, float sphere_radius) {
	VOXEL_PROFILE_SCOPE();
	VOXEL_ASSERT_RETURN(sphere_radius > 0.001f);

	const Vector3i src_size = src.get_size();

	VOXEL_ASSERT_RETURN(src_size.x >= 0);
	VOXEL_ASSERT_RETURN(src_size.y >= 0);
	VOXEL_ASSERT_RETURN(src_size.z >= 0);

	const float sphere_radius_squared = sphere_radius * sphere_radius;
	const float inv_sphere_radius = 1.f / sphere_radius;

	Vector3i src_pos;
	for (src_pos.z = 0; src_pos.z < src_size.z; ++src_pos.z) {
		for (src_pos.x = 0; src_pos.x < src_size.x; ++src_pos.x) {
			for (src_pos.y = 0; src_pos.y < src_size.y; ++src_pos.y) {
				const float src_sd = src.get_voxel_f(src_pos, VoxelBuffer::CHANNEL_SDF);

				const float sphere_ds = math::distance_squared(sphere_pos, to_vec3f(src_pos));
				if (sphere_ds > sphere_radius_squared) {
					// 超出笔刷范围
					continue;
				}

				const float distance = Math::sqrt(sphere_ds);
				const float sd_offset = strength * (sphere_radius - distance) * inv_sphere_radius;

				// 在有符号距离场中，相减会使形状“生长”。
				// 允许负强度，这样也可以用于“收缩”。
				src.set_voxel_f(src_sd - sd_offset, src_pos, VoxelBuffer::CHANNEL_SDF);
			}
		}
	}
}

} // namespace voxel::ops
