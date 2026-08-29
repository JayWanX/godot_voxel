# VoxelNode

继承自：[Node3D](https://docs.godotengine.org/en/stable/classes/class_node3d.html)

派生：[VoxelLodTerrain](VoxelLodTerrain.md), [VoxelTerrain](VoxelTerrain.md)

体素体积的基类。

## 属性：


类型                                                                                                                                                 | 名称                                           | 默认值 
-------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------- | ----
[ShadowCastingSetting](https://docs.godotengine.org/en/stable/classes/class_geometryinstance3d.html#enum-geometryinstance3d-shadowcastingsetting)  | [cast_shadow](#i_cast_shadow)                | 1   
[VoxelFormat](VoxelFormat.md)                                                                                                                      | [format](#i_format)                          |     
[VoxelGenerator](VoxelGenerator.md)                                                                                                                | [generator](#i_generator)                    |     
[GIMode](https://docs.godotengine.org/en/stable/classes/class_geometryinstance3d.html#enum-geometryinstance3d-gimode)                              | [gi_mode](#i_gi_mode)                        | 0   
[VoxelMesher](VoxelMesher.md)                                                                                                                      | [mesher](#i_mesher)                          |     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                                                                               | [render_layers_mask](#i_render_layers_mask)  | 1   
[VoxelStream](VoxelStream.md)                                                                                                                      | [stream](#i_stream)                          |     
<p></p>

## 方法：


返回值                                                                         | 函数签名                                                                                                     
--------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------
[Node3D](https://docs.godotengine.org/en/stable/classes/class_node3d.html)  | [convert_to_nodes](#i_convert_to_nodes) ( [NodeConversionFlags](VoxelNode.md#enumerations) flags ) const 
[VoxelTool](VoxelTool.md)                                                   | [get_voxel_tool](#i_get_voxel_tool) ( )                                                                  
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **NodeConversionFlags**：

- <span id="i_NODE_CONVERSION_INCLUDE_INSTANCER"></span>**NODE_CONVERSION_INCLUDE_INSTANCER** = **1** --- 如果地形具有 [VoxelInstancer](VoxelInstancer.md)，其生成的实例将作为每个区块的 [MultiMeshInstance](https://docs.godotengine.org/en/stable/classes/class_multimeshinstance.html) 包含在结果中。
- <span id="i_NODE_CONVERSION_INCLUDE_INVISIBLE_BLOCKS"></span>**NODE_CONVERSION_INCLUDE_INVISIBLE_BLOCKS** = **2** --- 某些区块会被地形系统在内部隐藏，尤其是作为 LOD 系统的一部分。如果提供此标志，这些区块将包含在输出中。
- <span id="i_NODE_CONVERSION_INCLUDE_MATERIAL_OVERRIDES"></span>**NODE_CONVERSION_INCLUDE_MATERIAL_OVERRIDES** = **4** --- 指定地形上存在的材质覆盖应应用于结果中的区块。


## 属性描述

### [ShadowCastingSetting](https://docs.godotengine.org/en/stable/classes/class_geometryinstance3d.html#enum-geometryinstance3d-shadowcastingsetting)<span id="i_cast_shadow"></span> **cast_shadow** = 1

设置地形网格将使用的阴影投射模式。

### [VoxelFormat](VoxelFormat.md)<span id="i_format"></span> **format**

覆盖体素的默认格式。

警告：更改此项将重新加载地形。如果它连接了使用不同格式保存数据的数据流，可能会加载不正确。

### [VoxelGenerator](VoxelGenerator.md)<span id="i_generator"></span> **generator**

当数据流中不存在体素数据块时，用于加载体素数据块的程序化生成器。

### [GIMode](https://docs.godotengine.org/en/stable/classes/class_geometryinstance3d.html#enum-geometryinstance3d-gimode)<span id="i_gi_mode"></span> **gi_mode** = 0

设置地形网格将使用的全局光照模式。

### [VoxelMesher](VoxelMesher.md)<span id="i_mesher"></span> **mesher**

定义体素如何被转换为可见网格。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_render_layers_mask"></span> **render_layers_mask** = 1

设置地形网格将使用的渲染层。

### [VoxelStream](VoxelStream.md)<span id="i_stream"></span> **stream**

持久体素数据的主要来源。如果保持未赋值，整个体积将使用生成器。

## 方法描述

### [Node3D](https://docs.godotengine.org/en/stable/classes/class_node3d.html)<span id="i_convert_to_nodes"></span> **convert_to_nodes**( [NodeConversionFlags](VoxelNode.md#enumerations) flags ) 

生成一棵表示调用时地形状态的节点树，仅使用 Godot 原生节点。

此方法可作为将地形快照生成为原生场景的一种方式，该场景可在未安装插件的情况下用标准 Godot 版本打开。它也可用于调试，或作为绕开某些期望网格实例节点（如某些 GI 烘焙选项、导航生成或 GLTF 导出）的 Godot 特性的手段。

返回的根节点不会添加到场景树中。如何处理它由你决定。如果你希望它出现在场景中，应将其添加为现有节点的子节点。否则，别忘了释放它。

输出结构：地形的每个区块都会创建自己的 [MeshInstance3D](https://docs.godotengine.org/en/stable/classes/class_meshinstance3d.html)，并附带唯一的 [ArrayMesh](https://docs.godotengine.org/en/stable/classes/class_arraymesh.html) 资源。网格和/或网格实例也可能附有材质。根据地形的配置方式，这些材质可以是每个区块唯一的。如果地形具有 LOD，区块将存储在相应的 LOD 节点下。如果地形具有实例化器，可包含多网格区块。有关选项，请参见 [NodeConversionFlags](VoxelNode.md#enumerations)。

资源：除非另有指定，否则树中节点附带的资源将直接引用地形系统实际使用的资源。注意，根据配置的不同，当地形运行下一个处理周期时，这些资源有可能被更改（例如，带每区块参数的材质会被复用，因此当区块被卸载时，节点中引用的材质可能会随着新区块重新使用它们而发生剧烈变化）。

性能：这可能是一项昂贵的操作，可能会拖慢游戏。

打包为场景文件：可以通过设置 [Node.owner](https://docs.godotengine.org/en/stable/classes/class_node.html#class-node-property-owner)（参见 Godot 文档）将输出保存为 [PackedScene](https://docs.godotengine.org/en/stable/classes/class_packedscene.html)。它包含大量唯一网格，因此保存的场景可能非常庞大。如果这是个问题，建议保存为二进制 `.scn`。如果地形使用带细节渲染的 [ShaderMaterial](https://docs.godotengine.org/en/stable/classes/class_shadermaterial.html)，区块还将拥有唯一纹理，进一步增大体积。

GLTF：如果地形使用 [ShaderMaterial](https://docs.godotengine.org/en/stable/classes/class_shadermaterial.html) 且你想将地形转换为 [GLTFDocument](https://docs.godotengine.org/en/stable/classes/class_gltfdocument.html)，这些材质不会随之迁移，因为 GLTF 不支持它们。这也意味着，如果你将 [VoxelMesherTransvoxel](VoxelMesherTransvoxel.md) 与 LOD 一起使用，GLTF 文档中的网格将产生多余的几何体以及接缝处的裂缝（这些通常由着色器处理）。

LOD：如果地形具有 LOD，你只会获得当前活动 LOD 的快照。精细网格将围绕观察者居中，并随着距离越远细节越少。

### [VoxelTool](VoxelTool.md)<span id="i_get_voxel_tool"></span> **get_voxel_tool**( ) 

创建一个绑定到此节点的 [VoxelTool](VoxelTool.md) 实例，以访问体素和编辑方法。

只要节点仍然存在，你可以将它保存在成员变量中，以避免重复创建。

_生成于 2026-08-28_
