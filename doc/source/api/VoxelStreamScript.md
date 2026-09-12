# VoxelStreamScript

继承自：[VoxelStream](VoxelStream.md)

使用脚本定义的自定义数据流的基类。

## 方法：


返回值                                                                   | 函数签名                                                                                                                                                                                                                                                                        
--------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [_get_used_channels_mask](#i__get_used_channels_mask) ( ) virtual const                                                                                                                                                                                                     
[int](https://docs.godotengine.org/en/stable/classes/class_int.html)  | [_load_voxel_block](#i__load_voxel_block) ( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position_in_blocks, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod ) virtual 
[void](#)                                                             | [_save_voxel_block](#i__save_voxel_block) ( [VoxelBuffer](VoxelBuffer.md) buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position_in_blocks, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod ) virtual     
<p></p>

## 方法描述

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i__get_used_channels_mask"></span> **_get_used_channels_mask**( ) 

告知 [VoxelBuffer](VoxelBuffer.md) 中的哪些通道支持保存体素数据，以防数据流只保存特定通道。

### [int](https://docs.godotengine.org/en/stable/classes/class_int.html)<span id="i__load_voxel_block"></span> **_load_voxel_block**( [VoxelBuffer](VoxelBuffer.md) out_buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position_in_blocks, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod ) 

当需要加载体素数据块时调用。假定 `out_buffer` 始终具有相同大小。返回 [ResultCode](VoxelStream.md#enumerations)。

### [void](#)<span id="i__save_voxel_block"></span> **_save_voxel_block**( [VoxelBuffer](VoxelBuffer.md) buffer, [Vector3i](https://docs.godotengine.org/en/stable/classes/class_vector3i.html) position_in_blocks, [int](https://docs.godotengine.org/en/stable/classes/class_int.html) lod ) 

当需要保存体素数据块时调用。假定 `out_buffer` 始终具有相同大小。

_生成于 2026-09-12_
