# VoxelBlockyModelCube

继承自：[VoxelBlockyModel](VoxelBlockyModel.md)

生成一个在各面上具有特定贴图的立方体模型。

## 属性：


类型                                                                              | 名称                                                         | 默认值                      
------------------------------------------------------------------------------- | ---------------------------------------------------------- | -------------------------
[Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html)  | [atlas_size_in_tiles](#i_atlas_size_in_tiles)              | Vector2i(16, 16)         
[AABB[]](https://docs.godotengine.org/en/stable/classes/class_aabb[].html)      | [collision_aabbs](#i_collision_aabbs)                      | [AABB(0, 0, 0, 1, 1, 1)] 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)        | [height](#i_height)                                        | 1.0                      
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [mesh_ortho_rotation_index](#i_mesh_ortho_rotation_index)  | 0                        
<p></p>

## 方法：


返回值                                                                             | 函数签名                                                                                                                                                                
------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------
[Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html)  | [get_tile](#i_get_tile) ( [Side](VoxelBlockyModel.md#enumerations) side ) const                                                                                     
[void](#)                                                                       | [set_tile](#i_set_tile) ( [Side](VoxelBlockyModel.md#enumerations) side, [Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html) position )  
<p></p>

## 属性描述

### [Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html)<span id="i_atlas_size_in_tiles"></span> **atlas_size_in_tiles** = Vector2i(16, 16)

设置纹理图集的参考尺寸（以贴图数量为单位）。必须设置它，模型才能根据指定的贴图位置生成正确的纹理坐标。

如果你不使用图集，且每个面都使用相同的完整纹理，请使用 (1,1)。

### [AABB[]](https://docs.godotengine.org/en/stable/classes/class_aabb[].html)<span id="i_collision_aabbs"></span> **collision_aabbs** = [AABB(0, 0, 0, 1, 1, 1)]


### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_height"></span> **height** = 1.0

*(此属性暂无文档)*

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_mesh_ortho_rotation_index"></span> **mesh_ortho_rotation_index** = 0

*(此属性暂无文档)*

## 方法描述

### [Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html)<span id="i_get_tile"></span> **get_tile**( [Side](VoxelBlockyModel.md#enumerations) side ) 

*(此方法暂无文档)*

### [void](#)<span id="i_set_tile"></span> **set_tile**( [Side](VoxelBlockyModel.md#enumerations) side, [Vector2i](https://docs.godotengine.org/en/stable/classes/class_vector2i.html) position ) 

*(此方法暂无文档)*

_生成于 2026-08-28_
