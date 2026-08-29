/*************************************************************************/
/*  random_pcg.h                                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
// 添加 godot 命名空间封装，以适配模块构建

#ifndef VOXEL_RANDOM_PCG_H
#define VOXEL_RANDOM_PCG_H

#ifdef VOXEL_GODOT

// 使用内置版本
#include <core/math/random_pcg.h>

#else

#include "../../macros.h"
#include "../../math/constants.h"
#include "pcg.h"
#include <math.h>

#if defined(__GNUC__)
#define CLZ32(x) __builtin_clz(x)
#elif defined(_MSC_VER)
#include <intrin.h>
static int __bsr_clz32(uint32_t x) {
	unsigned long index;
	_BitScanReverse(&index, x);
	return 31 - index;
}
#define CLZ32(x) __bsr_clz32(x)
#else
#endif

#if defined(__GNUC__)
#define LDEXP(s, e) __builtin_ldexp(s, e)
#define LDEXPF(s, e) __builtin_ldexpf(s, e)
#else
#include <math.h>
#define LDEXP(s, e) ldexp(s, e)
#define LDEXPF(s, e) ldexp(s, e)
#endif

namespace godot {

class RandomPCG {
	pcg32_random_t pcg;
	uint64_t current_seed = 0; // 当前生成器状态所基于的种子。
	uint64_t current_inc = 0;

public:
	static const uint64_t DEFAULT_SEED = 12047754176567800795U;
	static const uint64_t DEFAULT_INC = PCG_DEFAULT_INC_64;

	RandomPCG(uint64_t p_seed = DEFAULT_SEED, uint64_t p_inc = DEFAULT_INC);

	inline void seed(uint64_t p_seed) {
		current_seed = p_seed;
		pcg32_srandom_r(&pcg, current_seed, current_inc);
	}
	inline uint64_t get_seed() {
		return current_seed;
	}

	inline void set_state(uint64_t p_state) {
		pcg.state = p_state;
	}
	inline uint64_t get_state() const {
		return pcg.state;
	}

	void randomize();
	inline uint32_t rand() {
		return pcg32_random_r(&pcg);
	}
	inline uint32_t rand(uint32_t bounds) {
		return pcg32_boundedrand_r(&pcg, bounds);
	}

	// 以"足够好"的均匀性获取 [0, 1] 范围内的浮点数。
	// 这些函数把 rand() 的输出当作无限二进制数的小数部分来采样，
	// 并应用了一些技巧来减少运算和分支：
	// 1. 我们不移动到第一个 1 再拼接随机位，而是直接把最高位和最低位设为 1。
	//    只要 RNG 确实逐位均匀，这应该有完全相同的效果。
	// 2. 为了补偿指数信息的丢失，我们从另一个随机数中统计前导零的个数，
	//    然后把它加到初始偏移上。
	//    这与对真实比特流进行计数和移位具有相同的概率：n 个零的概率为 2^-n。
	// 对于所有高于 2^-96（float 为 2^-64）的数，这些函数都应是均匀的。
	// 不过，低于该阈值的所有数都会被下取整为 0。
	// 阈值的选择是为了尽量减少 rand() 调用次数，同时把数值保持在某种主观的质量
	// 标准内。如果 clz 或 ldexp 不可用，则回退到按位截断以换取性能，牺牲均匀性。
	inline double randd() {
#if defined(CLZ32)
		uint32_t proto_exp_offset = rand();
		if (VOXEL_UNLIKELY(proto_exp_offset == 0)) {
			return 0;
		}
		uint64_t significand = (((uint64_t)rand()) << 32) | rand() | 0x8000000000000001U;
		return LDEXP((double)significand, -64 - CLZ32(proto_exp_offset));
#else
#pragma message("RandomPCG::randd - intrinsic clz is not available, falling back to bit truncation")
		return (double)(((((uint64_t)rand()) << 32) | rand()) & 0x1FFFFFFFFFFFFFU) / (double)0x1FFFFFFFFFFFFFU;
#endif
	}
	inline float randf() {
#if defined(CLZ32)
		uint32_t proto_exp_offset = rand();
		if (VOXEL_UNLIKELY(proto_exp_offset == 0)) {
			return 0;
		}
		return LDEXPF((float)(rand() | 0x80000001), -32 - CLZ32(proto_exp_offset));
#else
#pragma message("RandomPCG::randf - intrinsic clz is not available, falling back to bit truncation")
		return (float)(rand() & 0xFFFFFF) / (float)0xFFFFFF;
#endif
	}

	inline double randfn(double p_mean, double p_deviation) {
		return p_mean +
				p_deviation *
				(cos(voxel::math::TAU<double> * randd()) * sqrt(-2.0 * log(randd()))); // Box-Muller 变换
	}
	inline float randfn(float p_mean, float p_deviation) {
		return p_mean +
				p_deviation *
				(cos(voxel::math::TAU<float> * randf()) * sqrt(-2.0 * log(randf()))); // Box-Muller 变换
	}

	double random(double p_from, double p_to);
	float random(float p_from, float p_to);
	int random(int p_from, int p_to);
};

} // namespace godot

using namespace godot;

#endif // VOXEL_GODOT
#endif // RANDOM_PCG_H
