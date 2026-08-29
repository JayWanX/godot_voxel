// 真正最小化的 PCG32 代码 / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

#include "pcg.h"

uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    // 推进内部状态
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    // 计算输出函数（XSH RR），使用旧状态以最大化指令级并行度
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

// 源码来自 http://www.pcg-random.org/downloads/pcg-c-basic-0.9.zip
void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate, uint64_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

// 源码来自 https://github.com/imneme/pcg-c-basic/blob/master/pcg_basic.c
// pcg32_boundedrand_r(rng, bound):
//     生成一个均匀分布的数 r，满足 0 <= r < bound
uint32_t pcg32_boundedrand_r(pcg32_random_t *rng, uint32_t bound) {
	// 为避免偏差，我们需要让 RNG 的范围是 bound 的整数倍，
	// 做法是丢弃小于某个阈值的输出。
	// 计算阈值的朴素方案是
	//
	//     uint32_t threshold = 0x100000000ull % bound;
	//
	// 但 64 位除法/取模比 32 位的慢（尤其是在 32 位平台上）。
	// 本质上我们使用
	//
	//     uint32_t threshold = (0x100000000ull-bound) % bound;
	//
	// 因为这个版本会算出同样的模数，但左边的值小于 2^32。
	uint32_t threshold = -bound % bound;

	// 均匀性保证这个循环一定会终止。实际上它通常能快速终止；
	// 平均而言（假设所有 bound 等可能出现），82.25% 的情况下只需
	// 一次迭代。最坏情况下，如果传入的 bound 是 2^31 + 1
	// （即 2147483649），几乎会使 50% 的范围失效。实际上
	// bound 通常很小，只有极小一部分范围被丢弃。
	for (;;) {
		uint32_t r = pcg32_random_r(rng);
		if (r >= threshold)
			return r % bound;
	}
}
