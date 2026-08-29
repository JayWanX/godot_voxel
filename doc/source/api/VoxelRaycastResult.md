# VoxelRaycastResult

继承自：[RefCounted](https://docs.godotengine.org/en/stable/classes/class_refcounted.html)

通过 [VoxelTool.raycast](VoxelTool.md#i_raycast) 执行的射线投射的结果。

## 属性：


类型                                                                              | 名称                                         | 默认值               
------------------------------------------------------------------------------- | ------------------------------------------ | ------------------
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)        | [distance](#i_distance)                    | 0.0               
[Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)    | [normal](#i_normal)                        | Vector3(0, 0, 0)  
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)  | [position](#i_position)                    | Vector3i(0, 0, 0) 
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)  | [previous_position](#i_previous_position)  | Vector3i(0, 0, 0) 
<p></p>

## 属性描述

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_distance"></span> **distance** = 0.0

射线原点与被击中体素所表示立方体表面之间的距离。

### [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html)<span id="i_normal"></span> **normal** = Vector3(0, 0, 0)

指向背离被击中表面的单位向量。

仅当 [VoxelTool](VoxelTool.md) 配置为计算法线时才可用。参见 [VoxelTool.set_raycast_normal_enabled](VoxelTool.md#i_set_raycast_normal_enabled)。

对于方块风体素，此法线将基于碰撞盒而非网格。

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_position"></span> **position** = Vector3i(0, 0, 0)

被击中体素的整数位置。在方块风游戏中，这将是与之交互的体素的位置。

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_previous_position"></span> **previous_position** = Vector3i(0, 0, 0)

最终命中前，沿射线的上一个体素的整数位置。在方块风游戏中，这将是放置于被指向体素之上的体素的位置。

_生成于 2026-08-28_
