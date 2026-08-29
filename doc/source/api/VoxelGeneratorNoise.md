# VoxelGeneratorNoise

继承自：[VoxelGenerator](VoxelGenerator.md)

使用 3D 噪声生成悬垂形状的体素生成器。

## 属性：


类型                                                                                        | 名称                               | 默认值             
----------------------------------------------------------------------------------------- | -------------------------------- | ----------------
[ChannelId](VoxelBuffer.md#enumerations)                                                  | [channel](#i_channel)            | CHANNEL_SDF (1) 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                  | [height_range](#i_height_range)  | 200.0           
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                  | [height_start](#i_height_start)  | -100.0          
[FastNoiseLite](https://docs.godotengine.org/en/stable/classes/class_fastnoiselite.html)  | [noise](#i_noise)                |                 
<p></p>

## 属性描述

### [ChannelId](VoxelBuffer.md#enumerations)<span id="i_channel"></span> **channel** = CHANNEL_SDF (1)

生成器将体素数据写入的通道。这取决于你需要的网格化类型。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_height_range"></span> **height_range** = 200.0

形状变化将出现的高度范围。此范围越大，悬垂就越多。

形状在靠近底部时密度最大（找到空气的概率低），并逐渐降低密度，直到达到最大高度。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_height_start"></span> **height_start** = -100.0

形状的底部。其下方的所有内容都将被地面填充。

### [FastNoiseLite](https://docs.godotengine.org/en/stable/classes/class_fastnoiselite.html)<span id="i_noise"></span> **noise**

用作密度函数的噪声。生成器需要它才能工作。

_生成于 2026-08-28_
