#include "FastSIMD/FastSIMD.h"

#include <algorithm>
#include <cstdint>

#if FASTSIMD_x86

#ifdef __GNUG__
#include <x86intrin.h>
#else
#include <intrin.h>
#endif

#endif


#include "FastSIMD/SIMDTypeList.h"

static_assert(FastSIMD::SIMDTypeList::MinimumCompiled & FastSIMD::COMPILED_SIMD_LEVELS, "FASTSIMD_FALLBACK_SIMD_LEVEL is not a compiled SIMD level, check FastSIMD_Config.h");

#if FASTSIMD_x86
// 定义 cpuid 指令的接口。
// 输入：eax = functionnumber，ecx = 0
// output: eax = output[0], ebx = output[1], ecx = output[2], edx = output[3]
static void cpuid( int output[4], int functionnumber )
{
#if defined( __GNUC__ ) || defined( __clang__ )              // 使用内联汇编，Gnu/AT&T 语法

    int a, b, c, d;
    __asm("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(functionnumber), "c"(0) : );
    output[0] = a;
    output[1] = b;
    output[2] = c;
    output[3] = d;

#elif defined( _MSC_VER ) || defined ( __INTEL_COMPILER )     // Microsoft 或 Intel 编译器，已包含 intrin.h

    __cpuidex( output, functionnumber, 0 ); // CPUID 的内置函数

#else                                                      // 未知平台。尝试使用 masm/intel 语法的内联汇编

    __asm
    {
        mov eax, functionnumber
        xor ecx, ecx
        cpuid;
        mov esi, output
            mov[esi], eax
            mov[esi + 4], ebx
            mov[esi + 8], ecx
            mov[esi + 12], edx
    }

#endif
}

// 定义 xgetbv 指令的接口
static int64_t xgetbv( int ctr )
{
#if (defined( _MSC_FULL_VER ) && _MSC_FULL_VER >= 160040000) || (defined( __INTEL_COMPILER ) && __INTEL_COMPILER >= 1200) // 支持 _xgetbv 内置函数的 Microsoft 或 Intel 编译器

    return _xgetbv( ctr ); // XGETBV 的内置函数

#elif defined( __GNUC__ )                                    // 使用内联汇编，Gnu/AT&T 语法

    uint32_t a, d;
    __asm("xgetbv" : "=a"(a), "=d"(d) : "c"(ctr) : );
    return a | (uint64_t( d ) << 32);

#else  // #elif defined (_WIN32)                           // other compiler. try inline assembly with masm/intel/MS syntax

    uint32_t a, d;
    __asm {
        mov ecx, ctr
        _emit 0x0f
        _emit 0x01
        _emit 0xd0; // xgetbv
        mov a, eax
            mov d, edx
    }
    return a | (uint64_t( d ) << 32);

#endif
}
#endif

FASTSIMD_API FastSIMD::eLevel FastSIMD::CPUMaxSIMDLevel()
{
    static eLevel simdLevel = Level_Null;

    if ( simdLevel > Level_Null )
    {
        return simdLevel;
    }

#if FASTSIMD_x86
    int abcd[4] = { 0,0,0,0 }; // cpuid 结果

#if !FASTSIMD_64BIT
    simdLevel = Level_Scalar; // 默认值

    cpuid( abcd, 0 ); // 调用 cpuid 函数 0
    if ( abcd[0] == 0 )
        return simdLevel; // 不支持更多 cpuid 函数

    cpuid( abcd, 1 ); // 调用 cpuid 功能 1 以获取功能标志
    if ( (abcd[3] & (1 << 0)) == 0 )
        return simdLevel; // 不支持浮点
    if ( (abcd[3] & (1 << 23)) == 0 )
        return simdLevel; // no MMX
    if ( (abcd[3] & (1 << 15)) == 0 )
        return simdLevel; // 不支持条件移动
    if ( (abcd[3] & (1 << 24)) == 0 )
        return simdLevel; // no FXSAVE
    if ( (abcd[3] & (1 << 25)) == 0 )
        return simdLevel; // no SSE
    simdLevel = Level_SSE;
    // 1：支持 SSE

    if ( (abcd[3] & (1 << 26)) == 0 )
        return simdLevel; // no SSE2
#else
    cpuid( abcd, 1 ); // 调用 cpuid 功能 1 以获取功能标志
#endif

    simdLevel = Level_SSE2; // 64 位的默认值
    // 2：支持 SSE2

    if ( (abcd[2] & (1 << 0)) == 0 )
        return simdLevel; // no SSE3
    simdLevel = Level_SSE3;
    // 3：支持 SSE3

    if ( (abcd[2] & (1 << 9)) == 0 )
        return simdLevel; // no SSSE3
    simdLevel = Level_SSSE3;
    // 4：支持 SSSE3

    if ( (abcd[2] & (1 << 19)) == 0 )
        return simdLevel; // no SSE4.1
    simdLevel = Level_SSE41;
    // 5：支持 SSE4.1

    if ( (abcd[2] & (1 << 23)) == 0 )
        return simdLevel; // no POPCNT
    if ( (abcd[2] & (1 << 20)) == 0 )
        return simdLevel; // no SSE4.2
    simdLevel = Level_SSE42;
    // 6：支持 SSE4.2

    if ( (abcd[2] & (1 << 26)) == 0 )
        return simdLevel; // no XSAVE
    if ( (abcd[2] & (1 << 27)) == 0 )
        return simdLevel; // no OSXSAVE
    if ( (abcd[2] & (1 << 28)) == 0 )
        return simdLevel; // no AVX

    uint64_t osbv = xgetbv( 0 );
    if ( (osbv & 6) != 6 )
        return simdLevel; // 操作系统中未启用 AVX
    simdLevel = Level_AVX;
    // 7：支持 AVX

    cpuid( abcd, 7 ); // 调用 cpuid 叶子 7 以获取功能标志
    if ( (abcd[1] & (1 << 5)) == 0 )
        return simdLevel; // no AVX2
    simdLevel = Level_AVX2;
    // 8：支持 AVX2

    if( (osbv & (0xE0)) != 0xE0 )
        return simdLevel; // 操作系统中未启用 AVX512
    if ( (abcd[1] & (1 << 16)) == 0 )
        return simdLevel; // no AVX512
    cpuid( abcd, 0xD ); // 调用 cpuid leaf 0xD 获取功能标志
    if ( (abcd[0] & 0x60) != 0x60 )
        return simdLevel; // no AVX512
    // 9：支持 AVX512

    cpuid( abcd, 7 ); // 调用 cpuid 叶子 7 以获取功能标志
    if ( (abcd[1] & (1 << 31)) == 0 )
        return simdLevel; // no AVX512VL
    // 10：支持 AVX512VL

    if ( (abcd[1] & 0x40020000) != 0x40020000 )
        return simdLevel; // 不支持 AVX512BW、AVX512DQ
    simdLevel = Level_AVX512;
    // 11：支持 AVX512BW 和 AVX512DQ
#endif

#if FASTSIMD_ARM
    simdLevel = Level_NEON;
#endif

    return simdLevel;
}

template<typename CLASS_T, FastSIMD::eLevel SIMD_LEVEL>
CLASS_T* SIMDLevelSelector( FastSIMD::eLevel maxSIMDLevel, FastSIMD::MemoryAllocator allocator )
{
    if constexpr( ( CLASS_T::Supported_SIMD_Levels & SIMD_LEVEL ) != 0 )
    {
        CLASS_T* newClass = SIMDLevelSelector<CLASS_T, FastSIMD::SIMDTypeList::GetNextCompiledAfter<SIMD_LEVEL>>( maxSIMDLevel, allocator );

        if( !newClass && SIMD_LEVEL <= maxSIMDLevel )
        {
            return FastSIMD::ClassFactory<CLASS_T, SIMD_LEVEL>( allocator );
        }

        return newClass;
    }
    else
    {
        if constexpr( SIMD_LEVEL == FastSIMD::Level_Null )
        {
            return nullptr;
        }

        return SIMDLevelSelector<CLASS_T, FastSIMD::SIMDTypeList::GetNextCompiledAfter<SIMD_LEVEL>>( maxSIMDLevel, allocator );        
    }
}

template<typename CLASS_T>
CLASS_T* FastSIMD::New( eLevel maxSIMDLevel, FastSIMD::MemoryAllocator allocator )
{
    if( maxSIMDLevel == Level_Null )
    {
        maxSIMDLevel = CPUMaxSIMDLevel();
    }
    else
    {
        maxSIMDLevel = std::min( maxSIMDLevel, CPUMaxSIMDLevel() );        
    }

    static_assert(( CLASS_T::Supported_SIMD_Levels & FastSIMD::SIMDTypeList::MinimumCompiled ), "MinimumCompiled SIMD Level must be supported by this class" );
    return SIMDLevelSelector<CLASS_T, SIMDTypeList::MinimumCompiled>( maxSIMDLevel, allocator );
}

#define FASTSIMD_BUILD_CLASS( CLASS ) \
template FASTSIMD_API CLASS* FastSIMD::New( FastSIMD::eLevel, FastSIMD::MemoryAllocator );

#define FASTSIMD_INCLUDE_HEADER_ONLY
#include "FastSIMD_BuildList.inl"
