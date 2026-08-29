# VoxelAStarGrid3D

继承自：[RefCounted](https://docs.godotengine.org/en/stable/classes/class_refcounted.html)

!!! warning
    此类被标记为实验性。未来版本中可能发生变更或被移除。请自行判断使用风险。

基于网格的 A* 寻路算法，适用于方块风体素地形。

## 描述：

可用于在方块风地形上的两个体素位置之间寻找路径。

该算法针对高 2 个体素、宽 1 个体素的角色调优，角色必须站在实心体素上，且最多能跳 1 个体素高。

不需要导航网格，它直接使用体素而无需烘焙。不过，搜索半径受区域限制（50 个体素及以上就开始相对昂贵）。

目前，此寻路器仅将 ID 为 0 的体素视为空气，其余均视为实心。

注意：此类中的"位置"均以体素为单位。如果地形有偏移，或体素比世界单位更小或更大，则可能需要转换坐标。

## 方法：


返回值                                                                                 | 函数签名                                                                                                                                                                                                                                
----------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[Vector3i[]](https://docs.godotengine.org/en/stable/classes/class_vector3i[].html)  | [debug_get_visited_positions](#i_debug_get_visited_positions) ( ) const                                                                                                                                                             
[Vector3i[]](https://docs.godotengine.org/en/stable/classes/class_vector3i[].html)  | [find_path](#i_find_path) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) from_position, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) to_position )              
[void](#)                                                                           | [find_path_async](#i_find_path_async) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) from_position, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) to_position )  
[AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html)              | [get_region](#i_get_region) ( )                                                                                                                                                                                                     
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)              | [is_running_async](#i_is_running_async) ( ) const                                                                                                                                                                                   
[void](#)                                                                           | [set_region](#i_set_region) ( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) box )                                                                                                                          
[void](#)                                                                           | [set_terrain](#i_set_terrain) ( [VoxelTerrain](VoxelTerrain.md) terrain )                                                                                                                                                           
<p></p>

## 信号：<span id="signals"></span>

### async_search_completed( [Vector3i[]](https://docs.godotengine.org/en/stable/classes/class_vector3i[].html) path ) 

当通过 [find_path_async](VoxelAStarGrid3D.md#i_find_path_async) 触发的搜索完成时发出。

## 方法描述

### [Vector3i[]](https://docs.godotengine.org/en/stable/classes/class_vector3i[].html)<span id="i_debug_get_visited_positions"></span> **debug_get_visited_positions**( ) 

获取上次寻路请求所访问过的体素位置列表（与 A* 在底层的工作方式相关）。仅用于调试。

### [Vector3i[]](https://docs.godotengine.org/en/stable/classes/class_vector3i[].html)<span id="i_find_path"></span> **find_path**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) from_position, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) to_position ) 

计算从体素位置到目标体素位置的路径。

这些位置应为紧贴地面、且有足够空间容纳角色的空气体素。

返回的路径将是一系列连续的体素位置，按顺序行走即可到达目的地。

如果未找到路径，或者起点或终点位置位于搜索区域之外，将返回空数组。

你也可以使用 [set_region](VoxelAStarGrid3D.md#i_set_region) 指定搜索区域。

### [void](#)<span id="i_find_path_async"></span> **find_path_async**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) from_position, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) to_position ) 

与 [find_path](VoxelAStarGrid3D.md#i_find_path) 相同，但在单独的线程上执行计算。结果将通过 [VoxelAStarGrid3D.async_search_completed](VoxelAStarGrid3D.md#signals) 信号发出。

同一时间只能有一个异步搜索处于活动状态。可使用 [is_running_async](VoxelAStarGrid3D.md#i_is_running_async) 进行检查。

### [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html)<span id="i_get_region"></span> **get_region**( ) 

获取将纳入寻路考虑的最大区域限制，单位为体素。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_running_async"></span> **is_running_async**( ) 

如果当前正在异步计算路径，则返回 true。参见 [find_path_async](VoxelAStarGrid3D.md#i_find_path_async)。

### [void](#)<span id="i_set_region"></span> **set_region**( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) box ) 

设置将纳入寻路考虑的最大区域限制，单位为体素。通常应在调用 [find_path](VoxelAStarGrid3D.md#i_find_path) 之前设置。

区域越大，搜索开销可能越高。请记住体素体积按立方增长，因此不要在过大区域上使用（例如 50 个体素已经相当大）。

### [void](#)<span id="i_set_terrain"></span> **set_terrain**( [VoxelTerrain](VoxelTerrain.md) terrain ) 

设置用于执行搜索的地形。

_生成于 2026-08-28_
