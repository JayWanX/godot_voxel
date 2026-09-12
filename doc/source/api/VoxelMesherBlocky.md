# VoxelMesherBlocky

继承自：[VoxelMesher](VoxelMesher.md)

通过批量合并与每个体素值对应的模型来生成网格，类似于 Minecraft 或 StarMade 等游戏。

## 描述：

被遮挡的面会从结果中移除，并且可以在边缘烘焙一定程度的环境光遮蔽。体素值应存储在 [VoxelBuffer.CHANNEL_TYPE](VoxelBuffer.md#i_CHANNEL_TYPE) 通道中。模型通过 [VoxelBlockyLibrary](VoxelBlockyLibrary.md) 定义，其中模型索引与体素值对应。模型不一定是立方体。

## 属性：


类型                                                                        | 名称                                                           | 默认值           
------------------------------------------------------------------------- | ------------------------------------------------------------ | --------------
[VoxelBlockyLibraryBase](VoxelBlockyLibraryBase.md)                       | [library](#i_library)                                        |               
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [occlusion_darkness](#i_occlusion_darkness)                  | 0.8           
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [occlusion_enabled](#i_occlusion_enabled)                    | true          
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [shadow_occluder_negative_x](#i_shadow_occluder_negative_x)  | false         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [shadow_occluder_negative_y](#i_shadow_occluder_negative_y)  | false         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [shadow_occluder_negative_z](#i_shadow_occluder_negative_z)  | false         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [shadow_occluder_positive_x](#i_shadow_occluder_positive_x)  | false         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [shadow_occluder_positive_y](#i_shadow_occluder_positive_y)  | false         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [shadow_occluder_positive_z](#i_shadow_occluder_positive_z)  | false         
[TintMode](VoxelMesherBlocky.md#enumerations)                             | [tint_mode](#i_tint_mode)                                    | TINT_NONE (0) 
<p></p>

## 方法：


返回值                                                                     | 函数签名                                                                                                                                                                                        
----------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)  | [get_shadow_occluder_side](#i_get_shadow_occluder_side) ( [Side](VoxelMesherBlocky.md#enumerations) side ) const                                                                            
[void](#)                                                               | [set_shadow_occluder_side](#i_set_shadow_occluder_side) ( [Side](VoxelMesherBlocky.md#enumerations) side, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled )  
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **Side**：

- <span id="i_SIDE_NEGATIVE_X"></span>**SIDE_NEGATIVE_X** = **0**
- <span id="i_SIDE_POSITIVE_X"></span>**SIDE_POSITIVE_X** = **1**
- <span id="i_SIDE_NEGATIVE_Y"></span>**SIDE_NEGATIVE_Y** = **2**
- <span id="i_SIDE_POSITIVE_Y"></span>**SIDE_POSITIVE_Y** = **3**
- <span id="i_SIDE_NEGATIVE_Z"></span>**SIDE_NEGATIVE_Z** = **4**
- <span id="i_SIDE_POSITIVE_Z"></span>**SIDE_POSITIVE_Z** = **5**

枚举 **TintMode**：

- <span id="i_TINT_NONE"></span>**TINT_NONE** = **0** --- 仅使用库模型中的颜色。
- <span id="i_TINT_RAW_COLOR"></span>**TINT_RAW_COLOR** = **1** --- 根据 [VoxelBuffer.CHANNEL_COLOR](VoxelBuffer.md#i_CHANNEL_COLOR) 通道调整体素颜色。值被解释为原始 RGBA 颜色。如果通道是 16 位，则颜色按每个分量 4 位打包。如果是 32 位，则颜色按每个分量 8 位打包。不支持其他位深。编码请参见 [VoxelTool.color_to_u32](VoxelTool.md#i_color_to_u32)。你也可以更改颜色通道的格式来使用此模式，参见 [VoxelNode.format](VoxelNode.md#i_format)。Alpha 仅在材质支持透明度时才会生效，但不会影响网格生成器对面进行剔除的方式。


## 属性描述

### [VoxelBlockyLibraryBase](VoxelBlockyLibraryBase.md)<span id="i_library"></span> **library**

此网格生成器将使用的模型库。如果你在没有地形的情况下使用网格生成器，请确保在构建网格之前调用 [VoxelBlockyLibraryBase.bake](VoxelBlockyLibraryBase.md#i_bake)，否则结果将为空或已过时。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_occlusion_darkness"></span> **occlusion_darkness** = 0.8

相邻体素遮挡产生的阴影暗度。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_occlusion_enabled"></span> **occlusion_enabled** = true

启用烘焙的环境光遮蔽。要渲染它，你需要一个应用顶点颜色的材质。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_shadow_occluder_negative_x"></span> **shadow_occluder_negative_x** = false

启用后，如果数据块完全被不透明体素覆盖，则生成一个覆盖数据块负 X 侧的四边形，以强制方向光投射阴影。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_shadow_occluder_negative_y"></span> **shadow_occluder_negative_y** = false

启用后，如果数据块完全被不透明体素覆盖，则生成一个覆盖数据块负 Y 侧的四边形，以强制方向光投射阴影。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_shadow_occluder_negative_z"></span> **shadow_occluder_negative_z** = false

启用后，如果数据块完全被不透明体素覆盖，则生成一个覆盖数据块负 Z 侧的四边形，以强制方向光投射阴影。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_shadow_occluder_positive_x"></span> **shadow_occluder_positive_x** = false

启用后，如果数据块完全被不透明体素覆盖，则生成一个覆盖数据块正 X 侧的四边形，以强制方向光投射阴影。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_shadow_occluder_positive_y"></span> **shadow_occluder_positive_y** = false

启用后，如果数据块完全被不透明体素覆盖，则生成一个覆盖数据块正 Y 侧的四边形，以强制方向光投射阴影。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_shadow_occluder_positive_z"></span> **shadow_occluder_positive_z** = false

启用后，如果数据块完全被不透明体素覆盖，则生成一个覆盖数据块正 Z 侧的四边形，以强制方向光投射阴影。

### [TintMode](VoxelMesherBlocky.md#enumerations)<span id="i_tint_mode"></span> **tint_mode** = TINT_NONE (0)

配置从体素数据应用颜色的方式。

## 方法描述

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_get_shadow_occluder_side"></span> **get_shadow_occluder_side**( [Side](VoxelMesherBlocky.md#enumerations) side ) 

获取指定侧面是否作为阴影遮挡体。

### [void](#)<span id="i_set_shadow_occluder_side"></span> **set_shadow_occluder_side**( [Side](VoxelMesherBlocky.md#enumerations) side, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled ) 

设置指定侧面是否作为阴影遮挡体。

_生成于 2026-09-12_
