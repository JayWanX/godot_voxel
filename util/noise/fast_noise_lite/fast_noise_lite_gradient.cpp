#include "fast_noise_lite_gradient.h"
#include "../../godot/core/array.h"
#ifdef VOXEL_GODOT
#include "../../godot/core/class_db.h"
#endif

namespace voxel {

namespace {

fast_noise_lite::FastNoiseLite::FractalType to_fnl_fractal_type(VOXEL_FastNoiseLiteGradient::FractalType type) {
	switch (type) {
		case VOXEL_FastNoiseLiteGradient::FRACTAL_NONE:
			return fast_noise_lite::FastNoiseLite::FractalType_None;

		case VOXEL_FastNoiseLiteGradient::FRACTAL_DOMAIN_WARP_PROGRESSIVE:
			return fast_noise_lite::FastNoiseLite::FractalType_DomainWarpProgressive;

		case VOXEL_FastNoiseLiteGradient::FRACTAL_DOMAIN_WARP_INDEPENDENT:
			return fast_noise_lite::FastNoiseLite::FractalType_DomainWarpIndependent;

		default:
			ERR_PRINT("Unknown type");
			break;
	}
	return fast_noise_lite::FastNoiseLite::FractalType_None;
}

} // namespace

VOXEL_FastNoiseLiteGradient::VOXEL_FastNoiseLiteGradient() {
	_fn.SetDomainWarpType(static_cast<_FastNoise::DomainWarpType>(_noise_type));
	_fn.SetSeed(_seed);
	_fn.SetFrequency(1.f / _period);
	_fn.SetDomainWarpAmp(_amplitude);

	_fn.SetFractalType(to_fnl_fractal_type(_fractal_type));
	_fn.SetFractalOctaves(_fractal_octaves);
	_fn.SetFractalLacunarity(_fractal_lacunarity);
	_fn.SetFractalGain(_fractal_gain);

	_fn.SetRotationType3D(static_cast<_FastNoise::RotationType3D>(_rotation_type_3d));
}

void VOXEL_FastNoiseLiteGradient::set_noise_type(NoiseType type) {
	if (_noise_type == type) {
		return;
	}
	_noise_type = type;
	_fn.SetDomainWarpType(static_cast<_FastNoise::DomainWarpType>(_noise_type));
	emit_changed();
}

VOXEL_FastNoiseLiteGradient::NoiseType VOXEL_FastNoiseLiteGradient::get_noise_type() const {
	return _noise_type;
}

void VOXEL_FastNoiseLiteGradient::set_seed(int seed) {
	if (_seed == seed) {
		return;
	}
	_seed = seed;
	_fn.SetSeed(seed);
	emit_changed();
}

int VOXEL_FastNoiseLiteGradient::get_seed() const {
	return _seed;
}

void VOXEL_FastNoiseLiteGradient::set_period(float p) {
	if (p < 0.0001f) {
		p = 0.0001f;
	}
	if (_period == p) {
		return;
	}
	_period = p;
	_fn.SetFrequency(1.f / _period);
	emit_changed();
}

float VOXEL_FastNoiseLiteGradient::get_period() const {
	return _period;
}

void VOXEL_FastNoiseLiteGradient::set_amplitude(float amp) {
	if (amp == _amplitude) {
		return;
	}
	_amplitude = amp;
	_fn.SetDomainWarpAmp(amp);
	emit_changed();
}

float VOXEL_FastNoiseLiteGradient::get_amplitude() const {
	return _amplitude;
}

void VOXEL_FastNoiseLiteGradient::set_fractal_type(FractalType type) {
	if (type == _fractal_type) {
		return;
	}
	_fractal_type = type;
	_fn.SetFractalType(to_fnl_fractal_type(_fractal_type));
	emit_changed();
}

VOXEL_FastNoiseLiteGradient::FractalType VOXEL_FastNoiseLiteGradient::get_fractal_type() const {
	return _fractal_type;
}

VOXEL_FastNoiseLiteGradient::_FastNoise::FractalType VOXEL_FastNoiseLiteGradient::get_fractal_type_fnl() const {
	return to_fnl_fractal_type(_fractal_type);
}

void VOXEL_FastNoiseLiteGradient::set_fractal_octaves(int octaves) {
	if (_fractal_octaves == octaves) {
		return;
	}
	_fractal_octaves = octaves;
	_fn.SetFractalOctaves(octaves);
	emit_changed();
}

int VOXEL_FastNoiseLiteGradient::get_fractal_octaves() const {
	return _fractal_octaves;
}

void VOXEL_FastNoiseLiteGradient::set_fractal_lacunarity(float lacunarity) {
	if (_fractal_lacunarity == lacunarity) {
		return;
	}
	_fractal_lacunarity = lacunarity;
	_fn.SetFractalLacunarity(lacunarity);
	emit_changed();
}

float VOXEL_FastNoiseLiteGradient::get_fractal_lacunarity() const {
	return _fractal_lacunarity;
}

void VOXEL_FastNoiseLiteGradient::set_fractal_gain(float gain) {
	if (_fractal_gain == gain) {
		return;
	}
	_fractal_gain = gain;
	_fn.SetFractalGain(gain);
	emit_changed();
}

float VOXEL_FastNoiseLiteGradient::get_fractal_gain() const {
	return _fractal_gain;
}

void VOXEL_FastNoiseLiteGradient::set_rotation_type_3d(RotationType3D type) {
	if (_rotation_type_3d == type) {
		return;
	}
	_rotation_type_3d = type;
	_fn.SetRotationType3D(static_cast<_FastNoise::RotationType3D>(_rotation_type_3d));
	emit_changed();
}

VOXEL_FastNoiseLiteGradient::RotationType3D VOXEL_FastNoiseLiteGradient::get_rotation_type_3d() const {
	return _rotation_type_3d;
}

void VOXEL_FastNoiseLiteGradient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_noise_type", "type"), &VOXEL_FastNoiseLiteGradient::set_noise_type);
	ClassDB::bind_method(D_METHOD("get_noise_type"), &VOXEL_FastNoiseLiteGradient::get_noise_type);

