# VoxelMesherTransvoxel

继承自：[VoxelMesher](VoxelMesher.md)

使用 [Transvoxel](https://transvoxel.org/) 算法实现等值面生成（平滑体素）。

## 描述：

使用 [Transvoxel](https://transvoxel.org/) 算法将平滑体素多边形化。注意：如果你想使用 LOD，你需要特殊的顶点着色器代码来正确渲染接缝（否则它们会在数据块之间渲染成薄片）。更多信息请参见 [https://voxel-tools.readthedocs.io/en/latest/smooth_terrain/#transvoxel](https://voxel-tools.readthedocs.io/en/latest/smooth_terrain/#transvoxel) 或包含平滑地形的示例。

## 属性：


类型                                                                        | 名称                                                                         | 默认值               
------------------------------------------------------------------------- | -------------------------------------------------------------------------- | ------------------
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [edge_clamp_margin](#i_edge_clamp_margin)                                  | 0.02              
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [mesh_optimization_enabled](#i_mesh_optimization_enabled)                  | false             
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [mesh_optimization_error_threshold](#i_mesh_optimization_error_threshold)  | 0.005             
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [mesh_optimization_target_ratio](#i_mesh_optimization_target_ratio)        | 0.0               
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [textures_ignore_air_voxels](#i_textures_ignore_air_voxels)                | false             
[TexturingMode](VoxelMesherTransvoxel.md#enumerations)                    | [texturing_mode](#i_texturing_mode)                                        | TEXTURES_NONE (0) 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [transitions_enabled](#i_transitions_enabled)                              | true              
<p></p>

## 方法：


返回值                                                                               | 函数签名                                                                                                                                                                              
--------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[ArrayMesh](https://docs.godotengine.org/en/stable/classes/class_arraymesh.html)  | [build_transition_mesh](#i_build_transition_mesh) ( [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) direction )  
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **TexturingMode**：

- <span id="i_TEXTURES_NONE"></span>**TEXTURES_NONE** = **0** --- 禁用纹理信息。如果你可以使用着色器以程序化方式应用纹理，此模式最快。
- <span id="i_TEXTURES_MIXEL4_S4"></span>**TEXTURES_MIXEL4_S4** = **1** --- 要求体素在 [VoxelBuffer.CHANNEL_INDICES](VoxelBuffer.md#i_CHANNEL_INDICES) 中以 16 位值打包 4 个 4 位索引，并在 [VoxelBuffer.CHANNEL_WEIGHTS](VoxelBuffer.md#i_CHANNEL_WEIGHTS) 中包含 4 个 4 位权重。 以 4 个纹理索引和 4 个权重的方式添加纹理信息，在 Godot 片元着色器中编码为 `CUSTOM1.xy`，其中 x 和 y 包含 4 个打包的 8 位值。 在 2x2x2 体素区域内超过 4 个纹理相互交叉的情况下，该区域的三角形将只使用权重最高的 4 个索引。 渲染此模式需要自定义着色器，通常使用纹理数组以便轻松索引纹理。
- <span id="i_TEXTURES_SINGLE_S4"></span>**TEXTURES_SINGLE_S4** = **2** --- 要求体素在 [VoxelBuffer.CHANNEL_INDICES](VoxelBuffer.md#i_CHANNEL_INDICES) 通道中具有 8 位纹理索引。 以 4 个纹理索引和 4 个权重的方式添加纹理信息，在 Godot 片元着色器中编码为 `CUSTOM1.xy`，其中 x 和 y 包含 4 个打包的 8 位值。 在 2x2x2 体素区域内超过 4 个纹理相互交叉的情况下，该区域的三角形将只使用权重最高的 4 个索引。 渲染此模式需要自定义着色器，通常使用纹理数组以便轻松索引纹理。


## 常量：

- <span id="i_TEXTURES_BLEND_4_OVER_16"></span>**TEXTURES_BLEND_4_OVER_16** = **1** --- *此常量已弃用。 Use TEXTURES_MIXEL4_S4* [TEXTURES_MIXEL4_S4](VoxelMesherTransvoxel.md#i_TEXTURES_MIXEL4_S4) 的旧版别名。

## 属性描述

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_edge_clamp_margin"></span> **edge_clamp_margin** = 0.02

计算行进立方体单元时，顶点可能被放置在单元边的任意位置，包括非常靠近角点的位置。这可能导致非常细或非常小的三角形，对某些物理引擎来说尤其是个问题。此边距是与角点的最小距离，低于该距离的顶点将被钳制到该距离。增大此值可能会降低网格质量并引入细小的脊线。此属性不能低于 0（此时不发生钳制），也不能高于 0.5（此时不发生插值，因为顶点总是被放置在边的中间）。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_mesh_optimization_enabled"></span> **mesh_optimization_enabled** = false

*(此属性暂无文档)*

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_mesh_optimization_error_threshold"></span> **mesh_optimization_error_threshold** = 0.005

*(此属性暂无文档)*

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_mesh_optimization_target_ratio"></span> **mesh_optimization_target_ratio** = 0.0

*(此属性暂无文档)*

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_textures_ignore_air_voxels"></span> **textures_ignore_air_voxels** = false

*(此属性暂无文档)*

### [TexturingMode](VoxelMesherTransvoxel.md#enumerations)<span id="i_texturing_mode"></span> **texturing_mode** = TEXTURES_NONE (0)

*(此属性暂无文档)*

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_transitions_enabled"></span> **transitions_enabled** = true

*(此属性暂无文档)*

## 方法描述

### [ArrayMesh](https://docs.godotengine.org/en/stable/classes/class_arraymesh.html)<span id="i_build_transition_mesh"></span> **build_transition_mesh**( [VoxelBuffer](VoxelBuffer.md) voxel_buffer, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) direction ) 

仅生成 Transvoxel 用于连接不同细节层级表面的网格部分。此方法主要用于测试目的。

_生成于 2026-08-28_
