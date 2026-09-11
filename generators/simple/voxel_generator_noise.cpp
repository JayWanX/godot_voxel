#include "voxel_generator_noise.h"
#include "../../constants/voxel_string_names.h"
#include <modules/noise/fastnoise_lite.h>
#include "../../util/math/funcs.h"
#include <modules/noise/fastnoise_lite.h>
#include <core/version.h>
#include <core/object/class_db.h>
#include <core/object/callable_mp.h>


namespace voxel {

VoxelGeneratorNoise::VoxelGeneratorNoise() {}

VoxelGeneratorNoise::~VoxelGeneratorNoise() {}

void VoxelGeneratorNoise::set_noise(Ref<FastNoiseLite> noise) {
	if (_noise == noise) {
		return;
	}
	if (_noise.is_valid()) {
		_noise->disconnect(
				VoxelStringNames::get_singleton().changed, callable_mp(this, &VoxelGeneratorNoise::_on_noise_changed)
		);
	}
	_noise = noise;
	Ref<FastNoiseLite> copy;
	if (_noise.is_valid()) {
		_noise->connect(
				VoxelStringNames::get_singleton().changed, callable_mp(this, &VoxelGeneratorNoise::_on_noise_changed)
		);
		// OpenSimplexNoise 资源不是线程安全的，因此我们复制一份供线程使用
		copy = _noise->duplicate();
	}
	// OpenSimplexNoise 资源不是线程安全的，因此我们复制一份供线程使用
	RWLockWrite wlock(_parameters_lock);
	_parameters.noise = copy;
}

void VoxelGeneratorNoise::_on_noise_changed() {
	ERR_FAIL_COND(_noise.is_null());
	RWLockWrite wlock(_parameters_lock);
	_parameters.noise = _noise->duplicate();
}

void VoxelGeneratorNoise::set_channel(VoxelBuffer::ChannelId p_channel) {
	ERR_FAIL_INDEX(p_channel, VoxelBuffer::MAX_CHANNELS);
	bool changed = false;
	{
		RWLockWrite wlock(_parameters_lock);
		if (_parameters.channel != p_channel) {
			_parameters.channel = p_channel;
			changed = true;
		}
	}
	if (changed) {
		emit_changed();
	}
}

VoxelBuffer::ChannelId VoxelGeneratorNoise::get_channel() const {
	RWLockRead rlock(_parameters_lock);
	return _parameters.channel;
}

int VoxelGeneratorNoise::get_used_channels_mask() const {
	RWLockRead rlock(_parameters_lock);
	return (1 << _parameters.channel);
}

Ref<FastNoiseLite> VoxelGeneratorNoise::get_noise() const {
	return _noise;
}

void VoxelGeneratorNoise::set_height_start(real_t y) {
	RWLockWrite wlock(_parameters_lock);
	_parameters.height_start = y;
}

real_t VoxelGeneratorNoise::get_height_start() const {
	RWLockRead rlock(_parameters_lock);
	return _parameters.height_start;
}

void VoxelGeneratorNoise::set_height_range(real_t hrange) {
	if (hrange < 0.1f) {
		hrange = 0.1f;
	}
	RWLockWrite wlock(_parameters_lock);
	_parameters.height_range = hrange;
}

real_t VoxelGeneratorNoise::get_height_range() const {
	RWLockRead rlock(_parameters_lock);
	return _parameters.height_range;
}

/*
// 对于等值面用例，可以通过只计算第一个倍频程来"塑形"噪声，
// 若超出等值面一定距离则丢弃后续倍频程，
// 因为此时我们假设后续倍频程不会改变符号（即不会跨越表面）。
// 这在某些区域可能会降低精度，但能加快结果生成。
static inline float get_shaped_noise(OpenSimplexNoise &noise, float x, float y, float z, float threshold, float bias) {
	x /= noise.get_period();
	y /= noise.get_period();
	z /= noise.get_period();

	float sum = noise._get_octave_noise_3d(0, x, y, z);

	// `threshold` 的默认值应为 `persistence`
	if (sum + bias > threshold || sum + bias < -threshold) {
		// 假设后续倍频程不会改变噪声的符号
		return sum;
	}

	float amp = 1.0;
	float max = 1.0;

	int i = 0;
	while (++i < noise.get_octaves()) {
		x *= noise.get_lacunarity();
		y *= noise.get_lacunarity();
		z *= noise.get_lacunarity();
		amp *= noise.get_persistence();
		max += amp;
		sum += noise._get_octave_noise_3d(i, x, y, z) * amp;
	}

	return sum / max;
}
*/

VoxelGenerator::Result VoxelGeneratorNoise::generate_block(VoxelGenerator::VoxelQueryData input) {
	Parameters params;
	{
		RWLockRead rlock(_parameters_lock);
		params = _parameters;
	}

	ERR_FAIL_COND_V(params.noise.is_null(), Result());

	FastNoiseLite &noise = **params.noise;
	VoxelBuffer &buffer = input.voxel_buffer;
	Vector3i origin_in_voxels = input.origin_in_voxels;
	int lod = input.lod;

	// 我们需要周期来正确产生有符号距离。这就是为什么我们不能随便使用任何噪声，否则就需要
	// 一个用户必须手动调整的额外属性。
	const float noise_period = 1.0 / math::max<real_t>(noise.get_frequency(), 0.0001);

	int isosurface_lower_bound = static_cast<int>(Math::floor(params.height_start));
	int isosurface_upper_bound = static_cast<int>(Math::ceil(params.height_start + params.height_range));

	const int air_type = 0;
	const int matter_type = 1;
	const int air_color = 0;
	const int matter_color = 1;

	Result result;

	if (origin_in_voxels.y >= isosurface_upper_bound) {
		// 用空气填充
		if (params.channel == VoxelBuffer::CHANNEL_SDF) {
			buffer.clear_channel_f(params.channel, 100.0);
		} else if (params.channel == VoxelBuffer::CHANNEL_TYPE) {
			buffer.clear_channel(params.channel, air_type);
		} else if (params.channel == VoxelBuffer::CHANNEL_COLOR) {
			buffer.clear_channel(params.channel, air_color);
		}
		result.max_lod_hint = true;

	} else if (origin_in_voxels.y + (buffer.get_size().y << lod) < isosurface_lower_bound) {
		// 用物质填充
		if (params.channel == VoxelBuffer::CHANNEL_SDF) {
			buffer.clear_channel_f(params.channel, -100.0);
		} else if (params.channel == VoxelBuffer::CHANNEL_TYPE) {
			buffer.clear_channel(params.channel, matter_type);
		} else if (params.channel == VoxelBuffer::CHANNEL_COLOR) {
			buffer.clear_channel(params.channel, matter_color);
		}
		result.max_lod_hint = true;

	} else {
		// const float iso_scale = 0.1f;
		const Vector3i size = buffer.get_size();
		const float height_range_inv = 1.f / params.height_range;
		// const float one_minus_persistence = 1.f - noise.get_persistence();

		for (int z = 0; z < size.z; ++z) {
			int lz = origin_in_voxels.z + (z << lod);

			for (int x = 0; x < size.x; ++x) {
				int lx = origin_in_voxels.x + (x << lod);

				for (int y = 0; y < size.y; ++y) {
					const int ly = origin_in_voxels.y + (y << lod);

					if (ly < isosurface_lower_bound) {
						// 下方只有物质
						if (params.channel == VoxelBuffer::CHANNEL_SDF) {
							// 不是一致的 SDF，但应该可以正常工作
							buffer.set_voxel_f(constants::SDF_FAR_INSIDE, x, y, z, params.channel);
						} else if (params.channel == VoxelBuffer::CHANNEL_TYPE) {
							buffer.set_voxel(matter_type, x, y, z, params.channel);
						} else if (params.channel == VoxelBuffer::CHANNEL_COLOR) {
							buffer.set_voxel(matter_color, x, y, z, params.channel);
						}
						continue;

					} else if (ly >= isosurface_upper_bound) {
						// 上方只有空气
						if (params.channel == VoxelBuffer::CHANNEL_SDF) {
							// 不是一致的 SDF，但应该可以正常工作
							buffer.set_voxel_f(constants::SDF_FAR_OUTSIDE, x, y, z, params.channel);
						} else if (params.channel == VoxelBuffer::CHANNEL_TYPE) {
							buffer.set_voxel(air_type, x, y, z, params.channel);
						} else if (params.channel == VoxelBuffer::CHANNEL_COLOR) {
							buffer.set_voxel(air_color, x, y, z, params.channel);
						}
						continue;
					}

					// 偏差（bias）使我们越往下噪声越表现为"物质"，越往上越表现为"空气"
					const float t = (ly - params.height_start) * height_range_inv;
					const float bias = 2.0 * t - 1.0;

					// 我们接近等值面，需要计算噪声值
					// float n = get_shaped_noise(noise, lx, ly, lz, one_minus_persistence, bias);
					const float n = noise.get_noise_3d(lx, ly, lz);
					// 我们必须将 -1..1 的噪声乘以其周期，以获得更好的有符号距离。不
					// 相乘会导致梯度移动过慢，产生方块感，因为 16 位
					// 编码是为正确的距离场调优的
					const float d = ((n + bias) * noise_period); // * iso_scale;

					if (params.channel == VoxelBuffer::CHANNEL_SDF) {
						buffer.set_voxel_f(d, x, y, z, params.channel);
					} else if (params.channel == VoxelBuffer::CHANNEL_TYPE && d < 0) {
						buffer.set_voxel(matter_type, x, y, z, params.channel);
					} else if (params.channel == VoxelBuffer::CHANNEL_COLOR && d < 0) {
						buffer.set_voxel(matter_color, x, y, z, params.channel);
					}
				}
			}
		}
	}

	return result;
}

void VoxelGeneratorNoise::_b_set_channel(godot::VoxelBuffer::ChannelId p_channel) {
	set_channel(VoxelBuffer::ChannelId(p_channel));
}

godot::VoxelBuffer::ChannelId VoxelGeneratorNoise::_b_get_channel() const {
	return godot::VoxelBuffer::ChannelId(get_channel());
}

void VoxelGeneratorNoise::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_channel", "channel"), &VoxelGeneratorNoise::_b_set_channel);
	ClassDB::bind_method(D_METHOD("get_channel"), &VoxelGeneratorNoise::_b_get_channel);

