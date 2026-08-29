# VoxelGeneratorFlat

继承自：[VoxelGenerator](VoxelGenerator.md)

生成无限平坦地面的体素生成器。

## 属性：


类型                                                                        | 名称                           | 默认值             
------------------------------------------------------------------------- | ---------------------------- | ----------------
[ChannelId](VoxelBuffer.md#enumerations)                                  | [channel](#i_channel)        | CHANNEL_SDF (1) 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [height](#i_height)          | 0.0             
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [voxel_type](#i_voxel_type)  | 1               
<p></p>

## 属性描述

### [ChannelId](VoxelBuffer.md#enumerations)<span id="i_channel"></span> **channel** = CHANNEL_SDF (1)

用于生成地面的通道。使用 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 生成平滑地形，使用其他通道生成方块风地形。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_height"></span> **height** = 0.0

地面的高度。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_voxel_type"></span> **voxel_type** = 1

如果 [channel](VoxelGeneratorFlat.md#i_channel) 设置为 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF) 以外的任何通道，此值将用于填充地面体素，而空气体素将设置为 0。

_生成于 2026-08-28_
