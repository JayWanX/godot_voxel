# VoxelGeneratorNoise2D

继承自：[VoxelGeneratorHeightmap](VoxelGeneratorHeightmap.md)

生成基于噪声的高度图地形的体素生成器。

## 属性：


类型                                                                        | 名称                               | 默认值   
------------------------------------------------------------------------- | -------------------------------- | ------
[Curve](https://docs.godotengine.org/en/stable/classes/class_curve.html)  | [curve](#i_curve)                |       
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [height_range](#i_height_range)  | 200.0 
[Noise](https://docs.godotengine.org/en/stable/classes/class_noise.html)  | [noise](#i_noise)                |       
<p></p>

## 属性描述

### [Curve](https://docs.godotengine.org/en/stable/classes/class_curve.html)<span id="i_curve"></span> **curve**

当赋值时，此曲线将改变高度变化的分布，从而可以为生成的形状提供某种"轮廓"。

默认情况下使用从 0 到 1 的线性曲线。

假定曲线的定义域从 0 到 1。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_height_range"></span> **height_range** = 200.0


### [Noise](https://docs.godotengine.org/en/stable/classes/class_noise.html)<span id="i_noise"></span> **noise**

用于生成高度图的噪声。生成器需要它才能工作。

_生成于 2026-09-12_