	ClassDB::bind_method(D_METHOD("set_noise", "noise"), &VoxelGeneratorNoise::set_noise);
	ClassDB::bind_method(D_METHOD("get_noise"), &VoxelGeneratorNoise::get_noise);

	ClassDB::bind_method(D_METHOD("set_height_start", "hstart"), &VoxelGeneratorNoise::set_height_start);
	ClassDB::bind_method(D_METHOD("get_height_start"), &VoxelGeneratorNoise::get_height_start);

	ClassDB::bind_method(D_METHOD("set_height_range", "hrange"), &VoxelGeneratorNoise::set_height_range);
	ClassDB::bind_method(D_METHOD("get_height_range"), &VoxelGeneratorNoise::get_height_range);

	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "channel", PROPERTY_HINT_ENUM, godot::VoxelBuffer::CHANNEL_ID_HINT_STRING),
			"set_channel",
			"get_channel"
	);
	ADD_PROPERTY(
			PropertyInfo(
					Variant::OBJECT,
					"noise",
					PROPERTY_HINT_RESOURCE_TYPE,
					FastNoiseLite::get_class_static(),
					PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT
			),
			"set_noise",
			"get_noise"
	);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_start"), "set_height_start", "get_height_start");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_range"), "set_height_range", "get_height_range");
}

} // namespace voxel
