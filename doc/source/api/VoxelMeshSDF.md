# VoxelMeshSDF

继承自：[Resource](https://docs.godotengine.org/en/stable/classes/class_resource.html)

网格作为体素的有符号距离场。

## 描述：

此资源可用于将网格烘焙为有符号距离场（SDF）。数据存储为内部体素缓冲区。随后可将其用作基于体素的运算中的形状或笔刷。


注意：如果可以，建议优先使用程序化形状（如球体或盒体），以获得更好的质量和性能。虽然 [VoxelMeshSDF](VoxelMeshSDF.md) 用途广泛，但使用成本更高，且目前质量低于程序化等效物。


注意 2：并非所有网格都能被烘焙。最适合烘焙的网格应是流形且表示封闭形状，具有清晰的内部和外部。

## 属性：


类型                                                                                  | 名称                                                         | 默认值                                
----------------------------------------------------------------------------------- | ---------------------------------------------------------- | -----------------------------------
[Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)  | [_data](#i__data)                                          | {}                                 
[BakeMode](VoxelMeshSDF.md#enumerations)                                            | [bake_mode](#i_bake_mode)                                  | BAKE_MODE_ACCURATE_PARTITIONED (1) 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)              | [boundary_sign_fix_enabled](#i_boundary_sign_fix_enabled)  | true                               
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                | [cell_count](#i_cell_count)                                | 64                                 
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)            | [margin_ratio](#i_margin_ratio)                            | 0.25                               
[Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)              | [mesh](#i_mesh)                                            |                                    
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                | [partition_subdiv](#i_partition_subdiv)                    | 32                                 
<p></p>

## 方法：


返回值                                                                       | 函数签名                                                                                                                         
------------------------------------------------------------------------- | -----------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                 | [bake](#i_bake) ( )                                                                                                          
[void](#)                                                                 | [bake_async](#i_bake_async) ( [SceneTree](https://docs.godotengine.org/en/stable/classes/class_scenetree.html) scene_tree )  
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)  | [debug_check_sdf](#i_debug_check_sdf) ( [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html) mesh )        
[AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html)    | [get_aabb](#i_get_aabb) ( ) const                                                                                            
[VoxelBuffer](VoxelBuffer.md)                                             | [get_voxel_buffer](#i_get_voxel_buffer) ( ) const                                                                            
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [is_baked](#i_is_baked) ( ) const                                                                                            
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [is_baking](#i_is_baking) ( ) const                                                                                          
<p></p>

## 信号：<span id="signals"></span>

### baked( ) 

当异步烘焙完成时发出。

## 枚举：<span id="enumerations"></span>

枚举 **BakeMode**：

- <span id="i_BAKE_MODE_ACCURATE_NAIVE"></span>**BAKE_MODE_ACCURATE_NAIVE** = **0** --- 从每个单元检查每个三角形以找到最近距离。它力求精确，但比其他技术慢得多。网格必须是封闭的，否则 SDF 将包含错误。通过获取最近的三角形并检查单元中心位于三角形的哪一侧来计算符号。
- <span id="i_BAKE_MODE_ACCURATE_PARTITIONED"></span>**BAKE_MODE_ACCURATE_PARTITIONED** = **1** --- 与朴素方法类似，但将空间细分为分区，以便更容易跳过无需检查的三角形。比朴素方法快，但仍相对较慢。
- <span id="i_BAKE_MODE_APPROX_INTERP"></span>**BAKE_MODE_APPROX_INTERP** = **2** --- 实验性方法，将空间细分为 4x4x4 的单元，如果这些单元中没有三角形，则通过插值 8 个角来跳过 SDF 计算。比朴素方法快，但并不是特别令人感兴趣。可能会被移除。
- <span id="i_BAKE_MODE_APPROX_FLOODFILL"></span>**BAKE_MODE_APPROX_FLOODFILL** = **3** --- 通过计算三角形附近一薄层精确值的"外壳"，然后用 26 方向泛洪填充传播这些值，来近似 SDF。符号仅在初始外壳上通过从每个单元中心进行多次射线投射来计算：如果射线击中背面，则假定单元在内部。否则，假定在外部。符号作为泛洪填充的一部分进行传播。虽然技术上不精确，但它目前是最快的方法，且结果通常足够好。
- <span id="i_BAKE_MODE_COUNT"></span>**BAKE_MODE_COUNT** = **4** --- 烘焙模式的数量。


## 属性描述

### [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)<span id="i__data"></span> **_data** = {}

*(此属性暂无文档)*

### [BakeMode](VoxelMeshSDF.md#enumerations)<span id="i_bake_mode"></span> **bake_mode** = BAKE_MODE_ACCURATE_PARTITIONED (1)

选择用于从网格计算 SDF 的算法。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_boundary_sign_fix_enabled"></span> **boundary_sign_fix_enabled** = true

某些算法可能无法自行正确计算距离的符号。通常形状外部为正号，形状内部为负号。错误可能导致错误的符号"泄漏"并传播到形状外部。

此选项会在发生这种情况时尝试修复：假定烘焙区域的边界始终位于外部，并向内传播正号，以确保没有负号泄漏。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_cell_count"></span> **cell_count** = 64

控制烘焙 SDF 的分辨率，相对于 3D 体素缓冲区的某一侧。增大可提高质量，但烘焙更慢且占用更多内存。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_margin_ratio"></span> **margin_ratio** = 0.25

控制在网格烘焙区域周围添加额外空间。

合适的 SDF 缓冲区应留有一定边距，以捕获外部梯度。如果没有边距，重新网格化时外部可能会出现截断或方块化。

此属性基于网格尺寸的比例添加边距。例如，如果网格宽 100 个单位，则 0.1 的边距将在其周围添加 10 个额外单位的空间。

### [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html)<span id="i_mesh"></span> **mesh**

调用 [bake](VoxelMeshSDF.md#i_bake) 函数时或按下编辑器中的 Bake 按钮时将烘焙的网格。

将此属性设回 null 不会擦除烘焙结果，因此你不需要加载原始网格即可使用 SDF。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_partition_subdiv"></span> **partition_subdiv** = 32

使用 [BAKE_MODE_ACCURATE_PARTITIONED](VoxelMeshSDF.md#i_BAKE_MODE_ACCURATE_PARTITIONED) 和 [BAKE_MODE_APPROX_FLOODFILL](VoxelMeshSDF.md#i_BAKE_MODE_APPROX_FLOODFILL) 模式时，控制在烘焙区域内使用的细分数量。

使用 [BAKE_MODE_ACCURATE_PARTITIONED](VoxelMeshSDF.md#i_BAKE_MODE_ACCURATE_PARTITIONED) 时，由于算法的缺陷，该值可能会根据网格的三角形数量按比例调整。如果网格三角形较少，较低的值表现更好。如果有很多小三角形，较高的值表现更好。然而，如果三角形大或数量少而值又较大，这也可能产生伪影。

## 方法描述

### [void](#)<span id="i_bake"></span> **bake**( ) 

在调用线程上，使用当前分配的 [mesh](VoxelMeshSDF.md#i_mesh) 烘焙 SDF。如果在主线程上执行，可能导致游戏卡顿。

### [void](#)<span id="i_bake_async"></span> **bake_async**( [SceneTree](https://docs.godotengine.org/en/stable/classes/class_scenetree.html) scene_tree ) 

在单独的线程上，使用当前分配的 [mesh](VoxelMeshSDF.md#i_mesh) 烘焙 SDF。另请参见 [is_baking](VoxelMeshSDF.md#i_is_baking)、[is_baked](VoxelMeshSDF.md#i_is_baked) 和 [VoxelMeshSDF.baked](VoxelMeshSDF.md#signals)。

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i_debug_check_sdf"></span> **debug_check_sdf**( [Mesh](https://docs.godotengine.org/en/stable/classes/class_mesh.html) mesh ) 

实验性。

运行一些检查，以验证烘焙后的 SDF 是否包含错误。传入的网格应为用于烘焙的网格。

如果有错误，返回的数组将包含缓冲区中最多 2 个包含错误值的单元的信息，格式如下：

```
[
	cell0_position: Vector3,
	cell0_triangle_v0: Vector3,
	cell0_triangle_v1: Vector3,
	cell0_triangle_v2: Vector3,
	cell1_position: Vector3,
	cell1_triangle_v0: Vector3,
	cell1_triangle_v1: Vector3,
	cell1_triangle_v2: Vector3,
]
```
如果没有错误，返回的数组将为空。

此方法是在此资源最初实现时遗留的，当时尝试在调试时自动化检查。根据用例的不同，错误并不总意味着 SDF 不可用。它可能会在未来被移除或更改。

### [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html)<span id="i_get_aabb"></span> **get_aabb**( ) 

获取烘焙形状的参考包围盒。由于 [margin_ratio](VoxelMeshSDF.md#i_margin_ratio) 属性，它可能比原始网格的 AABB 稍大。

### [VoxelBuffer](VoxelBuffer.md)<span id="i_get_voxel_buffer"></span> **get_voxel_buffer**( ) 

获取包含烘焙距离场的 [VoxelBuffer](VoxelBuffer.md)。结果将存储在 SDF 通道中。

不应修改此缓冲区。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_baked"></span> **is_baked**( ) 

获取资源是否包含已烘焙的 SDF 数据。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_baking"></span> **is_baking**( ) 

获取是否存在待处理的异步烘焙操作。

_生成于 2026-09-12_
