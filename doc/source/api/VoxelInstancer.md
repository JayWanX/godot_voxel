# VoxelInstancer

继承自：[Node3D](https://docs.godotengine.org/en/stable/classes/class_node3d.html)

在体素表面之上生成条目。

## 描述：

体素节点的附加组件，允许在表面生成静态元素。这些元素使用硬件实例化渲染，可以拥有碰撞，并且可以是持久化的。它必须是体素节点的子节点。

## 属性：


类型                                                                        | 名称                                                                               | 默认值                    
------------------------------------------------------------------------- | -------------------------------------------------------------------------------- | -----------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [collision_update_budget_microseconds](#i_collision_update_budget_microseconds)  | 500                    
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)  | [fading_duration](#i_fading_duration)                                            | 0.3                    
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)    | [fading_enabled](#i_fading_enabled)                                              | false                  
[VoxelInstanceLibrary](VoxelInstanceLibrary.md)                           | [library](#i_library)                                                            |                        
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)      | [mesh_lod_update_budget_microseconds](#i_mesh_lod_update_budget_microseconds)    | 500                    
[UpMode](VoxelInstancer.md#enumerations)                                  | [up_mode](#i_up_mode)                                                            | UP_MODE_POSITIVE_Y (0) 
<p></p>

## 方法：


返回值                                                                                 | 函数签名                                                                                                                                                                                                                                  
----------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                           | [debug_dump_as_scene](#i_debug_dump_as_scene) ( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) fpath ) const                                                                                              
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                | [debug_get_block_count](#i_debug_get_block_count) ( ) const                                                                                                                                                                           
[Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)  | [debug_get_block_infos](#i_debug_get_block_infos) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) world_position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) item_id )       
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)              | [debug_get_draw_flag](#i_debug_get_draw_flag) ( [DebugDrawFlag](VoxelInstancer.md#enumerations) flag ) const                                                                                                                          
[Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)  | [debug_get_instance_counts](#i_debug_get_instance_counts) ( ) const                                                                                                                                                                   
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)              | [debug_is_draw_enabled](#i_debug_is_draw_enabled) ( ) const                                                                                                                                                                           
[void](#)                                                                           | [debug_set_draw_enabled](#i_debug_set_draw_enabled) ( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled )                                                                                                
[void](#)                                                                           | [debug_set_draw_flag](#i_debug_set_draw_flag) ( [DebugDrawFlag](VoxelInstancer.md#enumerations) flag, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled )                                                
[void](#)                                                                           | [remove_instances_in_sphere](#i_remove_instances_in_sphere) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius )  
<p></p>

## 枚举：<span id="enumerations"></span>

枚举 **UpMode**：

- <span id="i_UP_MODE_POSITIVE_Y"></span>**UP_MODE_POSITIVE_Y** = **0** --- 上方朝向正 Y 轴。这是 Godot 中的默认假设。
- <span id="i_UP_MODE_SPHERE"></span>**UP_MODE_SPHERE** = **1** --- 上方与地形原点所在方向相反。例如，如果你的地形是一个星球，可以使用此模式。

枚举 **DebugDrawFlag**：

- <span id="i_DEBUG_DRAW_ALL_BLOCKS"></span>**DEBUG_DRAW_ALL_BLOCKS** = **0**
- <span id="i_DEBUG_DRAW_EDITED_BLOCKS"></span>**DEBUG_DRAW_EDITED_BLOCKS** = **1**
- <span id="i_DEBUG_DRAW_FLAGS_COUNT"></span>**DEBUG_DRAW_FLAGS_COUNT** = **2**


## 常量：

- <span id="i_MAX_LOD"></span>**MAX_LOD** = **8**

## 属性描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_collision_update_budget_microseconds"></span> **collision_update_budget_microseconds** = 500

提示应花费在更新基于距离的碰撞（参见 [VoxelInstanceLibraryMultiMeshItem.collision_distance](VoxelInstanceLibraryMultiMeshItem.md#i_collision_distance)）上的最大时间。如果某项操作耗时更长，该时间仍可能被超出，但在下一帧之前将不再进行进一步处理。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_fading_duration"></span> **fading_duration** = 0.3

淡入淡出完成所需的时间（以秒为单位，淡入和淡出相同）。当 [fading_enabled](VoxelInstancer.md#i_fading_enabled) 生效时使用。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_fading_enabled"></span> **fading_enabled** = false

启用后，当实例加载和卸载时，将更新每个实例的着色器参数，以便用于应用淡入淡出效果。

实例使用的材质必须具有包含 `instance uniform vec2 u_lod_fade;` 属性的着色器。X 将在淡入和淡出期间从 0 动画到 1。淡入时 Y 为 1，淡出时 Y 为 0。

### [VoxelInstanceLibrary](VoxelInstanceLibrary.md)<span id="i_library"></span> **library**

要生成的实例将取自的库。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_mesh_lod_update_budget_microseconds"></span> **mesh_lod_update_budget_microseconds** = 500

提示应花费在更新网格 LOD（参见 [VoxelInstanceLibraryMultiMeshItem](VoxelInstanceLibraryMultiMeshItem.md) 上的“网格 LOD”属性）上的最大时间。如果某项操作耗时更长，该时间仍可能被超出，但在下一帧之前将不再进行进一步处理。

### [UpMode](VoxelInstancer.md#enumerations)<span id="i_up_mode"></span> **up_mode** = UP_MODE_POSITIVE_Y (0)

生成实例时，认为地形上的“上方”方向位于何处。另请参见 [VoxelInstanceGenerator](VoxelInstanceGenerator.md)。

## 方法描述

### [void](#)<span id="i_debug_dump_as_scene"></span> **debug_dump_as_scene**( [String](https://docs.godotengine.org/en/stable/classes/class_string.html) fpath ) 

将全部实例导出为一个场景文件。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_debug_get_block_count"></span> **debug_get_block_count**( ) 

获取数据块数量。

### [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)<span id="i_debug_get_block_infos"></span> **debug_get_block_infos**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) world_position, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) item_id ) 

获取指定位置的块调试信息。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_debug_get_draw_flag"></span> **debug_get_draw_flag**( [DebugDrawFlag](VoxelInstancer.md#enumerations) flag ) 

获取调试绘制标志。

### [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)<span id="i_debug_get_instance_counts"></span> **debug_get_instance_counts**( ) 

统计各图层的实例数量。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_debug_is_draw_enabled"></span> **debug_is_draw_enabled**( ) 

调试绘制是否启用。

### [void](#)<span id="i_debug_set_draw_enabled"></span> **debug_set_draw_enabled**( [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled ) 

设置是否启用调试绘制。

### [void](#)<span id="i_debug_set_draw_flag"></span> **debug_set_draw_flag**( [DebugDrawFlag](VoxelInstancer.md#enumerations) flag, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled ) 

设置调试绘制标志。

### [void](#)<span id="i_remove_instances_in_sphere"></span> **remove_instances_in_sphere**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) center, [float](https://docs.godotengine.org/en/stable/classes/class_float.html) radius ) 

移除所有原点位于给定球体内的实例。坐标为相对于实例化器的局部坐标。

_生成于 2026-09-12_
