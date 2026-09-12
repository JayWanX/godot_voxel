# VoxelToolLodTerrain

继承自：[VoxelTool](VoxelTool.md)

专用于 [VoxelLodTerrain](VoxelLodTerrain.md) 的 [VoxelTool](VoxelTool.md) 实现。

## 描述：

此类中的函数特定于 [VoxelLodTerrain](VoxelLodTerrain.md)。对于通用函数，你也可以查看 [VoxelTool](VoxelTool.md)。

它不是一个可以单独实例化的类，你可以使用 `get_voxel_tool()` 方法从 [VoxelLodTerrain](VoxelLodTerrain.md) 获取它。

## 方法：


返回值                                                                       | 函数签名                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                 | [do_graph](#i_do_graph) ( [VoxelGeneratorGraph](VoxelGeneratorGraph.md) graph, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) area_size )                                                                                                                                                                                                                         
[void](#)                                                                 | [do_hemisphere](#i_do_hemisphere) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) flat_direction, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) smoothness=0.0 )                                                                                                
[void](#)                                                                 | [do_sphere_async](#i_do_sphere_async) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius )                                                                                                                                                                                                                                                                                  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [get_raycast_binary_search_iterations](#i_get_raycast_binary_search_iterations) ( ) const                                                                                                                                                                                                                                                                                                                                                                                                       
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [get_voxel_f_interpolated](#i_get_voxel_f_interpolated) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) position ) const                                                                                                                                                                                                                                                                                                                                         
[void](#)                                                                 | [run_blocky_random_tick](#i_run_blocky_random_tick) ( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) area, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) voxel_count, [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) batch_count=16, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) tags_mask=4294967295 )  
[Array](https://docs.godotengine.org/en/stable/classes/class_array.html)  | [separate_floating_chunks](#i_separate_floating_chunks) ( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) box, [Node](https://docs.godotengine.org/en/stable/classes/class_node.html) parent_node )                                                                                                                                                                                                                                                                      
[void](#)                                                                 | [set_raycast_binary_search_iterations](#i_set_raycast_binary_search_iterations) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) iterations )                                                                                                                                                                                                                                                                                                                             
[void](#)                                                                 | [stamp_sdf](#i_stamp_sdf) ( [VoxelMeshSDF](VoxelMeshSDF.md) mesh_sdf, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) isolevel, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sdf_scale )  *(deprecated)*                                                                                                                                   
<p></p>

## 方法描述

### [void](#)<span id="i_do_graph"></span> **do_graph**( [VoxelGeneratorGraph](VoxelGeneratorGraph.md) graph, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) area_size ) 

使用 [VoxelGeneratorGraph](VoxelGeneratorGraph.md) 作为画笔，允许对画笔执行的 SDF 操作进行编程。

该图必须具有 SDF 输入和 SDF 输出，并且最好假定形状为单位大小。例如，加法球体画笔可以使用半径为 1 的 `SdfSphere` 节点配合 `Min` 将其与地形 SDF 组合。

另请参阅[在线文档](https://voxel-tools.readthedocs.io/en/latest/generators/#using-voxelgeneratorgraph-as-a-brush)。

### [void](#)<span id="i_do_hemisphere"></span> **do_hemisphere**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) flat_direction, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) smoothness=0.0 ) 

对半球进行操作，其中 `flat_direction` 指向远离平面表面（类似于法线）。`smoothness` 决定平坦部分与圆角部分的混合程度，值越高，边缘越柔和、越圆润。

### [void](#)<span id="i_do_sphere_async"></span> **do_sphere_async**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius ) 

异步执行球形编辑。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_raycast_binary_search_iterations"></span> **get_raycast_binary_search_iterations**( ) 

获取射线检测的二分搜索迭代次数。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_get_voxel_f_interpolated"></span> **get_voxel_f_interpolated**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) position ) 

读取指定位置插值后的浮点体素值。

### [void](#)<span id="i_run_blocky_random_tick"></span> **run_blocky_random_tick**( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) area, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) voxel_count, [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) batch_count=16, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) tags_mask=4294967295 ) 

在指定区域内选取随机体素。如果体素模型将 [VoxelBlockyModel.random_tickable](VoxelBlockyModel.md#i_random_tickable) 设置为 `true` 且 [VoxelBlockyModel.tags_mask](VoxelBlockyModel.md#i_tags_mask) 与 `tags_mask` 中的任何位匹配，则对它们执行函数。这仅适用于使用 [VoxelMesherBlocky](VoxelMesherBlocky.md) 的地形。

给定的回调接受两个参数：体素位置（Vector3i）、体素值（int）。


`batch_count` 的目的是通过内部数据结构优化选取过程。算法如下：`voxel_count` 被分成若干个长度为 `batch_count` 的批次。对于每个批次，会选择与 `area` 相交的随机块，并在其中随机选取 `batch_count` 个体素。如果 `voxel_count` 不能被 `batch_count` 整除，将额外选取一个块来处理余数。

`batch` 可以通过将选取集中在特定块中来引入随机性偏差，但如果此函数随时间每帧调用，该偏差应该会平均化。如果你完全不想要偏差，请将 `batch_count` 设置为 1。

### [Array](https://docs.godotengine.org/en/stable/classes/class_array.html)<span id="i_separate_floating_chunks"></span> **separate_floating_chunks**( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) box, [Node](https://docs.godotengine.org/en/stable/classes/class_node.html) parent_node ) 

将漂浮的体素转换为刚体。

在长方体内部检测漂浮体素块。该长方体相对于此 VoxelTool 所附着的体素体积。块必须完全包含在该长方体内部才会被视为漂浮。块会从源体积中移除，并转换为具有凸碰撞形状的刚体。它们将被添加为所提供节点的子节点。它们将以“kinematic”状态开始，并在短暂时间后变为“rigid”，以便地形在移除后更新其碰撞体（否则它们会重叠）。该函数返回这些刚体的数组，你可以用它来为刚体附加进一步的行为（例如在一段时间或距离后消失）。

此算法会很快变得昂贵，因此长方体不应太大。大约 30 个体素的大小应该可以。

### [void](#)<span id="i_set_raycast_binary_search_iterations"></span> **set_raycast_binary_search_iterations**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) iterations ) 

在指定区域内选取随机体素并对其执行函数。这仅适用于使用 [VoxelMesherBlocky](VoxelMesherBlocky.md) 的地形。只会选取 [Voxel.random_tickable](https://docs.godotengine.org/en/stable/classes/class_voxel.html#class-voxel-property-random-tickable) 为 `true` 的体素。

给定的回调接受两个参数：体素位置（Vector3i）、体素值（int）。

只会考虑 LOD 0 处的体素。

### [void](#)<span id="i_stamp_sdf"></span> **stamp_sdf**( [VoxelMeshSDF](VoxelMeshSDF.md) mesh_sdf, [Transform3D](https://docs.godotengine.org/en/stable/classes/class_transform3d.html) transform, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) isolevel, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) sdf_scale ) 

*此方法已弃用。 Use [VoxelTool.do_mesh](VoxelTool.md#i_do_mesh) instead.*
用网格 SDF 印章雕刻地形。

_生成于 2026-09-12_