	ClassDB::bind_method(D_METHOD("set_seed", "seed"), &VOXEL_FastNoiseLiteGradient::set_seed);
	ClassDB::bind_method(D_METHOD("get_seed"), &VOXEL_FastNoiseLiteGradient::get_seed);

	ClassDB::bind_method(D_METHOD("set_period", "period"), &VOXEL_FastNoiseLiteGradient::set_period);
	ClassDB::bind_method(D_METHOD("get_period"), &VOXEL_FastNoiseLiteGradient::get_period);

	ClassDB::bind_method(D_METHOD("set_amplitude", "amplitude"), &VOXEL_FastNoiseLiteGradient::set_amplitude);
	ClassDB::bind_method(D_METHOD("get_amplitude"), &VOXEL_FastNoiseLiteGradient::get_amplitude);

	ClassDB::bind_method(D_METHOD("set_fractal_type", "type"), &VOXEL_FastNoiseLiteGradient::set_fractal_type);
	ClassDB::bind_method(D_METHOD("get_fractal_type"), &VOXEL_FastNoiseLiteGradient::get_fractal_type);

	ClassDB::bind_method(D_METHOD("set_fractal_octaves", "octaves"), &VOXEL_FastNoiseLiteGradient::set_fractal_octaves);
	ClassDB::bind_method(D_METHOD("get_fractal_octaves"), &VOXEL_FastNoiseLiteGradient::get_fractal_octaves);

	ClassDB::bind_method(
			D_METHOD("set_fractal_lacunarity", "lacunarity"), &VOXEL_FastNoiseLiteGradient::set_fractal_lacunarity
	);
	ClassDB::bind_method(D_METHOD("get_fractal_lacunarity"), &VOXEL_FastNoiseLiteGradient::get_fractal_lacunarity);

	ClassDB::bind_method(D_METHOD("set_fractal_gain", "gain"), &VOXEL_FastNoiseLiteGradient::set_fractal_gain);
	ClassDB::bind_method(D_METHOD("get_fractal_gain"), &VOXEL_FastNoiseLiteGradient::get_fractal_gain);

	ClassDB::bind_method(D_METHOD("set_rotation_type_3d", "type"), &VOXEL_FastNoiseLiteGradient::set_rotation_type_3d);
	ClassDB::bind_method(D_METHOD("get_rotation_type_3d"), &VOXEL_FastNoiseLiteGradient::get_rotation_type_3d);

	ClassDB::bind_method(D_METHOD("warp_2d", "position"), &VOXEL_FastNoiseLiteGradient::_b_warp_2d);
	ClassDB::bind_method(D_METHOD("warp_3d", "position"), &VOXEL_FastNoiseLiteGradient::_b_warp_3d);

	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "noise_type", PROPERTY_HINT_ENUM, "OpenSimplex2,OpenSimplex2Reduced,Value"),
			"set_noise_type",
			"get_noise_type"
	);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");

	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "period", PROPERTY_HINT_RANGE, "0.0001,10000.0,0.1,or_greater,exp"),
			"set_period",
			"get_period"
	);

	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "amplitude", PROPERTY_HINT_RANGE, "0.0001,10000.0,0.1,exp"),
			"set_amplitude",
			"get_amplitude"
	);

	ADD_GROUP("Fractal", "");

	ADD_PROPERTY(
			PropertyInfo(
					Variant::INT, "fractal_type", PROPERTY_HINT_ENUM, "None,DomainWarpProgressive,DomainWarpIndependent"
			),
			"set_fractal_type",
			"get_fractal_type"
	);

	ADD_PROPERTY(
			PropertyInfo(
					Variant::INT, "fractal_octaves", PROPERTY_HINT_RANGE, String("1,{0},1").format(varray(_MAX_OCTAVES))
			),
			"set_fractal_octaves",
			"get_fractal_octaves"
	);

	ADD_PROPERTY(
			PropertyInfo(Variant::FLOAT, "fractal_lacunarity"), "set_fractal_lacunarity", "get_fractal_lacunarity"
	);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fractal_gain"), "set_fractal_gain", "get_fractal_gain");

	ADD_GROUP("Advanced", "");

	ADD_PROPERTY(
			PropertyInfo(Variant::INT, "rotation_type_3d", PROPERTY_HINT_ENUM, "None,ImproveXYPlanes,ImproveXZPlanes"),
			"set_rotation_type_3d",
			"get_rotation_type_3d"
	);

	BIND_ENUM_CONSTANT(TYPE_OPEN_SIMPLEX_2);
	BIND_ENUM_CONSTANT(TYPE_OPEN_SIMPLEX_2_REDUCED);
	BIND_ENUM_CONSTANT(TYPE_VALUE);

	BIND_ENUM_CONSTANT(FRACTAL_NONE);
	BIND_ENUM_CONSTANT(FRACTAL_DOMAIN_WARP_PROGRESSIVE);
	BIND_ENUM_CONSTANT(FRACTAL_DOMAIN_WARP_INDEPENDENT);

	BIND_ENUM_CONSTANT(ROTATION_3D_NONE);
	BIND_ENUM_CONSTANT(ROTATION_3D_IMPROVE_XY_PLANES);
	BIND_ENUM_CONSTANT(ROTATION_3D_IMPROVE_XZ_PLANES);
}

} // namespace voxel
