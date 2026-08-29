# VoxelMesherCubes

继承自：[VoxelMesher](VoxelMesher.md)

根据颜色生成立方体体素网格。

## 描述：

基于存储在 [VoxelBuffer.CHANNEL_COLOR](VoxelBuffer.md#i_CHANNEL_COLOR) 通道中的颜色值，构建表示立方体体素的网格。

颜色通常会存储在 `COLOR` 顶点属性中，可能需要启用顶点颜色的材质，或使用自定义着色器。

## 属性：


类型                                                                              | 名称                                                   | 默认值           
------------------------------------------------------------------------------- | ---------------------------------------------------- | --------------
[ColorMode](VoxelMesherCubes.md#enumerations)                                   | [color_mode](#i_color_mode)                          | COLOR_RAW (0) 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [greedy_meshing_enabled](#i_greedy_meshing_enabled)  | true          
[Material](https://docs.godotengine.org/en/stable/classes/class_material.html)  | [opaque_material](#i_opaque_material)                |               
[VoxelColorPalette](VoxelColorPalette.md)                                       | [palette](#i_palette)                                |               
[Material](https://docs.godotengine.org/en/stable/classes/class_material.html)  | [transparent_material](#i_transparent_material)      |               
<p></p>

## 方法：


返回值                                                                     | 函数签名                                                                                                                                                                                                                                   
----------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)  | [generate_mesh_from_image](#i_generate_mesh_from_image) ( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) image, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) voxel_size ) static 
[void](#)                                                               | [set_material_by_index](#i_set_material_by_index) ( [Materials](VoxelMesherCubes.md#enumerations) id, [Material](https://docs.godotengine.org/en/stable/classes/class_material.html) material )                                        
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **Materials**：

- <span id="i_MATERIAL_OPAQUE"></span>**MATERIAL_OPAQUE** = **0** --- 不透明材质的索引。
- <span id="i_MATERIAL_TRANSPARENT"></span>**MATERIAL_TRANSPARENT** = **1** --- 透明材质的索引。
- <span id="i_MATERIAL_COUNT"></span>**MATERIAL_COUNT** = **2** --- 材质的最大数量。

枚举 **ColorMode**：

- <span id="i_COLOR_RAW"></span>**COLOR_RAW** = **0** --- 体素值将被直接解释为颜色。 8 位体素被解释为 `rrggbbaa`（每个分量 2 位），每个分量的范围从 0..3 转换为 0..255。 16 位体素被解释为 `rrrrgggg bbbbaaaa`（每个分量 4 位），每个分量的范围从 0..15 转换为 0..255。 32 位体素被解释为 `rrrrrrrr gggggggg bbbbbbbb aaaaaaaa`（每个分量 8 位），每个分量在 0..255 之间。
- <span id="i_COLOR_MESHER_PALETTE"></span>**COLOR_MESHER_PALETTE** = **1** --- 体素值将被解释为 [palette](VoxelMesherCubes.md#i_palette) 属性中分配的调色板内的索引。
- <span id="i_COLOR_SHADER_PALETTE"></span>**COLOR_SHADER_PALETTE** = **2** --- 体素值将按原样直接写入网格，而不是颜色。 它们被写入 `COLOR` 的红色分量，红色和蓝色保持为零。注意，它会在着色器中被归一化到 0..1，因此如果你需要取回整数值，可以使用 `int(COLOR.r * 255.0)`。 alpha 分量将设置为 [palette](VoxelMesherCubes.md#i_palette) 中对应颜色的透明度（仍然需要一个调色板资源来区分透明部分；RGB 值不会被使用）。 你需要使用 [ShaderMaterial](https://docs.godotengine.org/en/stable/classes/class_shadermaterial.html) 来读取顶点数据，并通过自定义着色器选择实际颜色。[StandardMaterial](https://docs.godotengine.org/en/stable/classes/class_standardmaterial.html) 无法与此模式配合使用。


## 属性描述

### [ColorMode](VoxelMesherCubes.md#enumerations)<span id="i_color_mode"></span> **color_mode** = COLOR_RAW (0)

设置构建网格时如何确定体素颜色。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_greedy_meshing_enabled"></span> **greedy_meshing_enabled** = true

启用贪婪网格化：网格生成器将尝试合并具有相同颜色的连续面，以减少多边形数量。

### [Material](https://docs.godotengine.org/en/stable/classes/class_material.html)<span id="i_opaque_material"></span> **opaque_material**

将用于网格不透明部分的材质。

### [VoxelColorPalette](VoxelColorPalette.md)<span id="i_palette"></span> **palette**

使用 [COLOR_MESHER_PALETTE](VoxelMesherCubes.md#i_COLOR_MESHER_PALETTE) 颜色模式时将使用的调色板。

### [Material](https://docs.godotengine.org/en/stable/classes/class_material.html)<span id="i_transparent_material"></span> **transparent_material**

将用于网格透明部分的材质（alpha 未设为最大的颜色）。

## 方法描述

### [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)<span id="i_generate_mesh_from_image"></span> **generate_mesh_from_image**( [Image](https://docs.godotengine.org/en/stable/classes/class_image.html) image, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) voxel_size ) 

根据图像像素生成 1 体素厚的贪婪网格。

### [void](#)<span id="i_set_material_by_index"></span> **set_material_by_index**( [Materials](VoxelMesherCubes.md#enumerations) id, [Material](https://docs.godotengine.org/en/stable/classes/class_material.html) material ) 

设置构建网格时将使用的其中一个材质。这等效于使用 [opaque_material](VoxelMesherCubes.md#i_opaque_material) 或 [transparent_material](VoxelMesherCubes.md#i_transparent_material)。

_生成于 2026-08-28_
