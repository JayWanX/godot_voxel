# VoxelFormat

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

指定体素的格式。

## 描述：

指定体素的格式。目前，它只存储每个通道在单个体素中占用的字节数。

体素有一个默认格式，对大多数使用场景来说通常已经足够，但有时有必要更改它。在这种情况下，你可以创建一个新的 [VoxelFormat](VoxelFormat.md) 资源，进行更改，并将其分配给 [VoxelNode](VoxelNode.md)。

警告：建议在开发早期就确定格式（无论是默认格式还是自定义格式）。如果你想在很晚之后更改，并且已有存档，那么你必须想办法转换它们，否则加载时会出问题。

## 属性：


类型                                                                        | 名称                                 | 默认值                         
------------------------------------------------------------------------- | ---------------------------------- | ----------------------------
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)  | [_data](#i__data)                  | [0, 1, 1, 0, 1, 1, 0, 0, 0] 
[Depth](VoxelBuffer.md#enumerations)                                      | [color_depth](#i_color_depth)      | DEPTH_8_BIT (0)             
[Depth](VoxelBuffer.md#enumerations)                                      | [indices_depth](#i_indices_depth)  | DEPTH_16_BIT (1)            
[Depth](VoxelBuffer.md#enumerations)                                      | [sdf_depth](#i_sdf_depth)          | DEPTH_16_BIT (1)            
[Depth](VoxelBuffer.md#enumerations)                                      | [type_depth](#i_type_depth)        | DEPTH_16_BIT (1)            
<p></p>

## 方法：


返回值                                   | 函数签名                                                                                                                                              
------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                             | [configure_buffer](#i_configure_buffer) ( [VoxelBuffer](VoxelBuffer.md) buffer ) const                                                            
[VoxelBuffer](VoxelBuffer.md)         | [create_buffer](#i_create_buffer) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) size ) const                   
[Depth](VoxelBuffer.md#enumerations)  | [get_channel_depth](#i_get_channel_depth) ( [ChannelId](VoxelBuffer.md#enumerations) channel_index ) const                                        
[void](#)                             | [set_channel_depth](#i_set_channel_depth) ( [ChannelId](VoxelBuffer.md#enumerations) channel_index, [Depth](VoxelBuffer.md#enumerations) depth )  
<p></p>

## 属性描述

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i__data"></span> **_data** = [0, 1, 1, 0, 1, 1, 0, 0, 0]

*(此属性暂无文档)*

### [Depth](VoxelBuffer.md#enumerations)<span id="i_color_depth"></span> **color_depth** = DEPTH_8_BIT (0)

[VoxelBuffer.CHANNEL_COLOR](VoxelBuffer.md#i_CHANNEL_COLOR) 的位深。

### [Depth](VoxelBuffer.md#enumerations)<span id="i_indices_depth"></span> **indices_depth** = DEPTH_16_BIT (1)

[VoxelBuffer.CHANNEL_INDICES](VoxelBuffer.md#i_CHANNEL_INDICES) 的位深。仅支持 8 位和 16 位位深。

### [Depth](VoxelBuffer.md#enumerations)<span id="i_sdf_depth"></span> **sdf_depth** = DEPTH_16_BIT (1)

[VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 的位深。

### [Depth](VoxelBuffer.md#enumerations)<span id="i_type_depth"></span> **type_depth** = DEPTH_16_BIT (1)

[VoxelBuffer.CHANNEL_TYPE](VoxelBuffer.md#i_CHANNEL_TYPE) 的位深。仅支持 8 位和 16 位位深。

## 方法描述

### [void](#)<span id="i_configure_buffer"></span> **configure_buffer**( [VoxelBuffer](VoxelBuffer.md) buffer ) 

使用当前格式的属性清除并格式化 [VoxelBuffer](VoxelBuffer.md)。应在尚未修改过的缓冲区上使用。

### [VoxelBuffer](VoxelBuffer.md)<span id="i_create_buffer"></span> **create_buffer**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) size ) 

创建一个具有当前格式的新 [VoxelBuffer](VoxelBuffer.md)。

### [Depth](VoxelBuffer.md#enumerations)<span id="i_get_channel_depth"></span> **get_channel_depth**( [ChannelId](VoxelBuffer.md#enumerations) channel_index ) 

获取特定通道的位深。更多信息请参见 [Depth](VoxelBuffer.md#enumerations)。

### [void](#)<span id="i_set_channel_depth"></span> **set_channel_depth**( [ChannelId](VoxelBuffer.md#enumerations) channel_index, [Depth](VoxelBuffer.md#enumerations) depth ) 

设置特定通道的位深。更多信息请参见 [Depth](VoxelBuffer.md#enumerations)。

_生成于 2026-08-28_
