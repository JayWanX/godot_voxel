#ifndef VOXEL_FAST_NOISE_2_H
#define VOXEL_FAST_NOISE_2_H

#ifndef VOXEL_ENABLE_FAST_NOISE_2
#error "FastNoise2 is disabled, it shouldn't be #included"
#endif

#include "../containers/span.h"
#include "../math/interval.h"

#if defined(__GNUC__) && !defined(__clang__)
// FastNoise2 使用了虚继承，但 Godot 4.5 添加了一个警告来强制禁止使用它。
// 参见 https://github.com/godotengine/godot/pull/103708
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvirtual-inheritance"
#endif

#include "FastNoise/FastNoise.h"

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <core/io/resource.h>

class Image;

namespace voxel {

// 不能叫 FastNoise？因为 FastNoise 已经是一个命名空间了
class FastNoise2 : public Resource {
	GDCLASS(FastNoise2, Resource)
public:
	static const int MAX_OCTAVES = 32;
	// 这个最小尺寸是为了修复 SIMD 操作的问题，它们需要使用不止一个元素。
	static const unsigned int MIN_BUFFER_SIZE = 16;
	static const int MAX_CELLULAR_INDEX = 3;

	enum SIMDLevel {
		SIMD_NULL = FastSIMD::Level_Null, // 未初始化
		SIMD_SCALAR = FastSIMD::Level_Scalar, // 80386 指令集（非 SIMD）
		SIMD_SSE = FastSIMD::Level_SSE, // CPU 支持 SSE（XMM）（不检测操作系统支持）
		SIMD_SSE2 = FastSIMD::Level_SSE2, // SSE2
		SIMD_SSE3 = FastSIMD::Level_SSE3, // SSE3
		SIMD_SSSE3 = FastSIMD::Level_SSSE3, // 补充版 SSE3（SSSE3）
		SIMD_SSE41 = FastSIMD::Level_SSE41, // SSE4.1
		SIMD_SSE42 = FastSIMD::Level_SSE42, // SSE4.2
		SIMD_AVX = FastSIMD::Level_AVX, // CPU 和操作系统均支持 AVX
		SIMD_AVX2 = FastSIMD::Level_AVX2, // AVX2
		SIMD_AVX512 = FastSIMD::Level_AVX512, // CPU 和操作系统均支持 AVX512、AVX512DQ

		SIMD_NEON = FastSIMD::Level_NEON, // ARM NEON
	};

	enum NoiseType { //
		TYPE_OPEN_SIMPLEX_2 = 0,
		TYPE_SIMPLEX,
		TYPE_PERLIN,
		TYPE_VALUE,
		TYPE_CELLULAR,
		// 特殊类型，用 Auburn 的 NoiseTool 制作的节点树覆盖大多数选项
		TYPE_ENCODED_NODE_TREE,
		TYPE_CELLULAR_VALUE,
		// TODO 在 Godot 内部实现 NoiseTool 图形编辑器？
		// TYPE_NODE_TREE,
	};

	static constexpr const char *NOISE_TYPE_HINT_STRING =
			"OpenSimplex2,Simplex,Perlin,Value,Cellular,EncodedNodeTree,CellularValue";

	enum FractalType { //
		FRACTAL_NONE = 0,
		FRACTAL_FBM,
		FRACTAL_RIDGED,
		FRACTAL_PING_PONG
	};

	static constexpr const char *FRACTAL_TYPE_HINT_STRING = "None,FBm,Ridged,PingPong";

	enum CellularDistanceFunction { //
		CELLULAR_DISTANCE_EUCLIDEAN = (int)FastNoise::DistanceFunction::Euclidean,
		CELLULAR_DISTANCE_EUCLIDEAN_SQ = (int)FastNoise::DistanceFunction::EuclideanSquared,
		CELLULAR_DISTANCE_MANHATTAN = (int)FastNoise::DistanceFunction::Manhattan,
		CELLULAR_DISTANCE_HYBRID = (int)FastNoise::DistanceFunction::Hybrid,
		CELLULAR_DISTANCE_MAX_AXIS = (int)FastNoise::DistanceFunction::MaxAxis
	};

	static constexpr const char *CELLULAR_DISTANCE_FUNCTION_HINT_STRING =
			"Euclidean,EuclideanSq,Manhattan,Hybrid,MaxAxis";

	enum CellularReturnType { //
		CELLULAR_RETURN_INDEX_0 = (int)FastNoise::CellularDistance::ReturnType::Index0,
		CELLULAR_RETURN_INDEX_0_ADD_1 = (int)FastNoise::CellularDistance::ReturnType::Index0Add1,
		CELLULAR_RETURN_INDEX_0_SUB_1 = (int)FastNoise::CellularDistance::ReturnType::Index0Sub1,
		CELLULAR_RETURN_INDEX_0_MUL_1 = (int)FastNoise::CellularDistance::ReturnType::Index0Mul1,
		CELLULAR_RETURN_INDEX_0_DIV_1 = (int)FastNoise::CellularDistance::ReturnType::Index0Div1
	};

	static constexpr const char *CELLULAR_RETURN_TYPE_HINT_STRING =
			"Index0,Index0Add1,Index0Sub1,Index0Mul1,Index0Div1";

	FastNoise2();

	// SIMD 级别
	SIMDLevel get_simd_level() const;
	// 获取 SIMD 级别的名称（C 字符串）
	static const char *get_simd_level_name_c_str(SIMDLevel level);
	// 获取 SIMD 级别的名称
	static String get_simd_level_name(SIMDLevel level);

	// 随机种子
	void set_seed(int seed);
	int get_seed() const;

