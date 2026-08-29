# VoxelViewer

继承自：[Node3D](https://docs.godotengine.org/en/stable/classes/class_node3d.html)

将此节点附加为角色的子节点，以便体素世界知道应在角色周围加载哪些数据块。

如果世界中不存在观察者，则不会生成任何内容。

## 描述：

体素世界使用所有 [VoxelViewer](VoxelViewer.md) 节点的位置和选项来确定在哪里加载体素数据块，并确定更新优先级。例如，放置在距玩家 100 单位的体素，其优先级将远低于玩家在面前挖掘时正在进行的修改。

## 属性：


类型                                                                        | 名称                                                                         | 默认值   
------------------------------------------------------------------------- | -------------------------------------------------------------------------- | ------
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [enabled_in_editor](#i_enabled_in_editor)                                  | false 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [requires_collisions](#i_requires_collisions)                              | true  
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [requires_data_block_notifications](#i_requires_data_block_notifications)  | false 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [requires_visuals](#i_requires_visuals)                                    | true  
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [view_distance](#i_view_distance)                                          | 128   
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [view_distance_vertical_ratio](#i_view_distance_vertical_ratio)            | 1.0   
<p></p>

## 方法：


返回值                                                                   | 函数签名                                                                                                                       
--------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [get_network_peer_id](#i_get_network_peer_id) ( ) const                                                                    
[void](#)                                                             | [set_network_peer_id](#i_set_network_peer_id) ( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id )  
<p></p>

## 属性描述

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_enabled_in_editor"></span> **enabled_in_editor** = false

设置此观察者是否会在编辑器中引起加载。这主要用于测试目的。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_requires_collisions"></span> **requires_collisions** = true

如果设置为 `true`，引擎将在此观察者周围生成经典碰撞形状。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_requires_data_block_notifications"></span> **requires_data_block_notifications** = false

*(此属性暂无文档)*

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_requires_visuals"></span> **requires_visuals** = true

如果设置为 `true`，引擎将在此观察者周围生成网格。此选项可为本地玩家启用。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_view_distance"></span> **view_distance** = 128

此观察者周围应生成多远的体素。

注意：实际视距可能受地形节点的限制。

注意 2：使用 [VoxelLodTerrain](VoxelLodTerrain.md) 时，此距离本质上相当于最后一个 LOD 延伸距离的限制。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_view_distance_vertical_ratio"></span> **view_distance_vertical_ratio** = 1.0

将垂直视距修改为 [view_distance](VoxelViewer.md#i_view_distance) 属性的一定比例。例如，如果 [view_distance](VoxelViewer.md#i_view_distance) 为 100 且此属性为 0.5，则水平视距为 100，垂直视距为 50。

此属性存在限制：仅在 [VoxelLodTerrain](VoxelLodTerrain.md) 使用 [VoxelLodTerrain.STREAMING_SYSTEM_CLIPBOX](VoxelLodTerrain.md#i_STREAMING_SYSTEM_CLIPBOX) 时实现，并且像视距一样，仅适用于最后一个 LOD。

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_network_peer_id"></span> **get_network_peer_id**( ) 

*(此方法暂无文档)*

### [void](#)<span id="i_set_network_peer_id"></span> **set_network_peer_id**( [int](https://docs.godotengine.org/en/stable/classes/class_int.html) id ) 

*(此方法暂无文档)*

_生成于 2026-08-28_
