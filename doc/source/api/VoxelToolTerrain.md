# VoxelToolTerrain

继承自：[VoxelTool](VoxelTool.md)

专用于 [VoxelTerrain](VoxelTerrain.md) 的 [VoxelTool](VoxelTool.md) 实现。

## 描述：

此类中的函数特定于 [VoxelTerrain](VoxelTerrain.md)。对于通用函数，你也可以查看 [VoxelTool](VoxelTool.md)。

它不是一个可以单独实例化的类，你可以使用 `get_voxel_tool()` 方法从 [VoxelTerrain](VoxelTerrain.md) 获取它。

## 方法：


返回值        | 函数签名                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)  | [do_hemisphere](#i_do_hemisphere) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) flat_direction, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) smoothness=0.0 )                                                                                                
[void](#)  | [for_each_voxel_metadata_in_area](#i_for_each_voxel_metadata_in_area) ( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) voxel_area, [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback )                                                                                                                                                                                                                                            
[void](#)  | [run_blocky_random_tick](#i_run_blocky_random_tick) ( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) area, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) voxel_count, [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) batch_count=16, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) tags_mask=4294967295 )  
<p></p>

## 方法描述

### [void](#)<span id="i_do_hemisphere"></span> **do_hemisphere**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius, [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) flat_direction, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) smoothness=0.0 ) 

对半球进行操作，其中 `flat_direction` 指向远离平面表面（类似于法线）。`smoothness` 决定平坦部分与圆角部分的混合程度，值越高，边缘越柔和、越圆润。

### [void](#)<span id="i_for_each_voxel_metadata_in_area"></span> **for_each_voxel_metadata_in_area**( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) voxel_area, [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback ) 

对给定区域内每个持有元数据的体素执行函数。

给定的回调接受两个参数：体素位置（Vector3i）、体素元数据（Variant）。

重要提示：不允许在此函数内部插入新元数据或移除元数据。

### [void](#)<span id="i_run_blocky_random_tick"></span> **run_blocky_random_tick**( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) area, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) voxel_count, [Callable](https://docs.godotengine.org/en/stable/classes/class_callable.html) callback, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) batch_count=16, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) tags_mask=4294967295 ) 

在指定区域内选取随机体素。如果体素模型将 [VoxelBlockyModel.random_tickable](VoxelBlockyModel.md#i_random_tickable) 设置为 `true` 且 [VoxelBlockyModel.tags_mask](VoxelBlockyModel.md#i_tags_mask) 与 `tags_mask` 中的任何位匹配，则对它们执行函数。这仅适用于使用 [VoxelMesherBlocky](VoxelMesherBlocky.md) 的地形。

给定的回调接受两个参数：体素位置（Vector3i）、体素值（int）。


`batch_count` 的目的是通过内部数据结构优化选取过程。算法如下：`voxel_count` 被分成若干个长度为 `batch_count` 的批次。对于每个批次，会选择与 `area` 相交的随机块，并在其中随机选取 `batch_count` 个体素。如果 `voxel_count` 不能被 `batch_count` 整除，将额外选取一个块来处理余数。

`batch` 可以通过将选取集中在特定块中来引入随机性偏差，但如果此函数随时间每帧调用，该偏差应该会平均化。如果你完全不想要偏差，请将 `batch_count` 设置为 1。

_生成于 2026-09-12_