	// 噪声类型
	void set_noise_type(NoiseType type);
	NoiseType get_noise_type() const;

	// 周期（频率的倒数）
	void set_period(float p);
	float get_period() const;

	// 分形

	// 分形类型
	void set_fractal_type(FractalType type);
	FractalType get_fractal_type() const;

	// 分形倍频程数
	void set_fractal_octaves(int octaves);
	int get_fractal_octaves() const;

	// 分形点隙（lacunarity）
	void set_fractal_lacunarity(float lacunarity);
	float get_fractal_lacunarity() const;

	// 分形增益
	void set_fractal_gain(float gain);
	float get_fractal_gain() const;

	// 乒乓（ping-pong）强度
	void set_fractal_ping_pong_strength(float s);
	float get_fractal_ping_pong_strength() const;

	// 梯田修饰器

	// 是否启用梯田修饰
	void set_terrace_enabled(bool enable);
	bool is_terrace_enabled() const;

	// 梯田倍率
	void set_terrace_multiplier(float m);
	float get_terrace_multiplier() const;

	// 梯田阶跃的平滑度
	void set_terrace_smoothness(float s);
	float get_terrace_smoothness() const;

	// 重映射

	// 是否启用输出值重映射
	void set_remap_enabled(bool enabled);
	bool is_remap_enabled() const;

	// 重映射输入最小值
	void set_remap_input_min(float min_value);
	float get_remap_input_min() const;

	// 重映射输入最大值
	void set_remap_input_max(float max_value);
	float get_remap_input_max() const;

	// 重映射输出最小值
	void set_remap_output_min(float min_value);
	float get_remap_output_min() const;

	// 重映射输出最大值
	void set_remap_output_max(float max_value);
	float get_remap_output_max() const;

	// 细胞

	// 细胞距离函数
	void set_cellular_distance_function(CellularDistanceFunction cdf);
	CellularDistanceFunction get_cellular_distance_function() const;

	// 细胞返回值类型
	void set_cellular_return_type(CellularReturnType rt);
	CellularReturnType get_cellular_return_type() const;

	// 细胞抖动
	void set_cellular_jitter(float jitter);
	float get_cellular_jitter() const;

	// 细胞索引 0
	void set_cellular_index0(int i);
	int get_cellular_index0() const;

	// 细胞索引 1
	void set_cellular_index1(int i);
	int get_cellular_index1() const;

	// 杂项

	// 编码的节点树数据
	void set_encoded_node_tree(String data);
	String get_encoded_node_tree() const;

	// 根据当前参数重建内部噪声生成器
	void update_generator();
	// 内部生成器是否构造成功
	bool is_valid() const;

	// 查询
	// TODO 双精度支持。FastNoise2 还没有，所以现在全部使用 `float`。

	// 采样单个坐标点的 2D 噪声
	float get_noise_2d_single(Vector2 pos) const;
	// 采样单个坐标点的 3D 噪声
	float get_noise_3d_single(Vector3 pos) const;

	// 逐个坐标批量采样 2D 噪声，结果写入 dst
	void get_noise_2d_series(Span<const float> src_x, Span<const float> src_y, Span<float> dst) const;
	// 逐个坐标批量采样 3D 噪声，结果写入 dst
	void get_noise_3d_series(
			Span<const float> src_x,
			Span<const float> src_y,
			Span<const float> src_z,
			Span<float> dst
	) const;

	// 采样二维矩形网格区域的噪声，结果写入 dst
	void get_noise_2d_grid(Vector2 origin, Vector2i size, Span<float> dst) const;
	// 采样三维立方网格区域的噪声，结果写入 dst
	void get_noise_3d_grid(Vector3 origin, Vector3i size, Span<float> dst) const;

	// 采样可平铺的 2D 网格噪声
	void get_noise_2d_grid_tileable(Vector2i size, Span<float> dst) const;

	// 生成噪声图像
	void generate_image(Ref<Image> image, bool tileable) const;

	// 获取输出的数值范围区间估计
	math::Interval get_estimated_output_range() const;

private:
	static void _bind_methods();

	int _seed = 1337;

	NoiseType _noise_type = TYPE_OPEN_SIMPLEX_2;
	String _last_set_encoded_node_tree;

	float _period = 64.f;

	FractalType _fractal_type = FRACTAL_NONE;
	int _fractal_octaves = 3;
	float _fractal_lacunarity = 2.f;
	float _fractal_gain = 0.5f;
	float _fractal_ping_pong_strength = 2.f;

	bool _terrace_enabled = false;
	float _terrace_multiplier = 1.0;
	float _terrace_smoothness = 0.0;

	CellularDistanceFunction _cellular_distance_function = CELLULAR_DISTANCE_EUCLIDEAN;
	CellularReturnType _cellular_return_type = CELLULAR_RETURN_INDEX_0;
	float _cellular_jitter = 1.0;
	uint8_t _cellular_index0 = 0;
	uint8_t _cellular_index1 = 1;

	bool _remap_enabled = false;
	float _remap_src_min = -1.0;
	float _remap_src_max = 1.0;
	float _remap_dst_min = -1.0;
	float _remap_dst_max = 1.0;

	FastNoise::SmartNode<> _generator;
};

} // namespace voxel

VARIANT_ENUM_CAST(voxel::FastNoise2::SIMDLevel);
VARIANT_ENUM_CAST(voxel::FastNoise2::NoiseType);
VARIANT_ENUM_CAST(voxel::FastNoise2::FractalType);
VARIANT_ENUM_CAST(voxel::FastNoise2::CellularDistanceFunction);
VARIANT_ENUM_CAST(voxel::FastNoise2::CellularReturnType);

#endif // VOXEL_FAST_NOISE_2_H