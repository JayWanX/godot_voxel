# VoxelGeneratorImage

继承自：[VoxelGeneratorHeightmap](VoxelGeneratorHeightmap.md)

使用图像生成基于高度图形状的体素生成器。

## 描述：

使用图像的红色通道生成高度图，使左上角以世界原点为中心。如果地形生成范围超出图像尺寸，图像将重复。

注意：图像中的值通过 `get_pixel` 读取，并假定介于 0 和 1 之间（已归一化）。这些值将由 [VoxelGeneratorHeightmap.height_start](VoxelGeneratorHeightmap.md#i_height_start) 和 [VoxelGeneratorHeightmap.height_range](VoxelGeneratorHeightmap.md#i_height_range) 变换。

## 属性：


类型                                                                        | 名称                               | 默认值   
------------------------------------------------------------------------- | -------------------------------- | ------
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [blur_enabled](#i_blur_enabled)  | false 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [height_range](#i_height_range)  | 200.0 
[Image](https://docs.godotengine.org/en/stable/classes/class_image.html)  | [image](#i_image)                |       
<p></p>

## 属性描述

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_blur_enabled"></span> **blur_enabled** = false

是否启用图像模糊。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_height_range"></span> **height_range** = 200.0


### [Image](https://docs.godotengine.org/en/stable/classes/class_image.html)<span id="i_image"></span> **image**

设置将用作高度图的图像。只会使用红色通道。最好使用采用 `RF` 或 `RH` 格式的图像，这些格式包含更高分辨率的高度。普通图像通常只有 8 位深度，会显得有方块感。

_生成于 2026-09-12_
