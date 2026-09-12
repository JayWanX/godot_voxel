# VoxelBlockyModel

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

派生：[VoxelBlockyModelCube](VoxelBlockyModelCube.md), [VoxelBlockyModelEmpty](VoxelBlockyModelEmpty.md), [VoxelBlockyModelFluid](VoxelBlockyModelFluid.md), [VoxelBlockyModelMesh](VoxelBlockyModelMesh.md)

存储在 [VoxelBlockyLibrary](VoxelBlockyLibrary.md) 中并由 [VoxelMesherBlocky](VoxelMesherBlocky.md) 使用的模型。

## 描述：

表示用于特定 TYPE 值的体素的模型。此类模型必须包含在 [VoxelBlockyLibrary](VoxelBlockyLibrary.md) 中，才能与 [VoxelTerrain](VoxelTerrain.md) 配合使用，或直接与 [VoxelMesherBlocky](VoxelMesherBlocky.md) 配合使用。

模型可以通过多种方式设置，请参见子类。

## 属性：


类型                                                                          | 名称                                           | 默认值               
--------------------------------------------------------------------------- | -------------------------------------------- | ------------------
[AABB[]](https://docs.godotengine.org/en/stable/classes/class_aabb[].html)  | [collision_aabbs](#i_collision_aabbs)        | []                
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [collision_mask](#i_collision_mask)          | 1                 
[Color](https://docs.godotengine.org/en/stable/classes/class_color.html)    | [color](#i_color)                            | Color(1, 1, 1, 1) 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)      | [culls_neighbors](#i_culls_neighbors)        | true              
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)      | [lod_skirts_enabled](#i_lod_skirts_enabled)  | true              
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)      | [random_tickable](#i_random_tickable)        | false             
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [tags_mask](#i_tags_mask)                    | 1                 
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)        | [transparency_index](#i_transparency_index)  | 0                 
<p></p>

## 方法：


返回值                                                                             | 函数签名                                                                                                                                                                                                                                
------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[Material](https://docs.godotengine.org/en/stable/classes/class_material.html)  | [get_material_override](#i_get_material_override) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) index ) const                                                                                              
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [get_mesh_ortho_rotation_index](#i_get_mesh_ortho_rotation_index) ( ) const                                                                                                                                                         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [is_mesh_collision_enabled](#i_is_mesh_collision_enabled) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) surface_index ) const                                                                              
[void](#)                                                                       | [rotate_90](#i_rotate_90) ( [Axis](https://docs.godotengine.org/en/stable/classes/class_vector3i.html#enum-vector3i-axis) axis, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) clockwise )                  
[void](#)                                                                       | [set_material_override](#i_set_material_override) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) index, [Material](https://docs.godotengine.org/en/stable/classes/class_material.html) material )           
[void](#)                                                                       | [set_mesh_collision_enabled](#i_set_mesh_collision_enabled) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) surface_index, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled )  
[void](#)                                                                       | [set_mesh_ortho_rotation_index](#i_set_mesh_ortho_rotation_index) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) i )                                                                                        
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **Side**：

- <span id="i_SIDE_NEGATIVE_X"></span>**SIDE_NEGATIVE_X** = **1**
- <span id="i_SIDE_POSITIVE_X"></span>**SIDE_POSITIVE_X** = **0**
- <span id="i_SIDE_NEGATIVE_Y"></span>**SIDE_NEGATIVE_Y** = **2**
- <span id="i_SIDE_POSITIVE_Y"></span>**SIDE_POSITIVE_Y** = **3**
- <span id="i_SIDE_NEGATIVE_Z"></span>**SIDE_NEGATIVE_Z** = **4**
- <span id="i_SIDE_POSITIVE_Z"></span>**SIDE_POSITIVE_Z** = **5**
- <span id="i_SIDE_COUNT"></span>**SIDE_COUNT** = **6**


## 属性描述

### [AABB[]](https://docs.godotengine.org/en/stable/classes/class_aabb[].html)<span id="i_collision_aabbs"></span> **collision_aabbs** = []

相对于模型的包围盒列表。它们用于基于盒子的碰撞，使用 [VoxelBoxMover](VoxelBoxMover.md)。它们不用于基于网格的碰撞。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_collision_mask"></span> **collision_mask** = 1

用于基于盒子的碰撞 [VoxelBoxMover](VoxelBoxMover.md) 和体素射线检测（[VoxelToolTerrain](VoxelToolTerrain.md)）的碰撞掩码。它不用于基于网格的碰撞。

### [Color](https://docs.godotengine.org/en/stable/classes/class_color.html)<span id="i_color"></span> **color** = Color(1, 1, 1, 1)

模型的颜色。当构建到体素网格中时，它将用于调整模型颜色。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_culls_neighbors"></span> **culls_neighbors** = true

如果启用，此体素会剔除其相邻体素的面。对于较密集的透明体素（如树叶），禁用可能会很有用。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_lod_skirts_enabled"></span> **lod_skirts_enabled** = true

如果启用且地形具有 LOD，此模型在位于数据块边缘时会产生“裙边”。这是为了隐藏不同 LOD 数据块之间的“裂缝”。

如果模型是透明的，你可能需要关闭此选项，因为裙边会从其他表面背后显现出来。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_random_tickable"></span> **random_tickable** = false

如果启用，TYPE 通道中具有此 ID 的体素将被 [VoxelToolTerrain.run_blocky_random_tick](VoxelToolTerrain.md#i_run_blocky_random_tick) 使用。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_tags_mask"></span> **tags_mask** = 1

用于在某些操作中过滤此模型的位掩码。例如，参见 [VoxelToolTerrain.run_blocky_random_tick](VoxelToolTerrain.md#i_run_blocky_random_tick)。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_transparency_index"></span> **transparency_index** = 0

确定当模型各面被相邻体素剔除时如何处理透明度。

如果某一侧的相邻体素的透明度索引低于或等于当前体素，则该面将被剔除。

## 方法描述

### [Material](https://docs.godotengine.org/en/stable/classes/class_material.html)<span id="i_get_material_override"></span> **get_material_override**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) index ) 

获取模型特定表面的材质覆盖。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_mesh_ortho_rotation_index"></span> **get_mesh_ortho_rotation_index**( ) 

获取库烘焙时将应用到模型的 90 度旋转 ID。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_mesh_collision_enabled"></span> **is_mesh_collision_enabled**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) surface_index ) 

判断特定表面是否生成基于网格的碰撞。

### [void](#)<span id="i_rotate_90"></span> **rotate_90**( [Axis](https://docs.godotengine.org/en/stable/classes/class_vector3i.html#enum-vector3i-axis) axis, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) clockwise ) 

将模型绕指定轴旋转 90 度。

### [void](#)<span id="i_set_material_override"></span> **set_material_override**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) index, [Material](https://docs.godotengine.org/en/stable/classes/class_material.html) material ) 

为模型的特定表面设置材质覆盖。它允许在多个模型上使用同一个网格，但每个模型使用不同的材质。

### [void](#)<span id="i_set_mesh_collision_enabled"></span> **set_mesh_collision_enabled**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) surface_index, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled ) 

启用或禁用特定表面上的基于网格的碰撞。它允许模型既有实心部分，也有玩家可以穿过的部分。

### [void](#)<span id="i_set_mesh_ortho_rotation_index"></span> **set_mesh_ortho_rotation_index**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) i ) 

获取库烘焙时将应用到模型的 90 度旋转 ID。这是一个代表 24 种可能 90 度旋转之一的数字。你也可以使用 [rotate_90](VoxelBlockyModel.md#i_rotate_90)。

_生成于 2026-09-12_
