# VoxelGeneratorHeightmap

继承自：[VoxelGenerator](VoxelGenerator.md)

派生：[VoxelGeneratorImage](VoxelGeneratorImage.md), [VoxelGeneratorNoise2D](VoxelGeneratorNoise2D.md), [VoxelGeneratorWaves](VoxelGeneratorWaves.md)

多个基本高度生成器的基类。

## 属性：


类型                                                                              | 名称                               | 默认值             
------------------------------------------------------------------------------- | -------------------------------- | ----------------
[ChannelId](VoxelBuffer.md#enumerations)                                        | [channel](#i_channel)            | CHANNEL_SDF (1) 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)        | [height_range](#i_height_range)  | 30.0            
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)        | [height_start](#i_height_start)  | -50.0           
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)        | [iso_scale](#i_iso_scale)        | 1.0             
[Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html)  | [offset](#i_offset)              | Vector2i(0, 0)  
<p></p>

## 属性描述

### [ChannelId](VoxelBuffer.md#enumerations)<span id="i_channel"></span> **channel** = CHANNEL_SDF (1)

生成体素时使用的通道。如果设置为 [VoxelBuffer.CHANNEL_SDF](VoxelBuffer.md#i_CHANNEL_SDF)，体素将是可由平滑网格生成器使用的有符号距离场。否则，地面以下将设置为值 1，地面以上将设置为值 0（方块风）。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_height_range"></span> **height_range** = 30.0

可生成的最低与最高表面点之间的最大距离。 

注意：由于 Godot 文档工具的 bug，此处显示的默认值不是 30.0，而是 200.0。这似乎是因为其中一个子类 `VoxelGeneratorWaves` 具有不同的默认值，该值是为了获得更好的实际效果而选择的。尽管该属性定义在基类中，现在它也出现在某些子类中。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_height_start"></span> **height_start** = -50.0

表面生成的最小高度。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_iso_scale"></span> **iso_scale** = 1.0

使用平滑地形配置时应用于有符号距离场的缩放。

### [Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html)<span id="i_offset"></span> **offset** = Vector2i(0, 0)

沿 X 和 Z 轴偏移高度生成。

_生成于 2026-09-12_
