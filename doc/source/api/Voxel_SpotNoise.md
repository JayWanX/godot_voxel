# Voxel_SpotNoise

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

细胞噪声的特化，用于廉价的"矿脉"生成或确定性的点散射。

## 描述：

将空间划分为网格，其中每个单元格包含一个圆形的"点"。当位置位于某个点内部时，噪声求值返回 1，否则返回 0。

局限：高抖动可能导致点与单元格边界重叠。这是预期的行为。如果你需要更高质量（但更慢）的结果，可以使用其它噪声库，例如 [FastNoiseLite](https://docs.godotengine.org/en/stable/classes/class_fastnoiselite.html)。

## 属性：


类型                                                                        | 名称                             | 默认值  
------------------------------------------------------------------------- | ------------------------------ | -----
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [cell_size](#i_cell_size)      | 32.0 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [jitter](#i_jitter)            | 0.9  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [seed](#i_seed)                | 1337 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [spot_radius](#i_spot_radius)  | 3.0  
<p></p>

## 方法：


返回值                                                                                                 | 函数签名                                                                                                                                                                                                                                                                         
--------------------------------------------------------------------------------------------------- | -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                            | [get_noise_2d](#i_get_noise_2d) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) x, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) y ) const                                                                             
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                            | [get_noise_2dv](#i_get_noise_2dv) ( [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) pos ) const                                                                                                                                                 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                            | [get_noise_3d](#i_get_noise_3d) ( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) x, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) y, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) z ) const 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)                            | [get_noise_3dv](#i_get_noise_3dv) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) pos ) const                                                                                                                                                 
[PackedVector2Array](https://docs.godotengine.org/en/stable/classes/class_packedvector2array.html)  | [get_spot_positions_in_area_2d](#i_get_spot_positions_in_area_2d) ( [Rect2](https://docs.godotengine.org/en/stable/classes/class_rect2.html) rect ) const                                                                                                                    
[PackedVector3Array](https://docs.godotengine.org/en/stable/classes/class_packedvector3array.html)  | [get_spot_positions_in_area_3d](#i_get_spot_positions_in_area_3d) ( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) aabb ) const                                                                                                                      
<p></p>

## 属性描述

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_cell_size"></span> **cell_size** = 32.0

设置每个单元格的大小。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_jitter"></span> **jitter** = 0.9

设置点的随机偏移量（抖动）。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_seed"></span> **seed** = 1337

设置随机种子。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_spot_radius"></span> **spot_radius** = 3.0

设置每个点的半径。

## 方法描述

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_2d"></span> **get_noise_2d**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) x, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) y ) 

获取给定 2D 坐标处的噪声值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_2dv"></span> **get_noise_2dv**( [Vector2](https://docs.godotengine.org/en/stable/classes/class_vector2.html) pos ) 

获取给定位置处的 2D 噪声值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_3d"></span> **get_noise_3d**( [float](https://docs.godotengine.org/en/stable/classes/class_float.html) x, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) y, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) z ) 

获取给定 3D 坐标处的噪声值。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_noise_3dv"></span> **get_noise_3dv**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) pos ) 

获取给定位置处的 3D 噪声值。

### [PackedVector2Array](https://docs.godotengine.org/en/stable/classes/class_packedvector2array.html)<span id="i_get_spot_positions_in_area_2d"></span> **get_spot_positions_in_area_2d**( [Rect2](https://docs.godotengine.org/en/stable/classes/class_rect2.html) rect ) 

获取给定矩形区域内所有点的位置。

### [PackedVector3Array](https://docs.godotengine.org/en/stable/classes/class_packedvector3array.html)<span id="i_get_spot_positions_in_area_3d"></span> **get_spot_positions_in_area_3d**( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) aabb ) 

获取给定 AABB 区域内所有点的位置。

_生成于 2026-08-28_
