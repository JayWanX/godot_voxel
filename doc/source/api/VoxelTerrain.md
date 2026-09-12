# VoxelTerrain

继承自：[VoxelNode](VoxelNode.md)

使用恒定细节层级的体素体积。

## 属性：


类型                                                                              | 名称                                                                                   | 默认值                                                                          
------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------ | -----------------------------------------------------------------------------
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [area_edit_notification_enabled](#i_area_edit_notification_enabled)                  | false                                                                        
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [automatic_loading_enabled](#i_automatic_loading_enabled)                            | true                                                                         
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [block_enter_notification_enabled](#i_block_enter_notification_enabled)              | false                                                                        
[AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html)          | [bounds](#i_bounds)                                                                  | AABB(-536870911, -536870911, -536870911, 1073741822, 1073741822, 1073741822) 
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [collision_layer](#i_collision_layer)                                                | 1                                                                            
[float](https://docs.godotengine.org/en/stable/classes/class_float.html)        | [collision_margin](#i_collision_margin)                                              | 0.04                                                                         
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [collision_mask](#i_collision_mask)                                                  | 1                                                                            
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [debug_draw_enabled](#i_debug_draw_enabled)                                          | false                                                                        
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [debug_draw_shadow_occluders](#i_debug_draw_shadow_occluders)                        | false                                                                        
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [debug_draw_visual_and_collision_blocks](#i_debug_draw_visual_and_collision_blocks)  | false                                                                        
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [debug_draw_volume_bounds](#i_debug_draw_volume_bounds)                              | false                                                                        
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [debug_draw_voxel_metadata](#i_debug_draw_voxel_metadata)                            | false                                                                        
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [generate_collisions](#i_generate_collisions)                                        | true                                                                         
[Material](https://docs.godotengine.org/en/stable/classes/class_material.html)  | [material_override](#i_material_override)                                            |                                                                              
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [max_view_distance](#i_max_view_distance)                                            | 128                                                                          
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)            | [mesh_block_size](#i_mesh_block_size)                                                | 16                                                                           
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)          | [use_gpu_generation](#i_use_gpu_generation)                                          | false                                                                        
<p></p>

## 方法：


返回值                                                                                             | 函数签名                                                                                                                                                                                                                                                                         
----------------------------------------------------------------------------------------------- | -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[void](#)                                                                                       | [_on_area_edited](#i__on_area_edited) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) area_origin, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) area_size ) virtual                                       
[void](#)                                                                                       | [_on_data_block_entered](#i__on_data_block_entered) ( [VoxelDataBlockEnterInfo](VoxelDataBlockEnterInfo.md) info ) virtual                                                                                                                                                   
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)                  | [data_block_to_voxel](#i_data_block_to_voxel) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) block_pos ) const                                                                                                                             
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [debug_get_draw_flag](#i_debug_get_draw_flag) ( [DebugDrawFlag](VoxelTerrain.md#enumerations) flag_index ) const                                                                                                                                                             
[void](#)                                                                                       | [debug_set_draw_flag](#i_debug_set_draw_flag) ( [DebugDrawFlag](VoxelTerrain.md#enumerations) flag_index, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled )                                                                                   
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)                            | [get_data_block_size](#i_get_data_block_size) ( ) const                                                                                                                                                                                                                      
[Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)              | [get_statistics](#i_get_statistics) ( ) const                                                                                                                                                                                                                                
[PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html)  | [get_viewer_network_peer_ids_in_area](#i_get_viewer_network_peer_ids_in_area) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) area_origin, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) area_size ) const 
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [has_data_block](#i_has_data_block) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) block_position ) const                                                                                                                                  
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [is_area_meshed](#i_is_area_meshed) ( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) area_in_voxels ) const                                                                                                                                          
[void](#)                                                                                       | [save_block](#i_save_block) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position )                                                                                                                                                      
[VoxelSaveCompletionTracker](VoxelSaveCompletionTracker.md)                                     | [save_modified_blocks](#i_save_modified_blocks) ( )                                                                                                                                                                                                                          
[bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)                          | [try_set_block_data](#i_try_set_block_data) ( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position, [VoxelBuffer](VoxelBuffer.md) voxels )                                                                                                
[Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)                  | [voxel_to_data_block](#i_voxel_to_data_block) ( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) voxel_pos ) const                                                                                                                               
<p></p>

## 信号：<span id="signals"></span>

### block_loaded( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position ) 

当从数据流加载新数据块时发出。这可能在网格或碰撞体可用之前发生。

### block_unloaded( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position ) 

当数据块因超出视距而被卸载时发出。

### mesh_block_entered( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position ) 

当网格区块自加入观察者范围以来首次收到更新时发出。这与网格是否为空无关。它跟踪与 [is_area_meshed](VoxelTerrain.md#i_is_area_meshed) 获得的相同状态的变化。

### mesh_block_exited( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position ) 

当网格区块被卸载时发出。它是 [VoxelTerrain.mesh_block_entered](VoxelTerrain.md#signals) 的对应信号。

## 枚举：<span id="enumerations"></span>

枚举 **DebugDrawFlag**：

- <span id="i_DEBUG_DRAW_VOLUME_BOUNDS"></span>**DEBUG_DRAW_VOLUME_BOUNDS** = **0**
- <span id="i_DEBUG_DRAW_VISUAL_AND_COLLISION_BLOCKS"></span>**DEBUG_DRAW_VISUAL_AND_COLLISION_BLOCKS** = **1**
- <span id="i_DEBUG_DRAW_VOXEL_METADATA"></span>**DEBUG_DRAW_VOXEL_METADATA** = **2**
- <span id="i_DEBUG_DRAW_FLAGS_COUNT"></span>**DEBUG_DRAW_FLAGS_COUNT** = **3**


## 属性描述

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_area_edit_notification_enabled"></span> **area_edit_notification_enabled** = false

区域被编辑时是否发送通知信号。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_automatic_loading_enabled"></span> **automatic_loading_enabled** = true

如果关闭，地形将不再在本地自动加载观察者周围的数据块。这可用于多人游戏场景中地形位于客户端的情况，因为数据块将由服务器发送。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_block_enter_notification_enabled"></span> **block_enter_notification_enabled** = false

数据块进入观察范围时是否发送通知信号。

### [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html)<span id="i_bounds"></span> **bounds** = AABB(-536870911, -536870911, -536870911, 1073741822, 1073741822, 1073741822)

定义地形允许存在体素的边界。如果使用无限世界生成器，数据块将仅在此区域内生成。区域外的所有内容都将保持为空。

如果新边界的任何维度大于 512 且 [max_view_distance](VoxelTerrain.md#i_max_view_distance) 大于 512，则 [max_view_distance](VoxelTerrain.md#i_max_view_distance) 将被钳制到 512。此措施是为了避免因可能加载的大量区块而导致崩溃。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_collision_layer"></span> **collision_layer** = 1

碰撞层的层级。

### [float](https://docs.godotengine.org/en/stable/classes/class_float.html)<span id="i_collision_margin"></span> **collision_margin** = 0.04

碰撞体的边缘裕量。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_collision_mask"></span> **collision_mask** = 1

碰撞掩码的层级。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_debug_draw_enabled"></span> **debug_draw_enabled** = false

是否启用调试绘制。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_debug_draw_shadow_occluders"></span> **debug_draw_shadow_occluders** = false

调试绘制阴影遮挡体。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_debug_draw_visual_and_collision_blocks"></span> **debug_draw_visual_and_collision_blocks** = false

调试绘制可视与碰撞数据块。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_debug_draw_volume_bounds"></span> **debug_draw_volume_bounds** = false

调试绘制生成的地形体积边界。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_debug_draw_voxel_metadata"></span> **debug_draw_voxel_metadata** = false

调试绘制体素元数据。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_generate_collisions"></span> **generate_collisions** = true

启用使用经典物理引擎生成碰撞形状。如果你需要真实或非平凡的碰撞或物理，请使用此功能。

注意 1：你还需要 [VoxelViewer](VoxelViewer.md) 请求碰撞，否则不会生成。

注意 2：如果你需要简单的 Minecraft/AABB 物理，可以使用 [VoxelBoxMover](VoxelBoxMover.md)，它在方块风世界中可能表现更好。

### [Material](https://docs.godotengine.org/en/stable/classes/class_material.html)<span id="i_material_override"></span> **material_override**

地形的材质覆盖。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_max_view_distance"></span> **max_view_distance** = 128

设置此地形可以支持的最大距离。如果 [VoxelViewer](VoxelViewer.md) 请求更大距离，它将被钳制。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_mesh_block_size"></span> **mesh_block_size** = 16

设置地形网格跨越多少个体素。

体素区块以 16x16x16 体素的立方区块存储，默认情况下地形网格与该尺寸匹配。但你可以将其设置为 32，使网格跨越 2x2x2 个体素区块。这是一种性能权衡。更大的网格尺寸可能加快渲染，但会减慢网格更新。

不支持 16 和 32 之外的值。

注意：此设置也会影响 [VoxelInstancer](VoxelInstancer.md) 区块。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_use_gpu_generation"></span> **use_gpu_generation** = false

启用 GPU 数据块生成，可加快生成速度。仅对支持它的生成器有效。需要 Vulkan。

## 方法描述

### [void](#)<span id="i__on_area_edited"></span> **_on_area_edited**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) area_origin, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) area_size ) 

区域被编辑时触发的回调。

### [void](#)<span id="i__on_data_block_entered"></span> **_on_data_block_entered**( [VoxelDataBlockEnterInfo](VoxelDataBlockEnterInfo.md) info ) 

数据块进入观察范围时触发的回调。

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_data_block_to_voxel"></span> **data_block_to_voxel**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) block_pos ) 

将数据块坐标转换为体素坐标。数据块的体素坐标对应于其最低角。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_debug_get_draw_flag"></span> **debug_get_draw_flag**( [DebugDrawFlag](VoxelTerrain.md#enumerations) flag_index ) 

获取调试绘制标志。

### [void](#)<span id="i_debug_set_draw_flag"></span> **debug_set_draw_flag**( [DebugDrawFlag](VoxelTerrain.md#enumerations) flag_index, [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html) enabled ) 

设置调试绘制标志。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i_get_data_block_size"></span> **get_data_block_size**( ) 

获取数据块大小。

### [Dictionary](https://docs.godotengine.org/en/stable/classes/class_dictionary.html)<span id="i_get_statistics"></span> **get_statistics**( ) 

获取关于处理地形所花费时间的调试信息。

返回的字典具有以下结构：

```
{
	"time_detect_required_blocks": int,
	"time_request_blocks_to_load": int,
	"time_process_load_responses": int,
	"time_request_blocks_to_update": int,
	"time_process_update_responses": int,
	"remaining_main_thread_blocks": int,
	"dropped_block_loads": int,
	"dropped_block_meshs": int,
	"updated_blocks": int
}
```

### [PackedInt32Array](https://docs.godotengine.org/en/stable/classes/class_packedint32array.html)<span id="i_get_viewer_network_peer_ids_in_area"></span> **get_viewer_network_peer_ids_in_area**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) area_origin, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) area_size ) 

获取指定区域内观察者的网络对等体 ID 列表。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_has_data_block"></span> **has_data_block**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) block_position ) 

指定位置是否存在数据块。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_is_area_meshed"></span> **is_area_meshed**( [AABB](https://docs.godotengine.org/en/stable/classes/class_aabb.html) area_in_voxels ) 

如果该区域已被网格化处理，则返回 true。这并不意味着该区域实际包含网格。

如果该区域尚未经过网格化处理（因此此处是否存在网格未知），则返回 false。

对于流式加载的地形，可用于确定某个区域是否已完全"加载"，以防游戏依赖网格或网格碰撞体。

### [void](#)<span id="i_save_block"></span> **save_block**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position ) 

强制保存特定数据块。

注意 1：只有当地形上设置的数据流支持保存时，这才有效。

注意 2：保存是异步的，不会阻塞游戏。保存可能只在你调用此方法后很短的时间内完成。

### [VoxelSaveCompletionTracker](VoxelSaveCompletionTracker.md)<span id="i_save_modified_blocks"></span> **save_modified_blocks**( ) 

强制保存所有已修改的数据块。

注意 1：只有当地形上设置的数据流支持保存时，这才有效。

注意 2：保存是异步的，不会阻塞游戏。保存可能只在你调用此方法后很短的时间内完成。

使用返回的跟踪器对象来了解保存何时完成。但是，在调用此方法之后发生的保存将不会被此对象跟踪。

注意，当观察者移动时被卸载的数据块也会触发保存任务，这与本函数无关。

### [bool](https://docs.godotengine.org/en/stable/classes/class_bool.html)<span id="i_try_set_block_data"></span> **try_set_block_data**( [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position, [VoxelBuffer](VoxelBuffer.md) voxels ) 

在给定位置创建或覆盖任何已有的数据块数据。若本地没有观察者在范围内则返回 false。

### [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html)<span id="i_voxel_to_data_block"></span> **voxel_to_data_block**( [Vector3](https://docs.godotengine.org/en/stable/classes/class_vector3.html) voxel_pos ) 

将体素坐标转换为数据块坐标。

_生成于 2026-09-12_
