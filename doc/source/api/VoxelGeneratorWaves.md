# VoxelGeneratorWaves

继承自：[VoxelGeneratorHeightmap](VoxelGeneratorHeightmap.md)

生成波浪形地形图案的体素生成器。

## 属性：


类型                                                                            | 名称                                   | 默认值             
----------------------------------------------------------------------------- | ------------------------------------ | ----------------
[Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)  | [pattern_offset](#i_pattern_offset)  | Vector2(0, 0)   
[Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)  | [pattern_size](#i_pattern_size)      | Vector2(30, 30) 
<p></p>

## 属性描述

### [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)<span id="i_pattern_offset"></span> **pattern_offset** = Vector2(0, 0)

波浪的偏移（或相位）。

### [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html)<span id="i_pattern_size"></span> **pattern_size** = Vector2(30, 30)

波浪的长度。注意这仅控制 X 和 Z 轴方向上的长度。高度由 [VoxelGeneratorHeightmap.height_start](VoxelGeneratorHeightmap.md#i_height_start) 和 [VoxelGeneratorHeightmap.height_range](VoxelGeneratorHeightmap.md#i_height_range) 控制。

_生成于 2026-09-12_
