#pragma once
#include "FastNoise_Config.h"

// 节点类定义
#include "Generators/BasicGenerators.h"
#include "Generators/Value.h"
#include "Generators/Perlin.h"
#include "Generators/Simplex.h"
#include "Generators/Cellular.h"
#include "Generators/Fractal.h"
#include "Generators/DomainWarp.h"
#include "Generators/DomainWarpFractal.h"
#include "Generators/Modifiers.h"
#include "Generators/Blends.h"

namespace FastNoise
{
    /// <summary>
    /// 创建 FastNoise 节点的新实例
    /// </summary>
    /// <example>
    /// auto node = FastNoise::New<FastNoise::Simplex>();
    /// </example>
    /// <typeparam name="T">要创建的节点类</typeparam>
    /// <param name="maxSimdLevel">最大 SIMD 级别，Null = Auto</param>
    /// <returns>保证 SmartNode<T> 不为 nullptr</returns>
    template<typename T>
    SmartNode<T> New( FastSIMD::eLevel maxSimdLevel /*= FastSIMD::Level_Null*/ )
    {
        static_assert( std::is_base_of<Generator, T>::value, "This function should only be used for FastNoise node classes, for example FastNoise::Simplex" );
        static_assert( std::is_member_function_pointer<decltype(&T::GetMetadata)>::value, "Cannot create abstract node class, use a derived class, for example: Fractal -> FractalFBm" );

#if FASTNOISE_USE_SHARED_PTR
        return SmartNode<T>( FastSIMD::New<T>( maxSimdLevel ) );
#else
        return SmartNode<T>( FastSIMD::New<T>( maxSimdLevel, &SmartNodeManager::Allocate ) );
#endif
    }

    /// <summary>
    /// 从编码字符串创建 FastNoise 节点树
    /// </summary>
    /// <example>
    /// FastNoise::SmartNode<> rootNode = FastNoise::NewFromEncodedNodeTree( "DQAFAAAAAAAAQAgAAAAAAD8AAAAAAA==" );
    /// </example>
    /// <param name="encodedNodeTreeString">可使用 NoiseTool 生成</param>
    /// <param name="maxSimdLevel">最大 SIMD 级别，Null = Auto</param>
    /// <returns>树的根节点，无效字符串返回 nullptr</returns>
    FASTNOISE_API SmartNode<> NewFromEncodedNodeTree( const char* encodedNodeTreeString, FastSIMD::eLevel maxSimdLevel = FastSIMD::Level_Null );
}
