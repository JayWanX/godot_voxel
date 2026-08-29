#pragma once
#include <functional>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

#include "FastNoise_Config.h"

#pragma warning( push )
#pragma warning( disable : 4251 )

namespace FastNoise
{
    class Generator;
    template<typename T>
    struct PerDimensionVariable;
    struct Metadata;
    struct NodeData;

    namespace Impl
    {
        template<typename T>
        FASTNOISE_API const Metadata& GetMetadata();
    }

    // 存储 FastNoise 节点类的定义
    // 节点名、成员名+类型、设置成员的函数
    struct FASTNOISE_API Metadata
    {
        virtual ~Metadata() = default;

        /// <returns>包含每种 FastNoise 节点类型元数据的数组</returns>
        static const std::vector<const Metadata*>& GetAll()
        {
            return sAllMetadata;
        }

        /// <returns>给定 Metadata::id 的元数据</returns>
        static const Metadata* GetFromId( uint16_t nodeId )
        {
            // 元数据尚未加载
            // 不要在静态初始化期间从元数据创建节点
            // 元数据通过静态变量加载，而静态变量的初始化顺序是随机的
            assert( sAllMetadata.size() );

            if( nodeId < sAllMetadata.size() )
            {
                return sAllMetadata[nodeId];
            }

            return nullptr;
        }

        /// <returns>给定节点类的元数据</returns>
        template<typename T>
        static const Metadata& Get()
        {
            static_assert( std::is_base_of<Generator, T>::value, "This function should only be used for FastNoise node classes, for example FastNoise::Simplex" );
            static_assert( std::is_member_function_pointer<decltype(&T::GetMetadata)>::value, "Cannot get Metadata for abstract node class, use a derived class, for example: Fractal -> FractalFBm" );

            return Impl::GetMetadata<T>();
        }

        /// <summary>
        /// Serialise node data and any source node datas (recursive)
        /// </summary>
        /// <param name="nodeData">根节点数据</param>
        /// <param name="fixUp">移除依赖环和无效节点类型</param>
        /// <returns>出错时返回空字符串</returns>
        static std::string SerialiseNodeData( NodeData* nodeData, bool fixUp = false );

        /// <summary>
        /// 将从 SerialiseNodeData 创建的字符串反序列化为节点数据树
        /// </summary>
        /// <param name="serialisedBase64NodeData">要反序列化的已编码字符串</param>
        /// <param name="nodeDataOut">新节点数据的存储</param>
        /// <returns>根节点</returns>
        static NodeData* DeserialiseNodeData( const char* serialisedBase64NodeData, std::vector<std::unique_ptr<NodeData>>& nodeDataOut );

        struct NameDesc
        {
            const char* name;
            const char* desc;
            
            NameDesc( const char* name, const char* desc = "" ) : name( name ), desc( desc ) {}
        };

        // 基础成员结构体
        struct Member
        {
            const char* name = "";
            const char* description = "";
            int dimensionIdx = -1;            
        };

        /// <summary>
        /// 为节点名添加空格：DomainScale -> Domain Scale
        /// </summary>
        /// <param name="metadata">FastNoise 节点元数据</param>
        /// <param name="removeGroups">从名称中移除元数据分组：FractalFBm -> FBm</param>
        /// <returns>带有格式化名称的 string</returns>
        static std::string FormatMetadataNodeName( const Metadata* metadata, bool removeGroups = false );

        /// <summary>
        /// 为按维度区分的成员变量添加维度前缀：
        /// DomainAxisScale::Scale -> X Scale
        /// </summary>
        /// <param name="member">FastNoise 节点元数据成员</param>
        /// <returns>带有格式化名称的 string</returns>
        static std::string FormatMetadataMemberName( const Member& member );

        // float、int 或 enum 值
        struct MemberVariable : Member
        {
            enum eType
            {
                EFloat,
                EInt,
                EEnum
            };

            union ValueUnion
            {
                float f;
                int i;

                ValueUnion( float v = 0 )
                {
                    f = v;
                }

                ValueUnion( int v )
                {
                    i = v;
                }

                operator float()
                {
                    return f;
                }

                operator int()
                {
                    return i;
                }

                bool operator ==( const ValueUnion& rhs ) const
                {
                    return i == rhs.i;
                }
            };

            eType type;
            ValueUnion valueDefault, valueMin, valueMax;
            std::vector<const char*> enumNames;

            // 为给定 generator 设置值的函数
            // 若 Generator 是正确的节点类则返回 true
            std::function<bool( Generator*, ValueUnion )> setFunc;
        };

        // Node lookup (must be valid for node to function)
        struct MemberNodeLookup : Member
        {
            // 为给定 generator 设置 source 的函数
            // 若 Generator* 是正确的节点类且 SmartNodeArg<> 是正确的节点类则返回 true
            std::function<bool( Generator*, SmartNodeArg<> )> setFunc;
        };

        // 常量浮点数或节点查找
        struct MemberHybrid : Member
        {
            float valueDefault = 0.0f;

            // 为给定 generator 设置值的函数
            // 若 Generator 是正确的节点类则返回 true
            std::function<bool( Generator*, float )> setValueFunc;

            // 为给定 generator 设置 source 的函数
            // 若同时也设置了值，则 Source 优先
            // 若 Generator 是正确的节点类且 SmartNodeArg<> 是正确的节点类则返回 true
            std::function<bool( Generator*, SmartNodeArg<> )> setNodeFunc;
        };

        uint16_t id;
        const char* name = "";
        const char* description = "";
        std::vector<const char*> groups;

        std::vector<MemberVariable>   memberVariables;
        std::vector<MemberNodeLookup> memberNodeLookups;
        std::vector<MemberHybrid>     memberHybrids;

        /// <summary>
        /// 从元数据创建 FastNoise 节点的新实例
        /// </summary>
        /// <example>
        /// auto node = metadata->CreateNode();
        /// metadata->memberVariables[0].setFunc( node.get(), 1.5f );
        /// </example>
        /// <param name="maxSimdLevel">最大 SIMD 级别，Null = Auto</param>
        /// <returns>保证 SmartNode<T> 不为 nullptr</returns>
        virtual SmartNode<> CreateNode( FastSIMD::eLevel maxSimdLevel = FastSIMD::Level_Null ) const = 0;

    protected:
        Metadata()
        {
            id = AddMetadata( this );
        }

    private:
        static uint16_t AddMetadata( const Metadata* newMetadata )
        {
            sAllMetadata.emplace_back( newMetadata );

            return (uint16_t)sAllMetadata.size() - 1;
        }

        static std::vector<const Metadata*> sAllMetadata;
    };

    // 存储创建 FastNoise 节点实例的数据
    // 节点类型、成员值
    struct FASTNOISE_API NodeData
    {
        NodeData( const Metadata* metadata );

        const Metadata* metadata;
        std::vector<Metadata::MemberVariable::ValueUnion> variables;
        std::vector<NodeData*> nodeLookups;
        std::vector<std::pair<NodeData*, float>> hybrids;

        bool operator ==( const NodeData& rhs ) const
        {
            return metadata == rhs.metadata &&
                variables == rhs.variables &&
                nodeLookups == rhs.nodeLookups &&
                hybrids == rhs.hybrids;
        }
    };
}

#pragma warning( pop )
